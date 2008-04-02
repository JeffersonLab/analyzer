#ifndef ROOT_THaG0HelicityReader
#define ROOT_THaG0HelicityReader

//////////////////////////////////////////////////////////////////////////
//
// THaG0HelicityReader
//
// Routines for decoding G0 helicity hardware
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

// If defined, compile with support for ring buffer scalers
//#define G0RINGBUFF

class THaEvData;

class THaG0HelicityReader {
  
public:
  THaG0HelicityReader();
  virtual ~THaG0HelicityReader();
  
  Bool_t GetValidTime() const { return fValidTime; }

  // Get ROC helicity information (for debugging)
  Int_t  GetQrt()      const { return fQrt; }
  Int_t  GetGate()     const { return fGate; }
  Int_t  GetReading()  const { return fPresentReading; }

#ifdef G0RINGBUFF
  // Get methods for Ring Buffer scalers
  Int_t  GetNumRead() const { return fNumread; } // Latest valid reading
  Int_t  GetBadRead() const { return fBadread; } // Latest problematic reading
  Int_t  GetRingClk()      const { return fRing_clock; }
  Int_t  GetRingQrt()      const { return fRing_qrt; }
  Int_t  GetRingHelicity() const { return fRing_helicity; }
  Int_t  GetRingTrig()     const { return fRing_trig; }
  Int_t  GetRingBCM()      const { return fRing_bcm; }
  Int_t  GetRingl1a()      const { return fRing_l1a; }
  Int_t  GetRingV2fh()     const { return fRing_v2fh; }
#endif

protected:

  // Used by ReadDatabase
  enum EROC { kHel = 0, kTime, kROC2, kROC3 };
  Int_t SetROCinfo( EROC which, Int_t roc, Int_t header, Int_t index );

  void  ClearG0();

  virtual Int_t ReadData( const THaEvData& evdata );

  Int_t fPresentReading;       // Current helicity reading
  Int_t fQrt, fGate;           // Current QRT and Gate

  Double_t fTimestamp;         // Event time from 105 kHz clock
  Double_t fOldT1, fOldT2, fOldT3;  // Last event's timestamps
  Bool_t   fValidTime;         // fTimestamp valid

  // ROC information
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).
  struct ROCinfo {
    Int_t roc;            // ROC to read out
    Int_t header;         // Headers to search for (0 = ignore)
    Int_t index;          // Index into buffer
  };
  ROCinfo fROCinfo[kROC3+1]; // Primary readouts and two redundant clocks

  Bool_t fHaveROCs;       // Required ROCs are defined
  Int_t  fG0Debug;        // Debug level

#ifdef G0RINGBUFF
  Int_t ReadRingScalers( const THaEvData& evdata, Int_t hroc, Int_t index );
  // ring buffer scaler data
  Int_t fNumread;         //!
  Int_t fBadread;         //!
  Int_t fRing_clock;      //!
  Int_t fRing_qrt;        //!
  Int_t fRing_helicity;   //!
  Int_t fRing_trig;       //!
  Int_t fRing_bcm;        //!
  Int_t fRing_l1a;        //!
  Int_t fRing_v2fh;       //!
#endif

private:

  static Int_t FindWord( const THaEvData& evdata, const ROCinfo& info );

  ClassDef(THaG0HelicityReader,1) // Helper class for reading G0 helicity data

};

#endif
