//*-- Author :  R. Michaels, March 2002
//  with contributions from Rob Feurbach and Ameya Kolarkar, Feb 2006.
//
//////////////////////////////////////////////////////////////////////////
//
// GenDecData
//
// Hall A miscellaneous decoder data, which typically does not belong
// to a detector class.  Provides global data to analyzer, and a place
// to rapidly add new channels.
//
// Normally the user should have a file "decdata.map" or "db_D.dat" in
// their pwd to define the locations of raw data for this class
// (only).  But if this file is not found, we define a default
// mapping, which was valid at least at one point in history.
//
// The scheme is as follows:
//
//    1. In Init() we define a list of global variables which are tied
//       to the variables of this class.  E.g. "ftime24".
//
//    2. Next we build a list of "BdataLoc2" objects which store information
//       about where the data are located.  These data are either directly
//       related to the variables of this class (e.g. ftime24 is a raw
//       data word) or one must analyze them to obtain a variable.
//
//    3. The BdataLoc2 objects may be defined by decdata.map which has an
//       obvious notation (see ~/examples/decdata.map).  The entries are either 
//       locations in crates or locations relative to a unique header.
//       If decdata.map is not in the pwd where you run analyzer, then this
//       class uses its own internal DefaultMap().
//
//    4. The BdataLoc2 objects pertain to one data channel (e.g. a fastbus
//       channel) and and may be multihit.
//
//    5. To add a new variable, if it is on a single-hit channel, you may
//       imitate 'ctimebb' if you know the (crate,slot,chan), and 
//       imitate 'ftime24' if you know the (crate,header,no-to-skip).
//       If your variable is more complicated and relies on several 
//       channels, imitate the way 'bits' leads to 'evtypebits'.
//
// OR  (new as of April 2004:  R.J. Feuerbach)
//       If you are simply interested in the readout of a channel, create
//       a name for it and give the location in the map file and a
//       global variable will be automatically created to monitor that channel.
//     Unfortunately, this leads to a limitation of using arrays as opposed
//     to variable-sized vector for the readout. Currently limited to 16 hits
//     per channel per event.
//
// R. Michaels, March 2002
// 
//////////////////////////////////////////////////////////////////////////

//#define WITH_DEBUG 1

#include "GenDecData.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "TDatime.h"
#include "TH1.h"
#include "VarDef.h"
#include <fstream>
#include <iostream>

using namespace std;

typedef vector<BdataLoc2*>::iterator Iter_t;

//_____________________________________________________________________________
void BdataLoc2::DoLoad(const THaEvData& evdata) {
  if ( IsSlot() ) {
    for ( Int_t i = 0; i < evdata.GetNumHits(crate,slot,chan); i++ ) {
      Load(evdata.GetData(crate,slot,chan,i));
    }
  } else {
    UInt_t len = evdata.GetRocLength(crate);
    for ( UInt_t i = 0; i < len; i++ ) {
      if ( (UInt_t)evdata.GetRawData(crate,i)==header ) {
        Load(evdata.GetRawData(crate,i+ntoskip));
        break;
      }
    }
  }
}

//_____________________________________________________________________________
GenDecData::GenDecData( const char* name, const char* descript ) : 
  THaApparatus( name, descript )
{
  Clear();
}

//_____________________________________________________________________________
GenDecData::~GenDecData()
{

  // Dtor. Remove global variables.

  SetupRawData( NULL, kDelete ); 
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end();  p++ ) delete *p;
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++ ) delete *p;
}

//_____________________________________________________________________________
void GenDecData::Clear( Option_t* opt ) 
{
  evtypebits = 0;
  evtype     = 0;
  evnum      = 0;
  timestamp  = 0;
  ctimebb    = 0;
  ctimec     = 0;
  ctimeND    = 0;
  vtime20 = 0; vtime21 = 0; vtime22 = 0; vtime23 = 0; vtime24 = 0; vtime25 = 0;
  ftime20 = 0; ftime21 = 0; ftime22 = 0; ftime23 = 0; ftime24 = 0; ftime25 = 0;
  evnum20 = 0; evnum21 = 0; evnum22 = 0; evnum23 = 0; evnum24 = 0; evnum25 = 0;
  evtype20 = 0; evtype21 = 0; evtype22 = 0; evtype23 = 0; evtype24 = 0; evtype25 = 0;
  dscan20 = 0; dscan21 = 0; dscan22 = 0; dscan23 = 0; dscan24 = 0; dscan25 = 0;
  rftime1    = 0;
  rftime2    = 0;
  misc1      = 0;
  misc2      = 0;
  misc3      = 0;
  misc4      = 0;
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++)  (*p)->Clear();
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) (*p)->Clear();
}

//_____________________________________________________________________________
Int_t GenDecData::SetupRawData( const TDatime* run_time, EMode mode )
{
  // Register global variables, open decdata map file, and parse it.
  // If mode == kDelete, remove global variables.

  Int_t retval = 0;


  RVarDef vars[] = {
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { "evtype",     "event type from bit pattern", "evtype" },  
    { "ctimebb",     "coincidence time on BB",     "ctimebb" },  
    { "ctimec",     "coarse coinc time",         "ctimec" },
    { "ctimeND",     "coarse coinc time",         "ctimeND" },
    { "times105k",  "105kHz time stamp",           "timestamp" },       
    { "ctr20",   " Counter on roc 20",         "ctr20" },    
    { "ctr21",   " Counter on roc 21",         "ctr21" },    
    { "ctr22",   " Counter on roc 22",         "ctr22" },    
    { "ctr23",   " Counter on roc 23",         "ctr23" },    
    { "ctr24",   " Counter on roc 24",         "ctr24" },    
    { "ctr25",   " Counter on roc 25",         "ctr25" },    
    { "ctr28",   " Counter on roc 28",         "ctr28" },    
    { "vtime20",   " vxWorks time roc 20",         "vtime20" },    
    { "vtime21",   " vxWorks time roc 21",         "vtime21" },    
    { "vtime22",   " vxWorks time roc 22",         "vtime22" },    
    { "vtime23",   " vxWorks time roc 23",         "vtime23" },    
    { "vtime24",   " vxWorks time roc 24",         "vtime24" },    
    { "vtime25",   " vxWorks time roc 25",         "vtime25" },    
    { "vtime28",   " vxWorks time roc 28",         "vtime28" },    
    { "ftime20",   " 105kHz clock time roc 20",         "ftime20" },    
    { "ftime21",   " 105kHz clock time roc 21",         "ftime21" },    
    { "ftime22",   " 105kHz clock time roc 22",         "ftime22" },    
    { "ftime23",   " 105kHz clock time roc 23",         "ftime23" },    
    { "ftime24",   " 105kHz clock time roc 24",         "ftime24" },    
    { "ftime25",   " 105kHz clock time roc 25",         "ftime25" },    
    { "ftime28",   " 105kHz clock time roc 28",         "ftime28" },    
    { "evnum20",   " Event num on roc 20",         "evnum20" },    
    { "evnum21",   " Event num on roc 21",         "evnum21" },    
    { "evnum22",   " Event num on roc 22",         "evnum22" },    
    { "evnum23",   " Event num on roc 23",         "evnum23" },    
    { "evnum24",   " Event num on roc 24",         "evnum24" },    
    { "evnum25",   " Event num on roc 25",         "evnum25" },    
    { "evnum28",   " Event num on roc 28",         "evnum28" },    
    { "evtype20",   " Event type on roc 20",         "evtype20" },    
    { "evtype21",   " Event type on roc 21",         "evtype21" },    
    { "evtype22",   " Event type on roc 22",         "evtype22" },    
    { "evtype23",   " Event type on roc 23",         "evtype23" },    
    { "evtype24",   " Event type on roc 24",         "evtype24" },    
    { "evtype25",   " Event type on roc 25",         "evtype25" },    
    { "evtype28",   " Event type on roc 28",         "evtype28" },    
    { "dscan20",   " Datascan on roc 20",         "dscan20" },    
    { "dscan21",   " Datascan on roc 21",         "dscan21" },    
    { "dscan22",   " Datascan on roc 22",         "dscan22" },    
    { "dscan23",   " Datascan on roc 23",         "dscan23" },    
    { "dscan24",   " Datascan on roc 24",         "dscan24" },    
    { "dscan25",   " Datascan on roc 25",         "dscan25" },    
    { "dscan28",   " Datascan on roc 28",         "dscan28" },    
    { "rftime1",    "RF time copy 1",              "rftime1" },         
    { "rftime2",    "RF time copy 2",              "rftime2" },         
    { "misc1",      "misc data 1",                 "misc1" },       
    { "misc2",      "misc data 2",                 "misc2" },                     
    { "misc3",      "misc data 3",                 "misc3" },                      
    { "misc4",      "misc data 4",                 "misc4" },
    { 0 }
  };
  Int_t nvar = sizeof(vars)/sizeof(RVarDef);

  // For the dynamically built list, make sure all is deleted BEFORE
  // building the list anew.
  //   Since entries in fWordLoc are in the list above, the order is somewhat impt
  if (gHaVars) {
    for (unsigned int i=0; i<fCrateLoc.size(); i++) {
      string nm = GetPrefix();
      nm += fCrateLoc[i]->name;
      gHaVars->RemoveName(nm.c_str());
    }
    for (unsigned int i=0; i<fWordLoc.size(); i++) {
      string nm = GetPrefix();
      nm += fWordLoc[i]->name;
      gHaVars->RemoveName(nm.c_str());
    }
  }
  DefineVarsFromList( vars, kDelete ); fIsSetup=0; // clean out everything
  
  if( mode != kDefine || !fIsSetup )
    retval = DefineVarsFromList( vars, mode );

  fIsSetup = ( mode == kDefine );
  
  if ( mode != kDefine ) 
    return retval;
  
// Set up the locations in raw data corresponding to variables of this class. 
// Each element of a BdataLoc2 is one channel or word.  Since a channel 
// may be a multihit device, the BdataLoc2 stores data in a vector.

  fCrateLoc.clear();   
  fWordLoc.clear();   

  ifstream decdatafile;

  const char* const here = "SetupRawData()";
  const char* name = GetDBFileName();

  TDatime date;
  if( run_time ) date = *run_time;
  vector<string> fnames = GetDBFileList( name, date, Here(here));
  // always look for 'decdata.map' in the current directory first.
  fnames.insert(fnames.begin(),string("decdata.map"));
  if( !fnames.empty() ) {
    vector<string>::iterator it = fnames.begin();
    do {
#ifdef WITH_DEBUG
      if( fDebug>0 ) {
        cout << "<" << Here(here) << ">: Opening database file " << *it;
      }
#endif
      decdatafile.clear();  // Forget previous failures before attempt
      decdatafile.open((*it).c_str());

#ifdef WITH_DEBUG
      if (decdatafile) {
        cout << "Opening file "<<*it<<endl;
      } else {
        cout << "cannot open file "<<*it<<endl;
      }
      if( fDebug>0 ) 
        if( !decdatafile ) cout << " ... failed" << endl;
        else               cout << " ... ok" << endl;
#endif
    } while ( !decdatafile && ++it != fnames.end() );
  }
  if( fnames.empty() || !decdatafile )
    if (THADEC_VERBOSE) {
      cout << "WARNING:: GenDecData: File db_"<<name<<".dat not found."<<endl;
      cout << "An example of this file should be in the examples directory."<<endl;
      cout << "Will proceed with default mapping for GenDecData."<<endl;
      return DefaultMap();
    }

  string sinput;
  const string comment = "#";
  while (getline(decdatafile, sinput)) {
#ifdef WITH_DEBUG
    cout << "sinput "<<sinput<<endl;
#endif
    vector<string> strvect( vsplit(sinput) );
    if (strvect.size() < 5 || strvect[0] == comment) continue;
    Bool_t found = kFALSE;
    for (int i = 0; i < nvar; i++) {
      if (vars[i].name && strvect[0] == vars[i].name) found = kTRUE;
    }
// !found may be ok, but might be a typo error too, so I print to warn you.
//    if ( !found && THADEC_VERBOSE ) 
//      cout << "GenDecData: new variable "<<strvect[0]<<" will be made global"<<endl;
    Int_t crate = (Int_t)atoi(strvect[2].c_str());  // crate #
    Int_t slot = (Int_t)atoi(strvect[3].c_str());
    Int_t chan = (Int_t)atoi(strvect[4].c_str());
    UInt_t header = header_str_to_base16(strvect[3].c_str());
    Int_t skip = (Int_t)atoi(strvect[4].c_str());
    BdataLoc2 *b=0;
    if (strvect[1] == "crate") {  // Crate data ?
      b = new BdataLoc2(strvect[0].c_str(), crate, slot, chan); 
      fCrateLoc.push_back(b);
    } else {         // Data is relative to a header
      b = new BdataLoc2(strvect[0].c_str(), crate, header, skip);
      fWordLoc.push_back(b);
    }
    
    if (!found) {
      // a new variable to add to our dynamic list
      string nm(fPrefix + strvect[0]);
      if (gHaVars) gHaVars->Define(nm.c_str(),"automatic",b->rdata[0],&(b->ndata));
    }
  }
  return retval;
}

//_____________________________________________________________________________
Int_t GenDecData::End( THaRunBase* run ) 
{
  return 0;
}


//_____________________________________________________________________________
THaAnalysisObject::EStatus GenDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no detectors, we
  // skip the detector initialization.

  fStatus = kNotinit;
  MakePrefix();
  return fStatus = static_cast<EStatus>( SetupRawData( &run_time ) );
}

//_____________________________________________________________________________


Int_t GenDecData::DefaultMap() {
// Default setup of mapping of data in this class to locations in the raw data.
// This is valid for a particular time.  If you have 'decdata.map' in your
// pwd, the code would use that instead.  See /examples directory for an
// example of decdata.map

// Coincidence time
   fCrateLoc.push_back(new BdataLoc2("ctimebb", 22, (Int_t) 21, 48));
   fCrateLoc.push_back(new BdataLoc2("ctimec", 22, (Int_t) 16, 32));
   fCrateLoc.push_back(new BdataLoc2("ctimeND", 24, (Int_t) 24, 32));
   fCrateLoc.push_back(new BdataLoc2("pulser1", 3, (Int_t) 3, 7));

// 105 kHz time stamp in TS1 crate (ROC28)
   fCrateLoc.push_back(new BdataLoc2("timestamp", 28, (UInt_t)0xfabc007, 6)); 

// vxWorks time stamps
   UInt_t header = 0xfabc0007;
   Int_t ntoskip = 5;
   Int_t crate = 20;
   fWordLoc.push_back(new BdataLoc2("vtime20", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc2("vtime21", 21, (UInt_t)0xfabc0007, 5));
   fWordLoc.push_back(new BdataLoc2("vtime22", 22, (UInt_t)0xfabc0008, 7));
   fWordLoc.push_back(new BdataLoc2("vtime23", 23, (UInt_t)0xfabc0007, 5));
   fWordLoc.push_back(new BdataLoc2("vtime24", 24, (UInt_t)0xfabc0008, 7));
   fWordLoc.push_back(new BdataLoc2("vtime25", 25, (UInt_t)0xfabc0007, 5));
   fWordLoc.push_back(new BdataLoc2("vtime28", 28, (UInt_t)0xfabc0007, 5));

// fast clock (105 kHz)
   header = 0xfabc0007;
   ntoskip = 6;
   crate = 20;
   fWordLoc.push_back(new BdataLoc2("ftime20", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc2("ftime21", 21, (UInt_t)0xfabc0007, 6));
   fWordLoc.push_back(new BdataLoc2("ftime22", 22, (UInt_t)0xfabc0008, 8));
   fWordLoc.push_back(new BdataLoc2("ftime23", 23, (UInt_t)0xfabc0007, 6));
   fWordLoc.push_back(new BdataLoc2("ftime24", 24, (UInt_t)0xfabc0008, 8));
   fWordLoc.push_back(new BdataLoc2("ftime25", 25, (UInt_t)0xfabc0007, 6));
   fWordLoc.push_back(new BdataLoc2("ftime28", 28, (UInt_t)0xfabc0007, 6));

// Event numbers
   header = 0xfabc0007;
   ntoskip = 2;
   crate = 20;
   fWordLoc.push_back(new BdataLoc2("evnum20", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc2("evnum21", 21, (UInt_t)0xfabc0007, 2));
   fWordLoc.push_back(new BdataLoc2("evnum22", 22, (UInt_t)0xfabc0008, 4));
   fWordLoc.push_back(new BdataLoc2("evnum23", 23, (UInt_t)0xfabc0007, 2));
   fWordLoc.push_back(new BdataLoc2("evnum24", 24, (UInt_t)0xfabc0008, 4));
   fWordLoc.push_back(new BdataLoc2("evnum25", 25, (UInt_t)0xfabc0007, 2));
   fWordLoc.push_back(new BdataLoc2("evnum28", 28, (UInt_t)0xfabc0007, 2));

// Event type
   header = 0xfabc0007;
   ntoskip = 3;
   crate = 20;
   fWordLoc.push_back(new BdataLoc2("evtype20", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc2("evtype21", 21, (UInt_t)0xfabc0007, 3));
   fWordLoc.push_back(new BdataLoc2("evtype22", 22, (UInt_t)0xfabc0008, 5));
   fWordLoc.push_back(new BdataLoc2("evtype23", 23, (UInt_t)0xfabc0007, 3));
   fWordLoc.push_back(new BdataLoc2("evtype24", 24, (UInt_t)0xfabc0008, 5));
   fWordLoc.push_back(new BdataLoc2("evtype25", 25, (UInt_t)0xfabc0007, 3));
   fWordLoc.push_back(new BdataLoc2("evtype28", 28, (UInt_t)0xfabc0007, 3));

// Data scan (fastbus only)
   fWordLoc.push_back(new BdataLoc2("dscan22", 22, (UInt_t)0xfabc0008, 2));
   fWordLoc.push_back(new BdataLoc2("dscan24", 24, (UInt_t)0xfabc0008, 2));


// RF time
   fCrateLoc.push_back(new BdataLoc2("rftime1", 23, (Int_t) 4, 16));
   fCrateLoc.push_back(new BdataLoc2("rftime2", 23, (Int_t) 4, 17));

// Bit pattern for trigger definition
   for (UInt_t i = 0; i < bits.GetNbits(); i++) {
     fCrateLoc.push_back(new BdataLoc2(Form("bit%d",i+1), 22, (Int_t) 20, i));
   }

// Anything else you want here...

   return 0;
}


//_____________________________________________________________________________
Int_t GenDecData::Decode(const THaEvData& evdata)
{
  Int_t i;
  Clear();

  static int jtst = 0;
  jtst++;  
  if (jtst > 400) jtst = 0;

// For each raw data registerd in fCrateLoc, get the data if it belongs to a 
// combination (crate, slot, chan).

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc2 *dataloc = *p;
    if ( dataloc->IsSlot() ) {  
      for (i = 0; i < evdata.GetNumHits(dataloc->crate, 
                         dataloc->slot, dataloc->chan); i++) {
        dataloc->Load(evdata.GetData(dataloc->crate, dataloc->slot, 
                                     dataloc->chan, i));
      }
    }
  }
  
// Crawl through the event and decode elements of fWordLoc which are defined
// as relative to a header.   fWordLoc are treated seperately from fCrateLoc
// for performance reasons; this loop could be slow !

  for (i = 0; i < evdata.GetEvLength(); i++) {
    for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
      BdataLoc2 *dataloc = *p;
      if ( dataloc->DidLoad() || dataloc->IsSlot() ) continue;
      if ( evdata.InCrate(dataloc->crate, i) ) {
        if ((UInt_t)evdata.GetRawData(i) == dataloc->header) {
            dataloc->Load(evdata.GetRawData(i + dataloc->ntoskip));
        }
      }
    }
  }

  evtype = evdata.GetEvType();   // CODA event type 
  evnum  = evdata.GetEvNum();

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc2 *dataloc = *p;

// bit pattern of triggers
    for (UInt_t i = 0; i < bits.GetNbits(); i++) {
      if ( dataloc->ThisIs(Form("bit%d",i+1)) ) TrigBits(i+1,dataloc);
    }

// coincidence times
    if ( dataloc->ThisIs("ctimebb")) ctimebb = dataloc->Get();
    if ( dataloc->ThisIs("ctimec")) ctimec = dataloc->Get();
    if ( dataloc->ThisIs("ctimeND")) ctimeND = dataloc->Get();

// RF time
    if ( dataloc->ThisIs("rftime1") ) rftime1  = dataloc->Get();
    if ( dataloc->ThisIs("rftime2") ) rftime2  = dataloc->Get();

  }

  for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
    BdataLoc2 *dataloc = *p;

// Synch data

    if ( dataloc->ThisIs("vtime20")) vtime20 = dataloc->Get();
    if ( dataloc->ThisIs("vtime21")) vtime21 = dataloc->Get();
    if ( dataloc->ThisIs("vtime22")) vtime22 = dataloc->Get();
    if ( dataloc->ThisIs("vtime23")) vtime23 = dataloc->Get();
    if ( dataloc->ThisIs("vtime24")) vtime24 = dataloc->Get();
    if ( dataloc->ThisIs("vtime25")) vtime25 = dataloc->Get();
    if ( dataloc->ThisIs("vtime28")) vtime28 = dataloc->Get();

    if ( dataloc->ThisIs("ftime20")) ftime20 = dataloc->Get();
    if ( dataloc->ThisIs("ftime21")) ftime21 = dataloc->Get();
    if ( dataloc->ThisIs("ftime22")) ftime22 = dataloc->Get();
    if ( dataloc->ThisIs("ftime23")) ftime23 = dataloc->Get();
    if ( dataloc->ThisIs("ftime24")) ftime24 = dataloc->Get();
    if ( dataloc->ThisIs("ftime25")) ftime25 = dataloc->Get();
    if ( dataloc->ThisIs("ftime28")) ftime28 = dataloc->Get();

    if ( dataloc->ThisIs("evnum20")) evnum20 = dataloc->Get();
    if ( dataloc->ThisIs("evnum21")) evnum21 = dataloc->Get();
    if ( dataloc->ThisIs("evnum22")) evnum22 = dataloc->Get();
    if ( dataloc->ThisIs("evnum23")) evnum23 = dataloc->Get();
    if ( dataloc->ThisIs("evnum24")) evnum24 = dataloc->Get();
    if ( dataloc->ThisIs("evnum25")) evnum25 = dataloc->Get();
    if ( dataloc->ThisIs("evnum28")) evnum28 = dataloc->Get();

    if ( dataloc->ThisIs("evtype20")) evtype20 = dataloc->Get();
    if ( dataloc->ThisIs("evtype21")) evtype21 = dataloc->Get();
    if ( dataloc->ThisIs("evtype22")) evtype22 = dataloc->Get();
    if ( dataloc->ThisIs("evtype23")) evtype23 = dataloc->Get();
    if ( dataloc->ThisIs("evtype24")) evtype24 = dataloc->Get();
    if ( dataloc->ThisIs("evtype25")) evtype25 = dataloc->Get();
    if ( dataloc->ThisIs("evtype28")) evtype28 = dataloc->Get();

    if ( dataloc->ThisIs("dscan20")) dscan20 = dataloc->Get();
    if ( dataloc->ThisIs("dscan21")) dscan21 = dataloc->Get();
    if ( dataloc->ThisIs("dscan22")) dscan22 = dataloc->Get();
    if ( dataloc->ThisIs("dscan23")) dscan23 = dataloc->Get();
    if ( dataloc->ThisIs("dscan24")) dscan24 = dataloc->Get();
    if ( dataloc->ThisIs("dscan25")) dscan25 = dataloc->Get();
    if ( dataloc->ThisIs("dscan28")) dscan28 = dataloc->Get();


  }

// debug 
//    Print();

  return 0;
}


//_____________________________________________________________________________
void GenDecData::Print( Option_t* opt ) const {
// Dump the data for purpose of debugging.
  cout << "----------------- ";
  cout << " Dump of GenDecData ";
  cout << "----------------- " << endl;
  cout << "event number "<<evnum<<endl;
  cout << "event types,  CODA = "<<evtype<<"   bit pattern = "<<evtypebits<<endl;
  cout << "event pattern bits : ";
  for (UInt_t i = 0; i < bits.GetNbits(); i++) 
    cout << " "<<i<<" = "<< bits.TestBitNumber(i)<<"  | ";
  cout << endl;
  cout << "Coinc. time "<<ctimebb<<"   "<<ctimec<<"  "<<ctimeND<<endl;
  cout << "RF timing "<<rftime1<<"  "<<rftime2<<endl;
  cout << "Synch data "<<endl;
  cout << "vxWorks time (hex)"<<hex<<vtime20<<"  "<<vtime21<<"  "<<vtime22<<"  "<<vtime23<<"  "<<vtime24<<"  "<<vtime25<<"  "<<vtime28<<endl;
  cout << "fast clock time (hex)"<<hex<<ftime20<<"  "<<ftime21<<"  "<<ftime22<<"  "<<ftime23<<"  "<<ftime24<<"  "<<ftime25<<"  "<<ftime28<<endl;
  cout << "evnum "<<dec<<evnum20<<"  "<<evnum21<<"  "<<evnum22<<"  "<<evnum23<<"  "<<evnum24<<"  "<<evnum25<<"  "<<evnum28<<endl;
  cout << "evtype "<<evtype20<<"  "<<evtype21<<"  "<<evtype22<<"  "<<evtype23<<"  "<<evtype24<<"  "<<evtype25<<"  "<<evtype28<<endl;
  cout << "fastbus data scan (hex) "<<hex<<dscan22<<"  "<<dscan24<<endl<<dec;

}


//_____________________________________________________________________________
vector<string> GenDecData::vsplit(const string& s) {
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
UInt_t GenDecData::header_str_to_base16(const char* hdr) {
// Utility to convert string header to base 16 integer
  static const char chex[] = "0123456789abcdef";
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
void GenDecData::TrigBits(UInt_t ibit, BdataLoc2 *dataloc) {
// Figure out which triggers got a hit.  These are multihit TDCs, so we
// need to sort out which hit we want to take by applying cuts.

  if( ibit >= kBitsPerByte*sizeof(UInt_t) ) return; //Limit of evtypebits
  bits.ResetBitNumber(ibit);

  int locdebug=0;

  static const UInt_t cutlo = 000;
  static const UInt_t cuthi = 3000;
  
  if (locdebug) cout << "TrigBits for bit "<<ibit<<"  Num hits "<<dataloc->NumHits()<<endl;

  for (int ihit = 0; ihit < dataloc->NumHits(); ihit++) {
    if (locdebug) cout << "hit "<<ihit<<"   data = "<<dataloc->Get(ihit)<<endl;    
    if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
      bits.SetBitNumber(ibit);
      evtypebits |= BIT(ibit);
    }
  }

  if (locdebug) cout << "evtypebits "<<evtypebits << "   evtype "<<evtype<<endl;

}
//_____________________________________________________________________________
ClassImp(GenDecData)

