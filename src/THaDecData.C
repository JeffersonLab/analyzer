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
//////////////////////////////////////////////////////////////////////////

#include "THaDecData.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "TDatime.h"
#include "VarDef.h"
#include <fstream>
#include <iostream>

using namespace std;

typedef vector<BdataLoc*>::iterator Iter_t;

//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript ) : 
  THaApparatus( name, descript )
{
  Clear();
}

//_____________________________________________________________________________
THaDecData::~THaDecData()
{
  // Dtor. Remove global variables.

  SetupDecData( NULL, kDelete ); 
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++ )  delete *p;
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++ ) delete *p;
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* opt ) 
{
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
  misc1      = 0;
  misc2      = 0;
  misc3      = 0;
  misc4      = 0;
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++)  (*p)->Clear();
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) (*p)->Clear();
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
    { "misc1",      "misc data 1",                 "misc1" },                      
    { "misc2",      "misc data 2",                 "misc2" },                     
    { "misc3",      "misc data 3",                 "misc3" },                      
    { "misc4",      "misc data 4",                 "misc4" },
    { 0 }
  };
  Int_t nvar = sizeof(vars)/sizeof(VarDef);

  if( mode != kDefine || !fIsSetup )
    retval = DefineVarsFromList( vars, mode );

  fIsSetup = ( mode == kDefine );
  if( mode != kDefine )
    return retval;

// Set up the locations in raw data corresponding to variables of this class. 
// Each element of a BdataLoc is one channel or word.  Since a channel 
// may be a multihit device, the BdataLoc stores data in a vector.

  fCrateLoc.clear();   
  fWordLoc.clear();   

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
#ifdef WITH_DEBUG
      if( fDebug>0 ) {
	cout << "<" << Here(here) << ">: Opening database file " << *it;
      }
#endif
      decdatafile.clear();  // Forget previous failures before attempt
      decdatafile.open((*it).c_str());
      
#ifdef WITH_DEBUG
      if( fDebug>0 ) 
	if( !decdatafile ) cout << " ... failed" << endl;
	else               cout << " ... ok" << endl;
#endif
    } while ( !decdatafile && ++it != fnames.end() );
  }
  if( fnames.empty() || !decdatafile )
    if (THADEC_VERBOSE) {
      cout << "WARNING:: THaDecData: File db_"<<name<<".dat not found."<<endl;
      cout << "An example of this file should be in the examples directory."<<endl;
      cout << "Will proceed with default mapping for THaDecData."<<endl;
      return DefaultMap();
    }

  string sinput;
  const string comment = "#";
  while (getline(decdatafile, sinput)) {
    vector<string> strvect( vsplit(sinput) );
    if (strvect.size() < 5 || strvect[0] == comment) continue;
    Bool_t found = kFALSE;
    for (int i = 0; i < nvar; i++) {
      if (strvect[0] == vars[i].name) found = kTRUE;
    }
// !found may be ok, but might be a typo error too, so I print to warn you.
//  if ( !found && THADEC_VERBOSE ) 
//  cout << "THaDecData: new variable "<<strvect[0]<<" is not global"<<endl;
    Int_t crate = (Int_t)atoi(strvect[2].c_str());  // crate #
    if (strvect[1] == "crate") {  // Crate data ?
      Int_t slot = (Int_t)atoi(strvect[3].c_str());
      Int_t chan = (Int_t)atoi(strvect[4].c_str());
      fCrateLoc.push_back(new BdataLoc(strvect[0].c_str(), crate, slot, chan)); 
    } else {         // Data is relative to a header
      UInt_t header = header_str_to_base16(strvect[3].c_str());
      Int_t skip = (Int_t)atoi(strvect[4].c_str());
      fWordLoc.push_back(new BdataLoc(strvect[0].c_str(), crate, header, skip));
    }
  }
  return retval;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no detectors, we
  // skip the detector initialization.

  fStatus = kNotinit;
  MakePrefix();
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
   fCrateLoc.push_back(new BdataLoc("ctimel", 2, (Int_t) 3, 48));
   fCrateLoc.push_back(new BdataLoc("ctimer", 1, (Int_t) 21, 4));
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
   
// Bit pattern for trigger definition

   for (UInt_t i = 0; i < bits.GetNbits(); i++) {
     fCrateLoc.push_back(new BdataLoc(Form("bit%d",i+1), 4, (Int_t) 11, 64+i));
   }

// Anything else you want here...

   return 0;
}


//_____________________________________________________________________________
Int_t THaDecData::Decode(const THaEvData& evdata)
{
  Int_t i;
  Clear();

// For each raw data registerd in fCrateLoc, get the data if it belongs to a 
// combination (crate, slot, chan).

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;
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

// bit pattern of triggers
    for (UInt_t i = 0; i < bits.GetNbits(); i++) {
      if ( dataloc->ThisIs(Form("bit%d",i+1)) ) TrigBits(i+1,dataloc);
    }

// synch ADCs
    if ( dataloc->ThisIs("synchadc1") ) synchadc1  = dataloc->Get();
    if ( dataloc->ThisIs("synchadc2") ) synchadc2  = dataloc->Get();
    if ( dataloc->ThisIs("synchadc3") ) synchadc3  = dataloc->Get();
    if ( dataloc->ThisIs("synchadc4") ) synchadc4  = dataloc->Get();
    if ( dataloc->ThisIs("synchadc14")) synchadc14 = dataloc->Get();

// coincidence times
    if ( dataloc->ThisIs("ctimel")) ctimel = 0.1*dataloc->Get();
    if ( dataloc->ThisIs("ctimer")) ctimer = 0.1*dataloc->Get();

  }

  for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
    BdataLoc *dataloc = *p;

// time stamps
    if ( dataloc->ThisIs("timestamp")) timestamp = dataloc->Get();
    if ( dataloc->ThisIs("timeroc1") ) timeroc1  = dataloc->Get();
    if ( dataloc->ThisIs("timeroc2") ) timeroc2  = dataloc->Get();
    if ( dataloc->ThisIs("timeroc3") ) timeroc3  = dataloc->Get();
    if ( dataloc->ThisIs("timeroc4") ) timeroc4  = dataloc->Get();
    if ( dataloc->ThisIs("timeroc14")) timeroc14 = dataloc->Get();

  }

// debug 
//  Dump();

  return 0;
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
void THaDecData::TrigBits(UInt_t ibit, BdataLoc *dataloc) {
// Figure out which triggers got a hit.  These are multihit TDCs, so we
// need to sort out which hit we want to take by applying cuts.

  if( ibit >= kBitsPerByte*sizeof(UInt_t) ) return; //Limit of evtypebits
  bits.ResetBitNumber(ibit);

  static const UInt_t cutlo = 400;
  static const UInt_t cuthi = 1200;
  
  for (int ihit = 0; ihit < dataloc->NumHits(); ihit++) {
    if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
      bits.SetBitNumber(ibit);
      evtypebits |= BIT(ibit);
    }
  }

}
//_____________________________________________________________________________
ClassImp(THaDecData)

