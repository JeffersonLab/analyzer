/////////////////////////////////////////////////////////////////////
//
//   ToyCodaDecoder
//   Hall A Event Data from one CODA "Event"
//
//   A toy model of an improved THaCodaDecoder
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
Int_t ToyCodaDecoder::LoadEvent(const Int_t* evbuffer)
{//PUBLIC
  // Main engine for decoding, called by public LoadEvent() methods
  // The crate map argument is ignored. Use SetCrateMapName instead
  assert( evbuffer );
  assert( fMap || fgNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  if(TestBit(kDebug)) dump(evbuffer);
  if (first_decode || fgNeedInit) {
    ret = init_cmap();
    if( ret != HED_OK ) return ret;
    ret = init_slotdata(fMap);
    if( ret != HED_OK ) return ret;
    // NEW STUFF
    if(first_decode) {
      InitHandlers();
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
  if (evidx[event_type] > 0) {
      event_handler[evidx[event_type]]->Decode(this);
  }
  return ret;
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
#ifdef LATER_ON
  evidx[EPICS_EVTYPE] = event_handler.size();   
  event_handler.push_back(new ToyEpicsEvtHandler());
  evidx[PRESCALE_EVTYPE] = event_handler.size();  
  event_handler.push_back(new ToyPrescaleEvtHandler());
#endif

  for (UInt_t i=0; i<event_handler.size(); i++) {
    event_handler[i]->Init(fMap);
  }
  
}



//_____________________________________________________________________________
ClassImp(ToyCodaDecoder)
