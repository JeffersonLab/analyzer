//*-- Author :    Bob Michaels,  March 2002

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
// Hall A miscellaneous decoder data, which typically does not 
// belong to a detector class.
// Provides global data to analyzer, and
// a place to rapidly add new channels.
//
// Normally the user should have a file "decdata.map" in their pwd
// to define the locations of raw data for this class (only).
// But if this file is not found, we define a default mapping, which
// was valid at least at one point in history.
//
// The scheme is as follows:
//
//    1. In Init() we define a list of global variables which are tied
//       to the variables of this class.  E.g. "timeroc2".
//
//    2. Next we build a list of "BdataLoc" objects which store information
//       about where the data are located.  These data are either directly
//       related to the variables of this class (e.g. timeroc2 is a raw
//       data word) or one must analyze them to obtain a variable.
//
//    3. The BdataLoc objects may be defined by decdata.map which has an
//       obvious notation (see ~/examples/decdata.map).  The entries are either 
//       locations in crates or locations relative to a unique header.
//       If decdata.map is not in the pwd where you run analyzer, then this
//       class uses its own internal DefaultMap().
//
//    4. The BdataLoc objects pertain to one data channel (e.g. a fastbus
//       channel) and and may be multihit.
//
//    5. To add a new variable, if it is on a single-hit channel, you may
//       imitate 'synchadc1' if you know the (crate,slot,chan), and 
//       imitate 'timeroc2' if you know the (crate,header,no-to-skip).
//       If your variable is more complicated and relies on several 
//       channels, imitate the way 'bits' leads to 'evtypebits'.
//
// R. Michaels, March 2002
//
// OR  (NEW as of April 2004:  R.J. Feuerbach)
//       If you are simply interested in the readout of a channel, create
//       a name for it and give the location in the map file and a
//       global variable will be automatically created to monitor that channel.
//     Unfortunately, this leads to a limitation of using arrays as opposed
//     to variable-sized vector for the readout. Currently limited to 16 hits
//     per channel per event.
//
//////////////////////////////////////////////////////////////////////////

//#define WITH_DEBUG 1

#include "THaDecData.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "TDatime.h"
#include "TH1.h"
#include "TClass.h"
#include "VarDef.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cstdlib>

using namespace std;

typedef vector<BdataLoc*>::iterator Iter_t;

THaDecData* THaDecData::fgThis = NULL;  //Pointer to single instance of this class
Int_t  THaDecData::fgVdcEffFirst = 2;


//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript ) : 
  THaApparatus( name, descript )
{
  if( fgThis ) {
    Error("THaDecData", "Only one instance of THaDecData allowed. "
	  "Object creation failed.");
    MakeZombie();
    return;
  }
  Reset();

  fgThis = this;
}

//_____________________________________________________________________________
THaDecData::~THaDecData()
{

  // Dtor. Remove global variables.

  if( fgThis == this ) {
    SetupDecData( NULL, kDelete ); 
    fgThis = NULL;
  }
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* opt ) 
{
  // Clear the object (set event-by-event data to zero)

  evtypebits = 0;
  evtype     = 0;
  ctimel     = 0;
  ctimer     = 0;
  pulser1    = 0;
  synchadc1  = 0;
  synchadc2  = 0;
  synchadc3  = 0;
  synchadc4  = 0;
  synchadc14 = 0;
  timestamp  = 0;
  timeroc1   = 0;
  timeroc2   = 0;
  timeroc3   = 0;
  timeroc4   = 0;
  timeroc14  = 0;
  rftime1    = 0;
  rftime2    = 0;
  edtpl      = 0;
  edtpr      = 0;
  lenroc12   = 0;
  lenroc16   = 0;
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++)  (*p)->Clear();
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) (*p)->Clear();
}

//_____________________________________________________________________________
void THaDecData::Reset( Option_t* opt ) 
{
  // Reset the object (zero all data, including histograms)

  Clear();
  for( vector<TH1F*>::iterator it = hist.begin(); it != hist.end(); it++ )
    (*it)->Reset();
  fgVdcEffFirst = 2;
}

//_____________________________________________________________________________
Int_t THaDecData::SetupDecData( const TDatime* run_time, EMode mode )
{
  // Register global variables, open decdata map file, and parse it.
  // If mode == kDelete, remove global variables.

  Int_t retval = 0;

  RVarDef vars[] = {
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { "evtype",     "event type from bit pattern", "evtype" },  
    { "ctimel",     "coincidence time on L-arm",   "ctimel" },  
    { "ctimer",     "coincidence time on R-arm",   "ctimer" },  
    { "pulser1",    "pulser in a TDC",             "pulser1" },  
    { "synchadc1",  "synch check adc 1",           "synchadc1" },       
    { "synchadc2",  "synch check adc 2",           "synchadc2" },       
    { "synchadc3",  "synch check adc 3",           "synchadc3" },       
    { "synchadc4",  "synch check adc 4",           "synchadc4" },       
    { "synchadc14", "synch check adc 14",          "synchadc14" },
    { "times100k",  "100kHz time stamp",           "timestamp" },       
    { "timeroc1",   "time stamp roc 1",            "timeroc1" },         
    { "timeroc2",   "time stamp roc 2",            "timeroc2" },         
    { "timeroc3",   "time stamp roc 3",            "timeroc3" },         
    { "timeroc4",   "time stamp roc 4",            "timeroc4" },         
    { "timeroc14",  "time stamp roc 14",           "timeroc14" },         
    { "rftime1",    "RF time copy 1",              "rftime1" },         
    { "rftime2",    "RF time copy 2",              "rftime2" },         
    { "edtpl",      "EDT pulser on L-arm",         "edtpl" },   
    { "edtpr",      "EDT pulser on R-arm",         "edtpr" },   
    { "lenroc12",   "ROC12 event length",         "lenroc12" },   
    { "lenroc16",   "ROC16 event length",         "lenroc16" },   
    { 0 }
  };

  Bool_t re_init = ((mode == kDefine) && fIsSetup);
  if( !re_init )
    retval = DefineVarsFromList( vars, mode );

  fIsSetup = ( mode == kDefine );

  if( mode != kDefine ) {
    // Undefine the dynamically-defined global variables
     for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++ ) {
      DefineChannel(*p,mode);
      if( mode == kDelete )
	delete *p;
    }
    for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++ ) {
      DefineChannel(*p,mode);
      if( mode == kDelete )
	delete *p;
    }
    if( mode == kDelete ) {
      for( vector<TH1F*>::iterator it = hist.begin(); it != hist.end(); it++ )
	delete *it;
      fCrateLoc.clear();   
      fWordLoc.clear(); 
      hist.clear();
    }

  } else {

// Set up the locations in raw data corresponding to variables of this class. 
// Each element of a BdataLoc is one channel or word.  Since a channel 
// may be a multihit device, the BdataLoc stores data in a vector.

    if( !re_init ) {
      fCrateLoc.clear();   
      fWordLoc.clear(); 
      BookHist();
    }  

    ifstream decdatafile;

    const char* const here = "SetupDecData()";
    const char* name = GetDBFileName();

    TDatime date;
    if( run_time ) date = *run_time;
    vector<string> fnames = GetDBFileList( name, date, Here(here));
    // always look for 'decdata.map' in the current directory first.
    fnames.insert(fnames.begin(),string("decdata.map"));
    if( !fnames.empty() ) {
      vector<string>::iterator it = fnames.begin();
      do {
	if( fDebug>0 ) {
	  cout << "<" << IsA()->GetName() << "::" << Here(here)
	       << ">: Opening database file " << *it;
	}
	decdatafile.clear();  // Forget previous failures before attempt
	decdatafile.open((*it).c_str());

	if( fDebug>0 ) {
	  if( !decdatafile ) cout << " ... failed" << endl;
	  else               cout << " ... ok" << endl;
	}
      } while ( !decdatafile && ++it != fnames.end() );
    }
    if( fnames.empty() || !decdatafile ) {
      if( !re_init ) {
	if( fDebug>0 )
	  Warning( Here(here), "File db_%s.dat not found.\nAn example of this "
		   "file should be in the examples directory.\nWill proceed "
		   "with default mapping for THaDecData.", name );
	return DefaultMap();
      } else {
	if( fDebug>0 )
	  Warning( Here(here), "File db_%s.dat not found for timestamp %s.\n"
		   "Variable definitions unchanged from prior initialization.\n"
		   "Update database to be sure you have valid data.",
		   name, date.AsString() );
	return retval;
      }
    }

    string sinput;
    const string comment = "#";
    while (getline(decdatafile, sinput)) {
      if( fDebug>3 )
	Info( Here(here), "sinput = %s", sinput.c_str() );
      vector<string> strvect( vsplit(sinput) );
      if (strvect.size() < 5 || strvect[0] == comment) continue;
      bool found = false;
      RVarDef* pdef = vars;
      while( pdef->name && !found ) {
	if( strvect[0] == pdef->name ) { 
	  found = true; break;
	}
	pdef++;
      }
      Int_t slot(0), chan(0), skip(0);
      UInt_t header(0);
      Int_t crate = (Int_t)atoi(strvect[2].c_str());  // crate #
      bool is_slot = (strvect[1] == "crate");
      BdataLoc* b;
      if( is_slot ) {  // Crate data ?
	slot = (Int_t)atoi(strvect[3].c_str());
	chan = (Int_t)atoi(strvect[4].c_str());
	b = new BdataLoc(strvect[0].c_str(), crate, slot, chan);
      } else {         // Data is relative to a header
	header = header_str_to_base16(strvect[3].c_str());
	skip = (Int_t)atoi(strvect[4].c_str());
	b = new BdataLoc(strvect[0].c_str(), crate, header, skip);
      }

      bool already_defined = false;
      if( re_init ) {
	// When reinitializing, use simple logic, assuming things didn't 
	// change much:
	// - if the name exists, update it with the chan/slot read from the 
	//   database for the new time
	// - if the name is new, add it - leave it up to the user to decide
	//   whether this is sensible
	// - if a name disappears, things are ambiguous. Just leave it as 
	//   it is, although something is probably wrong with the database
	//
	Iter_t p;
	for( p = fWordLoc.begin();  p != fWordLoc.end(); p++ ) {
	  if( b->name == (*p)->name ) {
	    already_defined = true;
	    break;
	  }
	}
	if( !already_defined ) {
	  for( p = fCrateLoc.begin(); p != fCrateLoc.end(); p++ ) {
	    if( b->name == (*p)->name ) {
	      already_defined = true;
	      break;
	    }
	  }
	}

	if( already_defined ) {
	  if ( **p != *b ) {
	    if( fDebug>2 ) 
	      Info( Here(here), 
		    "Updating variable %s", (*p)->name.c_str() );
	    if( is_slot )
	      (*p)->SetSlot( crate, slot, chan );
	    else
	      (*p)->SetHeader( crate, header, skip );
	  } else {
	    if( fDebug>2 )
	      Info( Here(here),
		    "Variable %s already defined and not changed",
		    (*p)->name.c_str() );
	  }
	}
      }
      
      if( !already_defined ) {
	if( found && fDebug>2 ) 
	  Info( Here(here), "Defining standard variable %s", 
		b->name.c_str());
	else if( !found && fDebug>2 ) 
	  // !found is ok, but might be a typo error too, so I print to warn you.
	  Info( Here(here), 
		"New variable %s will become global", b->name.c_str());

	if( is_slot ) {
	  fCrateLoc.push_back(b);
	} else {
	  fWordLoc.push_back(b);
	}
	
	if (!found) {
	  // if not one of the pre-defined global variables, make a new global
	  // variable for this BdataLoc
	  DefineChannel(b,mode);
	}
      }
    }
  }

  return retval;
}

//_____________________________________________________________________________
BdataLoc* THaDecData::DefineChannel(BdataLoc *b, EMode mode, const char* desc)
{
  if( gHaVars ) {
    string nm(fPrefix + b->name);
    if (mode==kDefine)
      gHaVars->Define(nm.c_str(),desc,b->rdata[0],&(b->ndata));
    else if (mode==kDelete) {
      gHaVars->RemoveName(nm.c_str());
      b = NULL;
    }
  }
  return b;
}
  
//_____________________________________________________________________________
Int_t THaDecData::End( THaRunBase* run ) 
{
  WriteHist();
  return 0;
}

//_____________________________________________________________________________
void THaDecData::WriteHist()
{
  //  cout << "Writing Bob Dec Data histos"<<endl<<flush;
  for (vector<TH1F*>::iterator it = hist.begin(); it != hist.end(); it++)
    (*it)->Write();
}

//_____________________________________________________________________________
  void THaDecData::BookHist()
{
  // VDC efficiencies

  hist.push_back(new TH1F("Lu1nhit","Num Hits Left U1",50,-1,49));
  hist.push_back(new TH1F("Lu2nhit","Num Hits Left U2",50,-1,49));
  hist.push_back(new TH1F("Lv1nhit","Num Hits Left V1",50,-1,49));
  hist.push_back(new TH1F("Lv2nhit","Num Hits Left V2",50,-1,49));
  hist.push_back(new TH1F("Ru1nhit","Num Hits Right U1",50,-1,49));
  hist.push_back(new TH1F("Ru2nhit","Num Hits Right U2",50,-1,49));
  hist.push_back(new TH1F("Rv1nhit","Num Hits Right V1",50,-1,49));
  hist.push_back(new TH1F("Rv2nhit","Num Hits Right V2",50,-1,49));

  hist.push_back(new TH1F("Lu1eff","Left arm U1 efficiency",400,0,400));
  hist.push_back(new TH1F("Lu2eff","Left arm U2 efficiency",400,0,400));
  hist.push_back(new TH1F("Lv1eff","Left arm V1 efficiency",400,0,400));
  hist.push_back(new TH1F("Lv2eff","Left arm V2 efficiency",400,0,400));
  hist.push_back(new TH1F("Ru1eff","Right arm U1 efficiency",400,0,400));
  hist.push_back(new TH1F("Ru2eff","Right arm U2 efficiency",400,0,400));
  hist.push_back(new TH1F("Rv1eff","Right arm V1 efficiency",400,0,400));
  hist.push_back(new TH1F("Rv2eff","Right arm V2 efficiency",400,0,400));

  hist.push_back(new TH1F("Lenroc12","Event length in ROC12",500,0,5000));
  hist.push_back(new TH1F("Lenroc16","Event length in ROC16",500,0,5000));

}


//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no detectors, we
  // skip the detector initialization.

  fStatus = kNotinit;
  if( IsZombie() )
    return fStatus;
    
  MakePrefix();
  cnt1 = 0;
  // Let VdcEff reassociate its global variable pointers upon re-init
  if( fgVdcEffFirst == 0 )
    fgVdcEffFirst = 1;
  return fStatus = static_cast<EStatus>( SetupDecData( &run_time ) );
}

//_____________________________________________________________________________


Int_t THaDecData::DefaultMap() {
// Default setup of mapping of data in this class to locations in the raw data.
// This is valid for a particular time.  If you have 'decdata.map' in your
// pwd, the code would use that instead.  See /examples directory for an
// example of decdata.map

// ADCs that show data synch.
   Int_t crate = 1, slot = 25, chan = 16;   
   fCrateLoc.push_back(new BdataLoc("synchadc1", crate, slot, chan));
   fCrateLoc.push_back(new BdataLoc("synchadc2", 2, (Int_t) 24, 48));
   fCrateLoc.push_back(new BdataLoc("synchadc3", 3, (Int_t) 22, 0));
   fCrateLoc.push_back(new BdataLoc("synchadc4", 4, (Int_t) 17, 48));
   fCrateLoc.push_back(new BdataLoc("synchadc14", 14, (Int_t) 1, 5));

// Coincidence time, etc
   fCrateLoc.push_back(new BdataLoc("ctimel", 4, (Int_t) 21, 48));
   fCrateLoc.push_back(new BdataLoc("ctimer", 2, (Int_t) 16, 32));
   fCrateLoc.push_back(new BdataLoc("pulser1", 3, (Int_t) 3, 7));

// 100 kHz time stamp in roc14, at 2 words beyond header=0xfca56000
   fCrateLoc.push_back(new BdataLoc("timestamp", 14, (UInt_t)0xfca56000, 2)); 

// vxWorks time stamps
   UInt_t header = 0xfabc0004;
   Int_t ntoskip = 4;
   crate = 1;
   fWordLoc.push_back(new BdataLoc("timeroc1", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc("timeroc2", 2, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc3", 3, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc4", 4, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc14", 14, (UInt_t)0xfadcb0b4, 1));

// RF time
   fCrateLoc.push_back(new BdataLoc("rftime1", 2, (Int_t) 16, 50));
   fCrateLoc.push_back(new BdataLoc("rftime2", 2, (Int_t) 16, 51));

// EDTM pulser
   fCrateLoc.push_back(new BdataLoc("edtpl", 3, (Int_t) 9, 81));
   fCrateLoc.push_back(new BdataLoc("edtpr", 2, (Int_t) 12, 48));

// Bit pattern for trigger definition

   for (UInt_t i = 0; i < bits.GetNbits(); i++) {
     fCrateLoc.push_back(new BdataLoc(Form("bit%d",i+1), 3, (Int_t) 5, 64+i));
   }

// Anything else you want here...

   return 0;
}


//_____________________________________________________________________________
Int_t THaDecData::Decode(const THaEvData& evdata)
{
  // Extract the requested variables from the event data

  if( !IsOK() )
    return -1;

  Clear();

  lenroc12 = evdata.GetRocLength(12);
  lenroc16 = evdata.GetRocLength(16);

  hist[16]->Fill(lenroc12);
  hist[17]->Fill(lenroc16);

// For each raw data registerd in fCrateLoc, get the data if it belongs to a 
// combination (crate, slot, chan).

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;
    if ( dataloc->IsSlot() ) {  
      for (Int_t i = 0; i < evdata.GetNumHits(dataloc->crate, 
		         dataloc->slot, dataloc->chan); i++) {
	dataloc->Load(evdata.GetData(dataloc->crate, dataloc->slot, 
				     dataloc->chan, i));
      }
    }
  }
  
// Crawl through the event and decode elements of fWordLoc which are defined
// as relative to a header.   fWordLoc are treated seperately from fCrateLoc
// for performance reasons; this loop could be slow !

  for (Int_t i = 0; i < evdata.GetEvLength(); i++) {
    //FIXME: hash list lookup of header -> BdataLoc
    //FIXME: skip to next header?
    for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
      BdataLoc *dataloc = *p;
      if ( dataloc->DidLoad() || dataloc->IsSlot() ) continue;
      if ( evdata.InCrate(dataloc->crate, i) ) {
        if ((UInt_t)evdata.GetRawData(i) == dataloc->header) {
            dataloc->Load(evdata.GetRawData(i + dataloc->ntoskip));
        }
      }
    }
  }

  evtype = evdata.GetEvType();   // CODA event type 

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;
    bool found = false;

// bit pattern of triggers

  // FIXME: dozens of string comparisons for every event?!?
  // should store pointer to member data with the corresponding BdataLoc object
  // otherwise use a hash list
    for (UInt_t i = 0; i < bits.GetNbits(); i++) {
      if ( dataloc->ThisIs(Form("bit%d",i+1)) ) {
	TrigBits(i+1,dataloc);
	found = true;
	break;
      }
    }
    if( found ) continue;

// synch ADCs
    if ( dataloc->ThisIs("synchadc1") ) synchadc1  = dataloc->Get();
    else if ( dataloc->ThisIs("synchadc2") ) synchadc2  = dataloc->Get();
    else if ( dataloc->ThisIs("synchadc3") ) synchadc3  = dataloc->Get();
    else if ( dataloc->ThisIs("synchadc4") ) synchadc4  = dataloc->Get();
    else if ( dataloc->ThisIs("synchadc14")) synchadc14 = dataloc->Get();

// coincidence times
    else if ( dataloc->ThisIs("ctimel")) ctimel = dataloc->Get();
    else if ( dataloc->ThisIs("ctimer")) ctimer = dataloc->Get();

// RF time
    else if ( dataloc->ThisIs("rftime1") ) rftime1  = dataloc->Get();
    else if ( dataloc->ThisIs("rftime2") ) rftime2  = dataloc->Get();

// EDTM pulser
    else if ( dataloc->ThisIs("edtpl") ) edtpl  = dataloc->Get();
    else if ( dataloc->ThisIs("edtpr") ) edtpr  = dataloc->Get();

  }

  for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
    BdataLoc *dataloc = *p;

// time stamps
    if ( dataloc->ThisIs("timestamp")) timestamp = dataloc->Get();
    else if ( dataloc->ThisIs("timeroc1") ) timeroc1  = dataloc->Get();
    else if ( dataloc->ThisIs("timeroc2") ) timeroc2  = dataloc->Get();
    else if ( dataloc->ThisIs("timeroc3") ) timeroc3  = dataloc->Get();
    else if ( dataloc->ThisIs("timeroc4") ) timeroc4  = dataloc->Get();
    else if ( dataloc->ThisIs("timeroc14")) timeroc14 = dataloc->Get();

  }

// debug 
//    Print();

  VdcEff();

  return 0;
}


//_____________________________________________________________________________
void THaDecData::VdcEff( )
{ 
  // Update VDC efficiency histograms with current event data

  static const string VdcVars[] = {"L.vdc.u1.wire", "L.vdc.v1.wire", 
				   "L.vdc.u2.wire", "L.vdc.v2.wire", 
				   "R.vdc.u1.wire", "R.vdc.v1.wire", 
				   "R.vdc.u2.wire", "R.vdc.v2.wire"};
  
  const Int_t nwire = 400;
  //FIXME: really push 3.2kB on the stack every event?
  Int_t wire[nwire];
  Int_t hitwire[nwire];   // lookup to avoid O(N^3) algorithm

  
  //FIXME: these static variables prevent multiple instances of this object!
  // use member variables
  static Int_t cnt = 0;
  static Double_t xcnt[8*nwire],eff[8*nwire];
  static THaVar* varp[8];
  if (fgVdcEffFirst>0) {
    if( fgVdcEffFirst>1) {
      cnt = 0;
      memset(eff,0,8*nwire*sizeof(eff[0]));
      memset(xcnt,0,8*nwire*sizeof(xcnt[0]));
    }
    for( Int_t i = 0; i<8; i++ ) {
      varp[i] = gHaVars->Find(VdcVars[i].c_str());
    }
    fgVdcEffFirst = 0;
  }

#ifdef WITH_DEBUG
  if (fDebug>4) 
    cout << "\n *************** \n Vdc Effic "<<endl;
#endif

  for (Int_t ipl = 0; ipl < 8; ipl++) {

    Int_t nhit = 0;
    THaVar* pvar = varp[ipl];
#ifdef WITH_DEBUG
     if (fDebug>4)
      cout << "plane "<<ipl<<"  "<<VdcVars[ipl]<<" $$$ "<<pvar<<endl;
#endif
     if (!pvar) continue;
     memset(wire,0,nwire*sizeof(wire[0]));
     memset(hitwire,0,nwire*sizeof(hitwire[0]));

     Int_t n = pvar->GetLen();
     nhit = n;
     hist[ipl]->Fill(nhit);
     if (n < 0) n = 0;
     if (n > nwire) n = nwire;
#ifdef WITH_DEBUG
     if (fDebug>4)
       cout << "nwire "<<n<<"  "<<nwire<<"  "<<nhit<<endl;
#endif

     for (Int_t i = 0; i < n; i++) {
       wire[i] = (Int_t) pvar->GetValue(i);
       if (wire[i]>=0 && wire[i]<nwire)
	 hitwire[wire[i]]=1;
#ifdef WITH_DEBUG
       if (fDebug>4)
         cout << "wire "<<i<<"  "<<wire[i]<<endl;
#endif
     }

// The following does not assume that wire[] is ordered.
     for (Int_t i = 0; i < n; i++) {
       // look for neighboring hit at +2 wires
       Int_t ngh2=wire[i]+2;
       if (wire[i]<0 || ngh2>=nwire) continue;
       
       if (hitwire[ngh2]) {
	 Int_t awire = wire[i]+1;
#ifdef WITH_DEBUG
	 if (fDebug>4) 
	   cout << "wire eff "<<i<<"  "<<awire<<endl;
#endif
	 if (awire>=0 && awire<nwire) {
	   xcnt[ipl*nwire+awire] = xcnt[ipl*nwire+awire] + 1;
	   
	   if ( hitwire[awire] ) {
	     eff[ipl*nwire+awire] = eff[ipl*nwire+awire] + 1;
	   } else {
	     eff[ipl*nwire+awire] = eff[ipl*nwire+awire] + 0;
	   }
	 }
       }
     }
     
     if ((cnt%500) == 0) {

       hist[ipl+8]->Reset();
       for (Int_t i = 0; i < nwire; i++) {

         Double_t xeff = -1;
         if (xcnt[ipl*nwire+i] != 0) {
	   xeff = eff[ipl*nwire+i]/xcnt[ipl*nwire+i];
	 }
#ifdef WITH_DEBUG
         if (fDebug>4) 
	   cout << "Efficiency "<<i<<"  "<<xcnt[ipl*nwire+i]<<"  "<<xeff<<endl;
#endif
         if (xeff > 0) hist[ipl+8]->Fill(i,xeff);
       }
     }

  }
  cnt++;
  if ((cnt < 2000 && cnt % 500 == 0) ||
      (cnt % 5000 == 0)) WriteHist();

  //  if ((cnt%10)==0) Print();

}


//_____________________________________________________________________________
void THaDecData::Print( Option_t* opt ) const {
// Dump the data for purpose of debugging.
  cout << "Dump of THaDecData "<<endl;
  cout << "event pattern bits : ";
  for (UInt_t i = 0; i < bits.GetNbits(); i++) 
    cout << " "<<i<<" = "<< bits.TestBitNumber(i)<<"  | ";
  cout << endl;
  cout << "event types,  CODA = "<<evtype<<"   bit pattern = "<<evtypebits<<endl;
  cout << "synch adcs "<<"  "<<synchadc1<<"  "<<synchadc2<<"  ";
  cout << synchadc3 <<"  "<<synchadc4<<"   "<<synchadc14<<endl;
  cout <<" time stamps "<<timestamp<<"  "<<timeroc1<<"  "<<timeroc2<<"  ";
  cout << timeroc3<<"  "<<timeroc4<<"  "<<timeroc14<<endl<<endl;
  cout << "RF timing "<<rftime1<<"  "<<rftime2<<endl;
  cout << "EDTM pulser "<<edtpl<<"  "<<edtpr<<endl;
  cout << endl;
  cout << "Crate-type variables: " << fCrateLoc.size() << endl;
  cout << "Word-type variables: " << fWordLoc.size() << endl;
  cout << "Histograms: " << hist.size() << endl;
}


//_____________________________________________________________________________
vector<string> THaDecData::vsplit(const string& s) {
// split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type ssiz_t;
  ssiz_t i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      ssiz_t j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
}

//_____________________________________________________________________________
UInt_t THaDecData::header_str_to_base16(const char* hdr) {
// Utility to convert string header to base 16 integer
  const char chex[] = "0123456789abcdef";
  if( !hdr ) return 0;
  const char* p = hdr+strlen(hdr);
  UInt_t result = 0;  UInt_t power = 1;
  while( p-- != hdr ) {
    const char* q = strchr(chex,tolower(*p));
    if( q ) {
      result += (q-chex)*power; 
      power *= 16;
    }
  }
  return result;
};

//_____________________________________________________________________________
void THaDecData::TrigBits(UInt_t ibit, BdataLoc *dataloc) {
// Figure out which triggers got a hit.  These are multihit TDCs, so we
// need to sort out which hit we want to take by applying cuts.

  if( ibit >= kBitsPerByte*sizeof(UInt_t) ) return; //Limit of evtypebits
  bits.ResetBitNumber(ibit);

  const UInt_t cutlo = 200;
  const UInt_t cuthi = 1500;
  
  for (int ihit = 0; ihit < dataloc->NumHits(); ihit++) {
    if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
      bits.SetBitNumber(ibit);
      evtypebits |= BIT(ibit);
    }
  }

}
//_____________________________________________________________________________
ClassImp(THaDecData)

