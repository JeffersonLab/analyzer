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
#include "THashTable.h"

class TH1F;
class BdataLoc;

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
   Int_t SetupDecData( const TDatime* runTime = NULL, EMode mode = kDefine );
   virtual void BookHist(); 
   void VdcEff();

  static THaDecData* fgThis;   //Pointer to instance of this class
  static Int_t fgVdcEffFirst; //If >0, initialize VdcEff() on next call

   ClassDef(THaDecData,0)  
};

#endif








