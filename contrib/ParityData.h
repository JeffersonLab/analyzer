#ifndef Podd_ParityData_h_
#define Podd_ParityData_h_

//////////////////////////////////////////////////////////////////////////
//
// ParityData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "TBits.h"
//#include "TH1.h"
//#include "TH2.h"
#include <vector>
#include <iostream>
#include <string>
#include <cstring>

using namespace std;

class TH1;
class THaScaler;

class BdataLoc {
// Utility class used by ParityData.
// Data location, either in (crates, slots, channel), or
// relative to a unique header in a crate or in an event.
   static const Int_t MxHits=16;
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
   void Clear() { ndata=0;  loaded_once = kFALSE; }
   void Print() { 
     cout << "DataLoc "<<name<<"   crate "<<crate;
     if (IsSlot()) {
       cout << "  slot "<<slot<<"   chan "<<chan<<endl;
     } else {
       cout << "  header "<<header<<"  ntoskip "<<ntoskip<<endl;
     }
   } 
   void Load(UInt_t data) {
     if (ndata<MxHits) rdata[ndata++]=data;
     loaded_once = kTRUE;
   }
   Bool_t DidLoad() { return loaded_once; }
   Int_t NumHits() { return ndata; }
   UInt_t Get(Int_t i=0) { 
     return (i >= 0 && ndata > i) ? rdata[i] : 0; }
   Bool_t ThisIs(const char* aname) { return (strstr(name.c_str(),aname) != 0);}
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


class ParityData : public THaApparatus {
  
public:
   ParityData( const char* name="D", const char* description="" );
   virtual ~ParityData();

   virtual EStatus Init( const TDatime& run_time );
   virtual Int_t   End(THaRunBase* r=0);
   virtual void    WriteHist(); 
   virtual Int_t   Reconstruct() { return 0; }
   virtual Int_t   Decode( const THaEvData& );

protected:
   virtual BdataLoc* DefineChannel(BdataLoc*, EMode, const char* desc="automatic");

private:
   TBits  bits;
   Int_t  *trigcnt;

   UInt_t evtypebits, evtype;

   // HAPPEX detector ADC and TDC
   // Left (l) and Right (r) HRS
   Double_t hapadcl1,hapadcl2;
   Double_t hapadcr1,hapadcr2;
   Double_t haptdcl1,haptdcl2;
   Double_t haptdcr1,haptdcr2;
 
   // UMass Profile Scanner.  Amplitude, X, Y.
   // Left (l) and Right (r) HRS
   Double_t profampl, profxl, profyl;
   Double_t profampr, profxr, profyr;

   // Cavity fastbus and ped
   Double_t *cped, *cfb;
   // Calibration factors for cavity data
   Double_t axcav4a,aycav4a,aqcav4a;
   Double_t bxcav4a,bycav4a,bqcav4a;
   Double_t axcav4b,aycav4b,aqcav4b;
   Double_t bxcav4b,bycav4b,bqcav4b;

   // Processed cavity data 
   Double_t xcav4a,ycav4a,qcav4a;
   Double_t xcav4b,ycav4b,qcav4b;

   // Fastbus stripline data and ped
   Double_t alpha4ax,alpha4ay,alpha4bx,alpha4by;
   Double_t kappa4a,kappa4b;
   Double_t att4ax,att4ay,att4bx,att4by;
   Double_t off4ax,off4ay,off4bx,off4by;
   Double_t *sped, *sfb;

   // Processed stripline data
   Double_t xp4a,xm4a,yp4a,ym4a;
   Double_t xp4b,xm4b,yp4b,ym4b;
   Double_t xstrip4a,ystrip4a;
   Double_t xstrip4b,ystrip4b;

   // Scalers
   THaScaler *lscaler,*rscaler;

   // Q^2 and missing mass on left (L) and right (R) HRS.
   Double_t q2L,mmL,q2R,mmR;

   std::vector < BdataLoc* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   std::vector < BdataLoc* > fWordLoc;    // Raw Data locations relative to header word

   virtual void Clear( Option_t* opt="" );
   virtual void Print( Option_t* opt="" ) const;
   std::vector<TH1* > hist;
   Int_t DefaultMap();
   void  PrintMap(Int_t flag=0);
   Int_t InitCalib();
   Int_t DoBpm();
   Int_t DoKine();
   Int_t ProfileXY();
   void TrigBits(UInt_t ibit, BdataLoc *dataloc);
   static std::vector<std::string> vsplit(const std::string& s);
   Int_t SetupParData( const TDatime* runTime = NULL, EMode mode = kDefine );
   virtual void BookHist(); 

   static UInt_t header_str_to_base16(const char* hdr);

   static const int PARDATA_VERBOSE = 1;
   static const int PARDATA_PRINT = 0;

   ClassDef(ParityData,0)  
};

#endif








