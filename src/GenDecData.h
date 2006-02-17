#ifndef ROOT_GenDecData
#define ROOT_GenDecData

//////////////////////////////////////////////////////////////////////////
//
// GenDecData
//
//   Copy of the THaDecData class, with the added functionality of
//   being able to specify and record variables not listed in the
//   code.  This code specific to Gen experiment e02013.
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "TBits.h"
//#include "TH1.h"
//#include "TH2.h"
#include <vector>
#include <string>
#include <cstring>

class TH1F;

class BdataLoc2 {
// Utility class used by GenDecData.
// Data location, either in (crates, slots, channel), or
// relative to a unique header in a crate or in an event.
  static const Int_t MxHits=16;
public:
// c'tor for (crate,slot,channel) selection
   BdataLoc2 ( const char* nm, Int_t cra, Int_t slo, Int_t cha ) :
     crate(cra), slot(slo), chan(cha), header(0), ntoskip(0), 
     name(nm), search_choice(0) {   }
// c'tor for header search (note, the only diff to above is 3rd arg is UInt_t)
   BdataLoc2 ( const char* nm, Int_t cra, UInt_t head, Int_t skip ) :
     crate(cra), slot(0), chan(0), header(head), ntoskip(skip),
     name(nm), search_choice(1) {   }
   Bool_t IsSlot() { return (search_choice == 0); }
   void Clear() { ndata = 0;  loaded_once = kFALSE; }
   void DoLoad(const THaEvData& evdata);
   
   void Load(UInt_t data) {
     if (ndata<MxHits)  rdata[ndata++]=data;
     loaded_once = kTRUE;
   }
   Bool_t DidLoad() { return loaded_once; }
   Int_t NumHits() { return ndata; }
   UInt_t Get(Int_t i=0) { 
     return (0 <= i && i < ndata) ? rdata[i] : 0; }
   Bool_t ThisIs(const char* aname) { return (strstr(name.c_str(),aname) != 0);}
   ~BdataLoc2() {}

   Int_t  crate, slot, chan;   // where to look in crates
   UInt_t header;              // header (unique either in data or in crate)
   Int_t ntoskip;              // how far to skip beyond header
   const std::string name;     // name of the variable in global list.

   UInt_t rdata[MxHits];  //[ndata] raw data.  (to accom. multihit chan.)
   Int_t ndata;           // number of relevant entries
private:
   Int_t  search_choice;       // whether to search in crates or rel. to header
 /* unfortunately ROOT 3.03-06 with gcc-3.0 don't like these next lines */
   Bool_t loaded_once;
   BdataLoc2();
   BdataLoc2(const BdataLoc2& dataloc);
   BdataLoc2& operator=(const BdataLoc2& dataloc);
}; 


class GenDecData : public THaApparatus {
  
public:
   GenDecData( const char* name="D", const char* description="" );
   virtual ~GenDecData();

   virtual EStatus Init( const TDatime& run_time );
   virtual Int_t   End(THaRunBase* r=0);
   virtual Int_t   Reconstruct() { return 0; }
   virtual Int_t   Decode( const THaEvData& );

private:
   TBits  bits;
   UInt_t evtypebits, evtype, evnum, timestamp;
   Double_t ctimebb, ctimec, ctimeND;
   UInt_t ctr20,ctr21,ctr22,ctr23,ctr24,ctr25,ctr28;
   UInt_t vtime20,vtime21,vtime22,vtime23,vtime24,vtime25,vtime28;
   UInt_t ftime20,ftime21,ftime22,ftime23,ftime24,ftime25,ftime28;
   UInt_t evnum20,evnum21,evnum22,evnum23,evnum24,evnum25,evnum28;
   UInt_t evtype20,evtype21,evtype22,evtype23,evtype24,evtype25,evtype28;
   UInt_t dscan20,dscan21,dscan22,dscan23,dscan24,dscan25,dscan28;
   Double_t rftime1,rftime2;
   UInt_t misc1, misc2, misc3, misc4;
   std::vector < BdataLoc2* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   std::vector < BdataLoc2* > fWordLoc;    // Raw Data locations relative to header word

   virtual void Clear( Option_t* opt="" );
   virtual void Print( Option_t* opt="" ) const;
   Int_t DefaultMap();
   void TrigBits(UInt_t ibit, BdataLoc2 *dataloc);
   static std::vector<std::string> vsplit(const std::string& s);
   Int_t SetupRawData( const TDatime* runTime = NULL, EMode mode = kDefine );
   static UInt_t header_str_to_base16(const char* hdr);

   static const int THADEC_VERBOSE = 1;

   ClassDef(GenDecData,0)  
};

#endif








