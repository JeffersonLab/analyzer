/////////////////////////////////////////////////////////////////////
//
//   ToyCodaDecoder
//
//   A toy model of an improved THaCodaDecoder.  
//   Work in progress towards OO decoder upgrade.
//   Dec 2013.  Bob Michaels
//
/////////////////////////////////////////////////////////////////////

#include "ToyCodaDecoder.h"
#include "ToyEvtTypeHandler.h"
#include "ToyPhysicsEvtHandler.h"
#include "ToyScalerEvtHandler.h"
#include "THaFastBusWord.h"
#include "THaCrateMap.h"
#include "THaEpics.h"
#include "THaUsrstrutils.h"
#include "THaBenchmark.h"
#include "ToyModule.h"
#include "TError.h"
#include <cstring>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <ctime>

#ifndef STANDALONE
#include "THaVarList.h"
#include "THaGlobals.h"
#endif

using namespace std;

//_____________________________________________________________________________
ToyCodaDecoder::ToyCodaDecoder() 
{
  memset(psfact,0,MAX_PSFACT*sizeof(*psfact));
}

//_____________________________________________________________________________
ToyCodaDecoder::~ToyCodaDecoder()
{
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetPrescaleFactor(Int_t trigger_type) const
{// PUBLIC

  // To get the prescale factors for trigger number "trigger_type"
  // (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
    return psfact[trigger_type - 1];
  }
  if (TestBit(kVerbose)) {
    cout << "Warning:  Requested prescale factor for ";
    cout << "un-prescalable trigger "<<dec<<trigger_type<<endl;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::LoadEvent(const Int_t* evbuffer, THaCrateMap* map)
{//PUBLIC
  // Main engine for decoding, called by public LoadEvent() methods
  // The crate map argument is ignored. Use SetCrateMapName instead
  assert( evbuffer );
  assert( fMap || fgNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  //  if(TestBit(kDebug)) dump(evbuffer);
  if (first_decode || fgNeedInit) {
    ret = init_cmap();
    if( ret != HED_OK ) return ret;
    ret = init_slotdata(fMap);
    if( ret != HED_OK ) return ret;
    // NEW STUFF
    if(first_decode) {
      InitHandlers();
      first_decode=kFALSE;
    }
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( Int_t i=0; i<fNSlotClear; i++ )
    crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_type = evbuffer[1]>>16;
  if(event_type <= 0) return HED_ERR;
// NEW STUFF
  cout << "Here AAAAA event_type = "<<dec<<event_type<<"  "<<evidx[event_type]<<endl;
  if (evidx[event_type] >= 0) {
      event_handler[evidx[event_type]]->Decode(this);
  }

  cout << "num slots ? "<<GetNslots()<<endl;

  return ret;
}


// To initialize the THaSlotData member on first call to decoder
int ToyCodaDecoder::init_slotdata(const THaCrateMap* map)
{
  // Update lists of used/clearable slots in case crate map changed
  cout << "into TOY DECODER init slotdata "<<map<<"   "<<fNSlotUsed<<endl;
  if(!map) return HED_ERR;

  for (Int_t iroc = 0; iroc<MAXROC; iroc++) {
    if (  !map->crateUsed(iroc)  ) continue;

    for (Int_t islot=0; islot < MAXSLOT; islot++) {

      if ( !map->slotUsed(iroc,islot) ) continue;

        makeidx(iroc,islot);
        cout << "Num slots defined "<<GetNslots()<<endl;

    }
  }

  for( int i=0; i<fNSlotUsed; i++ ) {
    THaSlotData* crslot = crateslot[fSlotUsed[i]];
    int crate = crslot->getCrate();
    int slot  = crslot->getSlot();
    cout << "print of crate slot ---   crate "<<crate<<"   slot "<<slot<<endl;
    crslot->print();
    crslot->loadModule(map);
    cout << "Dev type xxx  = "<<crslot->devType()<<endl;
// New(5/2014) line to define the module information
// TBD    crslot->loadModule(map->GetModuleInfo(crate,slot));
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot) ||
	!map->slotClear(crate,slot)) {
      for( int k=0; k<fNSlotClear; k++ ) {
	if( crslot == crateslot[fSlotClear[k]] ) {
	  for( int j=k+1; j<fNSlotClear; j++ )
	    fSlotClear[j-1] = fSlotClear[j];
	  fNSlotClear--;
	  break;
	}
      }
    }
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot)) {
      for( int j=i+1; j<fNSlotUsed; j++ )
	fSlotUsed[j-1] = fSlotUsed[j];
      fNSlotUsed--;
    }
  }


  return HED_OK;

}


//_____________________________________________________________________________
void ToyCodaDecoder::dump(const Int_t* evbuffer)
{
  if( !evbuffer ) return;
  Int_t len = evbuffer[0]+1;  
  Int_t type = evbuffer[1]>>16;
  Int_t num = evbuffer[4];
  cout << "\n\n Raw Data Dump  " << hex << endl;
  cout << "\n Event number  " << dec << num;
  cout << "  length " << len << "  type " << type << endl;
  Int_t ipt = 0;
  for (Int_t j=0; j<(len/5); j++) {
    cout << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=j; k<j+5; k++) {
      cout << hex << evbuffer[ipt++] << " ";
    }
  }
  if (ipt < len) {
    cout << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=ipt; k<len; k++) {
      cout << hex << evbuffer[ipt++] << " ";
    }
    cout << endl;
  }
  cout<<dec<<endl;
}


//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetScaler( const TString& spec, Int_t slot,
				 Int_t chan ) const
{// PUBLIC
  Int_t roc;
  roc = 0; //scalerdef[spec];
  if (roc >= 0) return GetScaler(roc, slot, chan);
  return 0;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetScaler(Int_t roc, Int_t slot, Int_t chan) const
{// PUBLIC
  // Get scaler data by roc, slot, chan.
  assert( ScalersEnabled() );  // Should never request data when not enabled
  assert( GoodIndex(roc,slot) );
  return crateslot[idx(roc,slot)]->getData(chan,0);
}

// This, and other routines, to be deprecated since there will be
// an EPICS event handler
//_____________________________________________________________________________
Bool_t ToyCodaDecoder::IsLoadedEpics(const char* tag) const
{// PUBLIC
  // Test if the given EPICS variable name has been loaded
  assert( tag );
  return kTRUE; //epics->IsLoaded(tag);
}

// This, and other routines, to be deprecated since there will be
// an EPICS event handler
//_____________________________________________________________________________
double ToyCodaDecoder::GetEpicsData(const char* tag, Int_t event) const
{//PUBLIC
  // EPICS data which is nearest CODA event# 'event'
  // event == 0 --> get latest data
  // Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  return 0;  //epics->GetData(tag, event);
}

// This, and other routines, to be deprecated since there will be
// an EPICS event handler
//_____________________________________________________________________________
string ToyCodaDecoder::GetEpicsString(const char* tag, Int_t event) const
{ // PUBLIC
  // EPICS string data which is nearest CODA event# 'event'
  // event == 0 --> get latest data

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  return ""; //epics->GetString(tag, event);
}

// This, and other routines, to be deprecated since there will be
// an EPICS event handler
//_____________________________________________________________________________
double ToyCodaDecoder::GetEpicsTime(const char* tag, Int_t event) const
{//PUBLIC
  // EPICS time stamp
  // event == 0 --> get latest data
  return 0; //epics->GetTimeStamp(tag, event);
}

//_____________________________________________________________________________
void ToyCodaDecoder::SetRunTime( ULong64_t tloc )
{// PUBLIC
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.

  if( fRunTime == tloc ) 
    return;
  fRunTime = tloc;
  fgNeedInit = true;  // force re-init
}

//_____________________________________________________________________________
void ToyCodaDecoder::InitHandlers() 
{ 
  // NEW STUFF
  // This will get replaced by code that reads a database
  // and defines event handlers for each event type.

  for (Int_t ievtype=0; ievtype<MAX_EVTYPES; ievtype++) {
      evidx.push_back(-1);  // initially undefined.
  }
  for (Int_t ievtype=1; ievtype<MAX_PHYS_EVTYPES; ievtype++) {
    evidx[ievtype] = 0;    
  }
  event_handler.push_back(new ToyPhysicsEvtHandler());
  evidx[SCALER_EVTYPE] = event_handler.size();  
  event_handler.push_back(new ToyScalerEvtHandler());
#ifdef THING1
  evidx[EPICS_EVTYPE] = event_handler.size();   
  event_handler.push_back(new ToyEpicsEvtHandler());
  evidx[PRESCALE_EVTYPE] = event_handler.size();  
  event_handler.push_back(new ToyPrescaleEvtHandler());
#endif
  cout << "ToyCodaDecoder:  event_handler size "<<dec<<event_handler.size()<<endl;


  for (UInt_t i=0; i<event_handler.size(); i++) {
    event_handler[i]->Init(fMap);
  }
  
}


//_____________________________________________________________________________
ClassImp(ToyCodaDecoder)
