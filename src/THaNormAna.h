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
#include <cstring>
#include <iostream>

#define NTRIG 12

using namespace std;

class TH1;
class THaScaler;

class BNormData {
// Container class used by THaNormAna
public:
  BNormData() {
    evcnt = new Int_t[3];
    avglive = new Double_t[3];
    corrfact = new Double_t[3];
    trigcnt = new Int_t[3*NTRIG];
    live = new Double_t[3*NTRIG];
    psfact = new Int_t[NTRIG];
    memset(evcnt,0,3*sizeof(Int_t));
    memset(avglive,0,3*sizeof(Double_t));
    memset(corrfact,0,3*sizeof(Double_t));
    memset(trigcnt,0,3*NTRIG*sizeof(Int_t));
    memset(live,0,3*NTRIG*sizeof(Double_t));
    memset(psfact,0,NTRIG*sizeof(Int_t));
  }
  virtual ~BNormData() {
    delete [] evcnt;
    delete [] avglive;
    delete [] corrfact;
    delete [] trigcnt;
    delete [] live;
    delete [] psfact;
  }
  void Print(Int_t ihel=0) {
    cout << "Print of normdata";
    cout << " for helicity "<<ihel<<endl; 
    cout << "EvCount "<<GetEvCount(ihel)<<endl;
    cout << "Avg Live "<<GetAvgLive(ihel)<<endl;
    cout << "Correction "<<GetCorrfact(ihel)<<endl;
    for (int itrig = 0; itrig < 12; itrig++) {
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
    evcnt[0]++;
    if (hel!=0) evcnt[Hel(hel)]++; 
  }
  void TrigCount(int trig, int hel=0) {
  // By my def, hel=0 is irrespective of helicity
    trigcnt[Trig(trig)]++;
    if (hel!=0) trigcnt[NTRIG*Hel(hel)+Trig(trig)]++;
  }
  void SetLive(Int_t trig, Double_t ldat, Int_t hel=0) {
     live[NTRIG*Hel(hel)+Trig(trig)] = ldat;
  }
  void SetPrescale(Int_t trig, Int_t ps) {
     psfact[Trig(trig)] = ps;
  }
  void SetAvgLive(Double_t alive, Double_t corr, 
             Int_t hel=0) {
     avglive[Hel(hel)] = alive;
     corrfact[Hel(hel)] = corr;
  }
  Double_t GetAvgLive(Int_t hel=0) {
     return avglive[Hel(hel)];
  }
  Double_t GetCorrfact(Int_t hel=0) {
     return corrfact[Hel(hel)];
  }
  Double_t GetLive(Int_t trig, Int_t hel=0) {
     return live[NTRIG*Hel(hel)+Trig(trig)];
  }
  Double_t GetPrescale(Int_t trig) {
      return (Double_t)psfact[Trig(trig)];
  }
  Double_t GetEvCount(Int_t hel=0) {
    return (Double_t)evcnt[Hel(hel)];
  }
  Double_t GetTrigCount(Int_t trig, Int_t hel=0) {
    return (Double_t)trigcnt[NTRIG*Hel(hel)+Trig(trig)];
  }
private:
  Int_t *evcnt, *trigcnt, *psfact;
  Double_t *avglive, *corrfact, *live;
  Int_t Trig(Int_t idx) {
// Trigger indices are 0,1,2... for T1,T2,..
    if (idx >= 0 && idx < NTRIG) return idx;
    return 0;
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
   Bool_t fSetPrescale,fHelEnable,fDoBcmCalib;
   UInt_t evtypebits;    // trigger bit pattern
   Int_t *tdcdata, *nhit;   // tdc for trigger bit
   Double_t alive,hpos_alive,hneg_alive; // scaler livetime
   Double_t bcmu3;       // a BCM (rate)
   Int_t *eventint;      // event intervals for BCM calib
   Double_t off_u1, off_u3, off_u10;
   Double_t off_d1, off_d3, off_d10;
   Double_t calib_u1, calib_u3, calib_u10;
   Double_t calib_d1, calib_d3, calib_d10;

   virtual void Print( Option_t* opt="" ) const;
   std::vector<TH1* > hist;
   void TrigBits( Int_t helicity );
   void LiveTime();
   Int_t SetupRawData( const TDatime* runTime = NULL, EMode mode = kDefine );
   virtual Int_t BcmCalib( const THaEvData& );
   virtual void BookHist(); 

   static const int fgMaxEvInt = 100;

   static const int fDEBUG = 0;

   ClassDef(THaNormAna,0)  
};

#endif








