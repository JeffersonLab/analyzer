#ifndef ROOT_THaDecData
#define ROOT_THaDecData

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>

#define MAXBIT   8

class BdataLoc {
// Utility class used by THaDecData.
// Data location, either in (crates, slots, channel), or
// relative to a unique header in a crate or in an event.
public:
// c'tor for (crate,slot,channel) selection
   BdataLoc ( const char* nm, Int_t cra, Int_t slo, Int_t cha ) :
     crate(cra), slot(slo), chan(cha), header(0), ntoskip(0), 
     name(nm), search_choice(0) {}
// c'tor for header search (note, the only diff to above is 3rd arg is UInt_t)
   BdataLoc ( const char* nm, Int_t cra, UInt_t head, Int_t skip ) :
     crate(cra), slot(0), chan(0), header(head), ntoskip(skip),
     name(nm), search_choice(1) {}
   Bool_t IsSlot() { return (search_choice == 0); }
   void Clear() { rdata.clear();  loaded_once = kFALSE; }
   void Load(UInt_t data) { rdata.push_back(data);  loaded_once = kTRUE; }
   Bool_t DidLoad() { return loaded_once; }
   Int_t NumHits() { return rdata.size(); }
   UInt_t Get(Int_t i=0) { 
     return (i >= 0 && rdata.size() > (unsigned int)i) ? rdata[i] : 0; }
   Bool_t ThisIs(const char* aname) { return (strstr(name.c_str(),aname) != 0);}
   ~BdataLoc() {}

   Int_t  crate, slot, chan;   // where to look in crates
   UInt_t header;              // header (unique either in data or in crate)
   Int_t ntoskip;              // how far to skip beyond header

private:
   const string name;          // name of the variable in global list.
   Int_t  search_choice;       // whether to search in crates or rel. to header
   vector<UInt_t> rdata;       // raw data.  (It is vector to accom. multihit chan.)
   Bool_t loaded_once;
   BdataLoc();
   BdataLoc(const BdataLoc& dataloc);
   BdataLoc& operator=(const BdataLoc& dataloc);
}; 


class THaDecData : public THaApparatus {
  
public:
   THaDecData( const char* name="D", const char* description="" );
   virtual ~THaDecData();

   virtual EStatus Init( const TDatime& run_time );
   virtual Int_t   Reconstruct() { return 0; }
   virtual Int_t   Decode( const THaEvData& );

private:
   Int_t *bits;
   UInt_t evtypebits, evtype;
   Double_t ctimel, ctimer;
   Double_t pulser1;
   UInt_t synchadc1, synchadc2, synchadc3, 
          synchadc4, synchadc14;
   UInt_t timestamp, timeroc1, timeroc2, timeroc3,  
          timeroc4, timeroc14;
   UInt_t misc1, misc2, misc3, misc4;
   vector < BdataLoc* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   vector < BdataLoc* > fWordLoc;    // Raw Data locations relative to header word

   virtual void Clear( Option_t* opt="" );
   virtual void Print( Option_t* opt="" ) const;
   Int_t DefaultMap();
   void TrigBits(Int_t ibit, BdataLoc *dataloc);
   static vector<string> vsplit(const string& s);
   Int_t SetupDecData( const TDatime* runTime = NULL, EMode mode = kDefine );
   static UInt_t header_str_to_base16(const char* hdr);

   static const int THADEC_VERBOSE = 1;

   ClassDef(THaDecData,0)  
};

#endif








