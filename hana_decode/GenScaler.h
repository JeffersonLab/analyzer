#ifndef Podd_GenScaler_h_
#define Podd_GenScaler_h_

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

    GenScaler() : VmeModule() {}
    GenScaler(Int_t crate, Int_t slot);
    virtual ~GenScaler();

    using Module::GetData;
    using Module::LoadSlot;

    virtual void  Clear(const Option_t* opt="");
    virtual Int_t Decode(const UInt_t *evbuffer);
    virtual UInt_t GetData(Int_t chan) const;   // Raw scaler counts
    virtual Bool_t IsSlot(UInt_t rdata);
    virtual void DoPrint() const;

    void GenInit();
    Int_t SetClock(Double_t deltaT, Int_t clockchan=0, Double_t clockrate=0);
    Double_t GetRate(Int_t chan) const;  // Scaler rate
    Double_t GetTimeSincePrev() const;  // returns deltaT since last reading
    Bool_t IsDecoded() const { return fIsDecoded; };
    void LoadNormScaler(GenScaler *scal);  // loads pointer to norm. scaler
    void DebugPrint(std::ofstream *file=0) const;

    // Loads sldat
    virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );
    // Load slot for bank structures
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len);

    virtual void SetBank(Int_t bank);


  protected:

    void LoadRates();
    Bool_t checkchan(Int_t chan) const { return (chan >=0 && chan < fWordsExpect); }
    Bool_t fIsDecoded, fFirstTime, fDeltaT;
    UInt_t *fDataArray, *fPrevData;
    Double_t *fRate;
    Int_t fClockChan, fNumChanMask, fNumChanShift;
    Bool_t fHasClock;
    Double_t fClockRate;
    GenScaler *fNormScaler;
    static const int fgNumChanDefault=32;

    ClassDef(GenScaler,0)  //   A generic scaler.  Abstract class.

  };

}

#endif
