#ifndef ROOT_THaDecData
#define ROOT_THaDecData

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
//       related to the variables of this class (e.g. timeroc2 is a a raw
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

#include "THaApparatus.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>

#define MAXBIT   8

class BdataLoc {
// Utility class used by THaDecData.
// Data location, either in (crates, slots, channel), or
// relative to a unique header in a crate or in an event.
public:
   char* name;                 // name of the variable in global list.
   Int_t  search_choice;       // whether to search in crates or rel. to header
   Int_t  crate, slot, chan;   // where to look in crates
   UInt_t header;              // header (unique either in data or in crate)
   Int_t ntoskip;              // how far to skip beyond header
   vector<UInt_t > rdata;      // raw data.  (It is vector to accom. multihit chan.)
// c'tor for (crate,slot,channel) selection
   BdataLoc ( const char* nm, Int_t cra, Int_t slo, Int_t cha ) :
      search_choice(0), crate(cra), slot(slo), chan(cha), 
      header(0), ntoskip(0) { name = new char[strlen(nm)+1]; strcpy(name,nm); }
// c'tor for header search (note, the only diff to above is 3rd arg is UInt_t)
   BdataLoc ( const char* nm, Int_t cra, UInt_t head, Int_t skip ) :
      search_choice(1), crate(cra), slot(0), chan(0),
      header(head), ntoskip(skip) { name = new char[strlen(nm)+1]; strcpy(name,nm); }
   Bool_t IsSlot() { return (search_choice == 0); };
   void Clear() { rdata.clear();  loaded_once = kFALSE; };
   void Load(UInt_t data) { rdata.push_back(data);  loaded_once = kTRUE; };
   Bool_t DidLoad() { return loaded_once; };
   Int_t NumHits() { return rdata.size(); };
   UInt_t Get(Int_t i) { if (i >= 0 && rdata.size() > (unsigned int)i) return rdata[i]; return 0; };
   UInt_t Get() { if (rdata.size() > (unsigned int)0) return rdata[0]; return 0; };  
   Bool_t ThisIs(char* aname) { return (strstr(name,aname) != NULL); };
   ~BdataLoc() { delete name; }
private:
   Bool_t loaded_once;
   BdataLoc(const BdataLoc& dataloc);
   BdataLoc& operator=(const BdataLoc& dataloc);
}; 


class THaDecData : public THaApparatus {
  
public:
   THaDecData( const char* description="" );
   virtual ~THaDecData() {}

   virtual Int_t  Init();  
   virtual Int_t  Reconstruct();
   virtual Int_t  Decode( const THaEvData& );

private:
   char *cbit;
   Int_t nvar;
   Int_t *bits;
   UInt_t evtypebits, evtype;
   UInt_t synchadc1, synchadc2, synchadc3, 
          synchadc4, synchadc14;
   UInt_t timestamp, timeroc1, timeroc2, timeroc3,  
          timeroc4, timeroc14;
   UInt_t misc1, misc2, misc3, misc4;
   vector < BdataLoc* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   vector < BdataLoc* > fWordLoc;    // Raw Data locations relative to header word
   Bool_t kFirst;

   void Clear();
   void Dump();
   Int_t DefaultMap();
   void TrigBits(Int_t ibit, BdataLoc *dataloc);
   vector<string> vsplit(const string& s);
   UInt_t header_str_to_base16(string hdr);

   static const int THADEC_VERBOSE = 1;

   ClassDef(THaDecData,0)  
};

#endif








