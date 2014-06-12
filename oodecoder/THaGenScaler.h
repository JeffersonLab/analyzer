#ifndef THaGenScaler_
#define THaGenScaler_

/////////////////////////////////////////////////////////////////////
//
//   THaGenScaler
//   Generic scaler
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "ToyModule.h"

const int DEFAULT_DELTAT = 4;

class THaGenScaler : public ToyModule {

public:

   THaGenScaler() {};  
   THaGenScaler(Int_t crate, Int_t slot, Int_t numchan);  
   virtual ~THaGenScaler();  

   Int_t SetClock(Double_t deltaT, Int_t clockchan=0, Double_t clockrate=0);

   Bool_t IsSlot(Int_t rdata);
   Int_t Decode(const Int_t *evbuffer);
   Int_t GetData(Int_t chan);
   Double_t GetRate(Int_t chan);
   Double_t GetTimeSincePrev();  // returns deltaT since last reading
   void Clear(const Option_t *opt) { fIsDecoded=kFALSE; };
   Bool_t IsDecoded() { return fIsDecoded; };
   void LoadNormScaler(THaGenScaler *scal);
   void DoPrint();

protected:

   void LoadRates();

   Bool_t checkchan(Int_t chan) { return (chan >=0 && chan < fWordsExpect); }
   static TypeIter_t fgThisType;
   Bool_t fIsDecoded, fFirstTime, fDeltaT;
   Int_t *fDataArray, *fPrevData;
   Double_t *fRate;
   Int_t fClockChan;
   Bool_t fHasClock;
   Double_t fClockRate;
   THaGenScaler *fNormScaler;

   Int_t fNumChanMask;
   Int_t slotmask, chanmask, datamask;
   ClassDef(THaGenScaler,0)  //   A generic scaler

};

#endif
