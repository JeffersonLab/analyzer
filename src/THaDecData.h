#ifndef ROOT_THaDecData
#define ROOT_THaDecData

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "TBits.h"
//#include "TH1.h"
//#include "TH2.h"
#include <vector>
#include <string>

class TH1F;

class BdataLoc {
// Utility class used by THaDecData.
// Data location, either in (crates, slots, channel), or
// relative to a unique header in a crate or in an event.
   static const Int_t MxHits=16;
 public:
   // c'tor for (crate,slot,channel) selection
   BdataLoc ( const char* nm, Int_t cra, Int_t slo, Int_t cha ) :
     crate(cra), slot(slo), chan(cha), header(0), ntoskip(0), 
     name(nm), search_choice(0) { Clear(); }
   // c'tor for header search (note, the only diff to above is 3rd arg is UInt_t)
  //FIXME: this kind of overloading (Int/UInt) is asking for trouble...
   BdataLoc ( const char* nm, Int_t cra, UInt_t head, Int_t skip ) :
     crate(cra), slot(0), chan(0), header(head), ntoskip(skip),
     name(nm), search_choice(1) { Clear(); }
   Bool_t IsSlot() { return (search_choice == 0); }
   void Clear() { ndata=0;  loaded_once = kFALSE; }
   void Load(UInt_t data) {
     if (ndata<MxHits) rdata[ndata++]=data;
     loaded_once = kTRUE;
   }
   Bool_t DidLoad() { return loaded_once; }
   Int_t NumHits() { return ndata; }
   UInt_t Get(Int_t i=0) { 
     return (i >= 0 && ndata > i) ? rdata[i] : 0; }
  //FIXME: this should be operator==( const char* )
   Bool_t ThisIs(const char* aname) { return name==aname;}
  // operator== and != compare the hardware definitions of two BdataLoc's
  Bool_t operator==( const BdataLoc& rhs ) const {
    return ( search_choice == rhs.search_choice && crate == rhs.crate &&
	     ( (search_choice == 0 && slot == rhs.slot && chan == rhs.chan) ||
	       (search_choice == 1 && header == rhs.header && 
		ntoskip == rhs.ntoskip)
	       )  
	     );
  }
  Bool_t operator!=( const BdataLoc& rhs ) const { return !(*this == rhs ); }
  void SetSlot( Int_t cr, Int_t sl, Int_t ch ) {
    crate = cr; slot = sl; chan = ch; header = ntoskip = search_choice = 0;
  }
  void SetHeader( Int_t cr, UInt_t hd, Int_t sk ) {
    crate = cr; header = hd; ntoskip = sk; slot = chan = 0; search_choice = 1;
  }
   ~BdataLoc() {}
   
   Int_t  crate, slot, chan;   // where to look in crates
   UInt_t header;              // header (unique either in data or in crate)
   Int_t ntoskip;              // how far to skip beyond header
   const std::string name;     // name of the variable in global list.
   
   UInt_t rdata[MxHits];       //[ndata] raw data (to accom. multihit chanl)
   Int_t  ndata;               // number of relevant entries
 private:
   Int_t  search_choice;       // whether to search in crates or rel. to header
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
   virtual Int_t   End(THaRunBase* r=0);
   virtual void    WriteHist(); 
   virtual Int_t   Reconstruct() { return 0; }
   virtual Int_t   Decode( const THaEvData& );

   void Reset( Option_t* opt="" );

protected:
   virtual BdataLoc* DefineChannel(BdataLoc*, EMode, const char* desc="automatic");

private:
   TBits  bits;
   UInt_t evtypebits, evtype;
   Double_t ctimel, ctimer;
   Double_t pulser1;
   UInt_t synchadc1, synchadc2, synchadc3, 
          synchadc4, synchadc14;
   UInt_t timestamp, timeroc1, timeroc2, timeroc3,  
          timeroc4, timeroc14;
   Double_t rftime1,rftime2;
   Double_t edtpl,edtpr;
   Double_t lenroc12,lenroc16;
   Int_t  cnt1;
  //FIXME: combine into one collection for better efficiency - and decide on the
  // proper data structure (array vs. list/hashlist etc.)
   std::vector < BdataLoc* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   std::vector < BdataLoc* > fWordLoc;    // Raw Data locations relative to header word

   virtual void Clear( Option_t* opt="" );
   virtual void Print( Option_t* opt="" ) const;
   std::vector<TH1F* > hist;
   Int_t DefaultMap();
   void TrigBits(UInt_t ibit, BdataLoc *dataloc);
   static std::vector<std::string> vsplit(const std::string& s);
   Int_t SetupDecData( const TDatime* runTime = NULL, EMode mode = kDefine );
   virtual void BookHist(); 
   void VdcEff();
   static UInt_t header_str_to_base16(const char* hdr);

  static THaDecData* fgThis;   //Pointer to instance of this class
  static Int_t fgVdcEffFirst; //If >0, initialize VdcEff() on next call

   ClassDef(THaDecData,0)  
};

#endif








