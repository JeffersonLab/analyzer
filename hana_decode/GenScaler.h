#ifndef Podd_GenScaler_h_
#define Podd_GenScaler_h_

/////////////////////////////////////////////////////////////////////
//
//   GenScaler
//   Generic scaler.
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"
#include <vector>

namespace Decoder {

  class GenScaler : public VmeModule {

  public:

    GenScaler() :
      fIsDecoded(false), fFirstTime(true), fDeltaT(0.0),
      fClockChan(0), fNumChanMask(0), fNumChanShift(0),
      fHasClock(false), fClockRate(0.0), fNormScaler(nullptr),
      firsttime(true), firstwarn(true) {}
    GenScaler( UInt_t crate, UInt_t slot );

    using VmeModule::GetData;

    virtual void   Clear( Option_t* opt = "" );
    virtual Int_t  Decode( const UInt_t* evbuffer );
    virtual UInt_t GetData( UInt_t chan ) const;   // Raw scaler counts
    virtual Bool_t IsSlot( UInt_t rdata );
    virtual void   DoPrint() const;

    void GenInit();
    Int_t    SetClock( Double_t deltaT, UInt_t clockchan = 0, Double_t clockrate = 0 );
    Double_t GetRate( UInt_t chan) const;  // Scaler rate
    Double_t GetTimeSincePrev() const;  // returns deltaT since last reading
    Bool_t   IsDecoded() const { return fIsDecoded; };
    void LoadNormScaler(GenScaler *scal);  // loads pointer to norm. scaler
    void DebugPrint(std::ofstream *file=nullptr) const;

    // Loads sldat
    virtual UInt_t LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop );
    // Load slot for bank structures
    virtual UInt_t LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer, UInt_t pos, UInt_t len);

    virtual void SetBank(Int_t bank);


  protected:

    void LoadRates();
    Bool_t checkchan( UInt_t chan ) const { return (chan < fWordsExpect); }
    Bool_t fIsDecoded, fFirstTime;
    Double_t fDeltaT;
    std::vector<UInt_t> fDataArray, fPrevData;
    std::vector<Double_t> fRate;
    UInt_t fClockChan, fNumChanMask, fNumChanShift;
    Bool_t fHasClock;
    Double_t fClockRate;
    GenScaler *fNormScaler;
    Bool_t firsttime, firstwarn;
    static const UInt_t fgNumChanDefault=32;

    ClassDef(GenScaler,0)  //   A generic scaler.  Abstract class.

  };

}

#endif
