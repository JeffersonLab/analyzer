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
//#include "THaVar.h"
#include "VarDef.h"
#include <stdio.h>

extern FILE *fp;

const char* const myName = "D";

ClassImp(THaDecData)

//_____________________________________________________________________________
THaDecData::THaDecData( const char* descript ) : 
  THaApparatus( myName, descript )
{
  fNmydets    = 0;
  bits        = new Int_t[MAXBIT];
  cbit        = new char[50];
  Clear();
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* opt ) 
{
  memset(bits, 0, MAXBIT*sizeof(UInt_t));
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
  for (vector<BdataLoc *>::iterator p = fWordLoc.begin(); 
     p != fWordLoc.end(); p++) {
          (*p)->Clear();
  }
  for (vector<BdataLoc *>::iterator p = fCrateLoc.begin(); 
     p != fCrateLoc.end(); p++) {
          (*p)->Clear();
  }
}

//_____________________________________________________________________________
Int_t THaDecData::SetupDecData( const TDatime* run_time, EMode mode )
{
  // Register global variables.

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
  Int_t retval = DefineVarsFromList( vars, mode );

  if( mode != kDefine )
    return retval;

  nvar = sizeof(vars)/sizeof(VarDef);

// Set up the locations in raw data corresponding to variables of this class. 
// Each element of a BdataLoc is one channel or word.  Since a channel 
// may be a multihit device, the BdataLoc stores data in a vector.

  fCrateLoc.clear();   
  fWordLoc.clear();   

  ifstream decdatafile ("decdata.map");
  if ( !decdatafile ) {
    if (THADEC_VERBOSE) {
      cout << "WARNING:: THaDecData: File decdata.map not found."<<endl;
      cout << "An example of this file should be in ./examples directory."<<endl;
      cout << "Will proceed with default mapping for THaDecData."<<endl;
      return DefaultMap();
    }
  }

  string sinput;
  vector<string> strvect;
  string comment = "#";
  while (getline(decdatafile, sinput)) {
    strvect.clear();
    strvect = vsplit(sinput);
    if (strvect[0] != comment && strvect.size() > 4) {
      Bool_t found = kFALSE;
      for (int i = 0; i < nvar; i++) {
	if (strcmp(vars[i].name, strvect[0].c_str()) == 0) found = kTRUE;
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
	UInt_t header = header_str_to_base16(strvect[3]);
	Int_t skip = (Int_t)atoi(strvect[4].c_str());
	fWordLoc.push_back(new BdataLoc(strvect[0].c_str(), crate, header, skip));
      }
    }
  }
  return retval;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no detectors, we
  // skip the detector initialization.

  return fStatus = static_cast<EStatus>( SetupDecData( &run_time ) );
}

//_____________________________________________________________________________


Int_t THaDecData::DefaultMap() {
// Default setup of mapping of data in this class to locations in the raw data.
// This is valid for a particular time.  If you have 'decdata.map' in your
// pwd, the code would use that instead.  See /examples directory for an
// example of decdata.map

// ADCs that show data synch.
   Int_t crate, slot, chan;   
   crate = 1;  slot = 25;  chan = 16;
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
   UInt_t header;
   Int_t ntoskip;
   crate = 1;  header = 0xfabc0004;  ntoskip = 4;
   fWordLoc.push_back(new BdataLoc("timeroc1", crate, header, ntoskip));
   fWordLoc.push_back(new BdataLoc("timeroc2", 2, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc3", 3, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc4", 4, (UInt_t)0xfabc0004, 4));
   fWordLoc.push_back(new BdataLoc("timeroc14", 14, (UInt_t)0xfadcb0b4, 1));
   
// Bit pattern for trigger definition

   for (Int_t i = 0; i < MAXBIT; i++) {
      Int_t chan = 64 + i;
      sprintf(cbit,"bit%d",i+1);
      fCrateLoc.push_back(new BdataLoc(cbit, 4, (Int_t) 11, chan));   
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

  for (vector<BdataLoc *>::iterator p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
     BdataLoc *dataloc = *p;
     if ( dataloc->IsSlot() ) {  
       for (i = 0; i < evdata.GetNumHits(dataloc->crate, 
             dataloc->slot, dataloc->chan); i++) {
          dataloc->Load(evdata.GetData(dataloc->crate, dataloc->slot, dataloc->chan, i));
       }
     }
  }
  
// Crawl through the event and decode elements of fWordLoc which are defined
// as relative to a header.   fWordLoc are treated seperately from fCrateLoc
// for performance reasons; this loop could be slow !

  for (i = 0; i < evdata.GetEvLength(); i++) {
    for (vector<BdataLoc *>::iterator p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
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

  for (vector<BdataLoc *>::iterator p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;

// bit pattern of triggers
   for (Int_t i = 0; i < MAXBIT; i++) {
      sprintf(cbit,"bit%d",i+1);
      if ( dataloc->ThisIs(cbit) ) TrigBits(i+1,dataloc);
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

  for (vector<BdataLoc *>::iterator p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
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
  for (int i = 0; i < MAXBIT; i++) cout << " "<<i<<" = "<< bits[i]<<"  | ";
  cout << endl;
  cout << "event types,  CODA = "<<evtype<<"   bit pattern = "<<evtypebits<<endl;
  cout << "synch adcs "<<"  "<<synchadc1<<"  "<<synchadc2<<"  ";
  cout << synchadc3 <<"  "<<synchadc4<<"   "<<synchadc14<<endl;
  cout <<" time stamps "<<timestamp<<"  "<<timeroc1<<"  "<<timeroc2<<"  ";
  cout << timeroc3<<"  "<<timeroc4<<"  "<<timeroc14<<endl<<endl;
}


//_____________________________________________________________________________
Int_t THaDecData::Reconstruct()
{
  // Nothing required
  return 0;
}

//_____________________________________________________________________________
vector<string> THaDecData::vsplit(const string& s) {
// split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type string_size;
  string_size i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      string_size j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
}

//_____________________________________________________________________________
UInt_t THaDecData::header_str_to_base16(string hdr) {
// Utility to convert string header to base 16 integer
  static bool hs16_first = true;
  static map<char, int> strmap;
  typedef map<char,int>::value_type valType;
  //  pair<char, int> pci;
  static char chex[]="0123456789abcdef";
  static vector<int> numarray; 
  static int linesize = 12;
  if (hs16_first) {
    hs16_first = false;
    for (int i = 0; i < 16; i++) {
      //      pci.first = chex[i];  pci.second = i;  strmap.insert(pci);
      strmap.insert(valType(chex[i],i));
    }
    numarray.reserve(linesize);
  }
  numarray.clear();
  for (string::iterator p = hdr.begin(); p != hdr.end(); p++) {
    map<char, int>::iterator pm = strmap.find(*p);     
    if (pm != strmap.end()) numarray.push_back(pm->second);
    if ((long)numarray.size() > linesize-1) break;
  }
  UInt_t result = 0;  UInt_t power = 1;
  for (vector<int>::reverse_iterator p = numarray.rbegin(); 
      p != numarray.rend(); p++) {
      result += (*p) * power;  power *= 16;
  }
  return result;
};

//_____________________________________________________________________________
void THaDecData::TrigBits(Int_t ibit, BdataLoc *dataloc) {
// Figure out which triggers got a hit.  These are multihit TDCs, so we
// need to sort out which hit we want to take by applying cuts.

  if (ibit < 0 || ibit >= MAXBIT) return;
  bits[ibit] = 0;

  static UInt_t cutlo = 400;
  static UInt_t cuthi = 1200;
  
  for (int ihit = 0; ihit < dataloc->NumHits(); ihit++) {
    if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
      bits[ibit] = 1;
      evtypebits |= 1<<ibit;
    }
  }

}


