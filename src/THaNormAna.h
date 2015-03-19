#ifndef ROOT_THaNormAna
#define ROOT_THaNormAna

//////////////////////////////////////////////////////////////////////////
//
// THaNormAna
//
//  Normalization Analysis
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TBits.h"
#include <vector>
#include <string>
#include <cstring> // for memset etc.
#include <iostream>
#include <cassert>

class TH1;
class THaScaler;

//TODO: does this need to be public?
class BRocScaler {
// Container class of ROC scaler data in THaNormAna.  All data public, so
// it is like a "struct".
 public:
  BRocScaler(Int_t ir, const char *nm, Int_t headp, Int_t headn, Int_t ch, 
	     Double_t *d) 
  : roc(ir), chan_name(nm), header_P(headp), header_N(headn), chan(ch) { data = d; }
  Int_t roc;                 // roc num you want
  std::string chan_name;     // name of channel
  Int_t header_P, header_N;  // headers of + & - helicity data
  Int_t chan;                // channel in scaler
  Double_t *data;            // pointer to data
};

class BNormData {
// Container class used by THaNormAna
public:
  BNormData() {
    memset(fData,0,NHEL*sizeof(BNorm_t));
    memset(psfact,0,NTRIG*sizeof(Int_t));
  }
  virtual ~BNormData() {}
  void Print(Int_t ihel=0) {
    using std::cout; using std::endl;
    cout << "Print of normdata";
    cout << " for helicity "<<ihel<<endl; 
    cout << "EvCount "<<GetEvCount(ihel)<<endl;
    cout << "Avg Live "<<GetAvgLive(ihel)<<endl;
    cout << "Correction "<<GetCorrfact(ihel)<<endl;
    for (int itrig = 0; itrig < NTRIG; itrig++) {
      cout << "Trig "<<itrig+1;
      cout << "  PS "<<GetPrescale(itrig);
      cout << "  Latch "<< GetTrigCount(itrig,ihel);
      cout << "  Live "<<GetLive(itrig,ihel)<<endl;
    }
  }
  Int_t InvHel(Int_t hel) {
// hel = (1,0,2)  maps to -1, 0, +1  
// 0 = no helicity in either case
    if (hel == 1) return -1;
    if (hel == 2) return 1;
    return 0;
  }
  void EvCount(int hel=0) { 
  // By my def, hel=0 is irrespective of helicity
    fData[Hel(0)].evcnt++;
    if (hel!=0) fData[Hel(hel)].evcnt++; 
  }
  void TrigCount(int trig, int hel=0) {
  // By my def, hel=0 is irrespective of helicity
    fData[Hel(0)].trigcnt[Trig(trig)]++;
    if (hel!=0) fData[Hel(hel)].trigcnt[Trig(trig)]++;
  }
  void SetLive(Int_t trig, Double_t ldat, Int_t hel=0) {
    fData[Hel(hel)].live[Trig(trig)] = ldat;
  }
  void SetPrescale(Int_t trig, Int_t ps) {
     psfact[Trig(trig)] = ps;
  }
  void SetAvgLive(Double_t alive, Double_t corr, 
             Int_t hel=0) {
    fData[Hel(hel)].avglive  = alive;
    fData[Hel(hel)].corrfact = corr;
  }
  Double_t GetAvgLive(Int_t hel=0) {
     return fData[Hel(hel)].avglive;
  }
  Double_t GetCorrfact(Int_t hel=0) {
    return fData[Hel(hel)].corrfact;
  }
  Double_t GetLive(Int_t trig, Int_t hel=0) {
    return fData[Hel(hel)].live[Trig(trig)];
  }
  Double_t GetPrescale(Int_t trig) {
    return psfact[Trig(trig)];
  }
  Double_t GetEvCount(Int_t hel=0) {
    return fData[Hel(hel)].evcnt;
  }
  Double_t GetTrigCount(Int_t trig, Int_t hel=0) {
    return fData[Hel(hel)].trigcnt[Trig(trig)];
  }
private:
  static const Int_t NHEL = 3, NTRIG = 12;
  struct BNorm_t {
    Int_t evcnt;
    Int_t trigcnt[NTRIG];
    Double_t avglive;
    Double_t corrfact;
    Double_t live[NTRIG];
  };
  BNorm_t fData[NHEL];
  Int_t psfact[NTRIG];
  Int_t Trig(Int_t idx) {
// Trigger indices are 0,1,2... for T1,T2,..
    assert(idx >= 0 && idx < NTRIG);
    return idx;
  }
  Int_t Hel(Int_t hel) {
// hel = -1, 0, +1  maps to (1,0,2)  
// 0 = no helicity in either case
    if (hel == -1) return 1;
    if (hel == 1) return 2;
    return 0;
  }
};


class THaNormAna : public THaPhysicsModule {
  
public:
   THaNormAna( const char* name="N", const char* description="Normalizion Analysis" );
   virtual ~THaNormAna();

   virtual EStatus Init( const TDatime& run_time );
   virtual Int_t   End(THaRunBase* r=0);
   virtual void    WriteHist(); 
   virtual void    DoBcmCalib() { fDoBcmCalib = kTRUE; };
   virtual void    SetEventInt(Int_t *ev);
   virtual Int_t   Reconstruct() { return 0; }
   virtual Int_t   Process( const THaEvData& );
   virtual Int_t   PrintSummary() const;

private:

   TBits  bits;
   THaScaler *myscaler;
   BNormData *normdata;
   std::vector<BRocScaler*> fRocScaler;
   Bool_t fSetPrescale,fHelEnable,fDoBcmCalib;
   UInt_t evtypebits;    // trigger bit pattern
   Int_t *tdcdata, *nhit;   // tdc for trigger bit
   Double_t alive,hpos_alive,hneg_alive; // scaler livetime
   Double_t dlive;   // differential livetime (one helicity)
   Double_t bcmu3;       // a BCM (rate)
   Int_t *eventint;      // event intervals for BCM calib
   // scaler data from roc10 or 11 (counts)
   Double_t roc11_bcmu3, roc11_bcmu10;
   Double_t roc11_t1,roc11_t2,roc11_t3,roc11_t4,roc11_t5;
   Double_t roc11_clk1024, roc11_clk104k;
   Double_t *norm_scaler, *norm_plus, *norm_minus;
   // current calibration:
   Double_t off_u1, off_u3, off_u10;
   Double_t off_d1, off_d3, off_d10;
   Double_t calib_u1, calib_u3, calib_u10;
   Double_t calib_d1, calib_d3, calib_d10;

   virtual void Print( Option_t* opt="" ) const;
   std::vector<TH1* > hist;
   void TrigBits( Int_t helicity );
   void LiveTime();
   void CalcAsy();
   void GetRocScalers(const THaEvData& );
   Int_t SetupRawData( const TDatime* runTime = NULL, EMode mode = kDefine );
   void InitRocScalers();
   virtual Int_t BcmCalib( const THaEvData& );
   virtual void BookHist(); 

   static const int fgMaxEvInt = 100;
   static const int fgNumRoc = 2;
   static const int fgNumChan = 32;

   static const int fDEBUG = 0;

  ClassDef(THaNormAna,0)  // Normalization analysis utilities
};

#endif








