#ifndef GenScaler_
#define GenScaler_

/////////////////////////////////////////////////////////////////////
//
//   GenScaler
//   Generic scaler.
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"

namespace Decoder {

class GenScaler : public VmeModule {

public:

   GenScaler() {};
   GenScaler(Int_t crate, Int_t slot);
   virtual ~GenScaler();

   using Module::GetData;

   virtual void  Clear(const Option_t *opt) { fIsDecoded=kFALSE; };
   virtual Int_t Decode(const UInt_t *evbuffer);
   virtual Int_t GetData(Int_t chan) const;   // Raw scaler counts
   virtual Bool_t IsSlot(UInt_t rdata);
   virtual void DoPrint() const;

   void GenInit();
   Int_t SetClock(Double_t deltaT, Int_t clockchan=0, Double_t clockrate=0);
   Double_t GetRate(Int_t chan) const;  // Scaler rate
   Double_t GetTimeSincePrev() const;  // returns deltaT since last reading
   Bool_t IsDecoded() const { return fIsDecoded; };
   void LoadNormScaler(GenScaler *scal);  // loads pointer to norm. scaler
   void DebugPrint(ofstream *file=0) const;

// Loads sldat
  virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );

protected:

   void LoadRates();
   Bool_t checkchan(Int_t chan) const { return (chan >=0 && chan < fWordsExpect); }
   Bool_t fIsDecoded, fFirstTime, fDeltaT;
   Int_t *fDataArray, *fPrevData;
   Double_t *fRate;
   Int_t fNumChan, fClockChan, fNumChanMask;
   Bool_t fHasClock;
   Double_t fClockRate;
   GenScaler *fNormScaler;

   ClassDef(GenScaler,0)  //   A generic scaler.  Abstract class.

};

}

#endif
