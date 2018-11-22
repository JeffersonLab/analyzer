#ifndef Podd_THaG0HelicityReader_h_
#define Podd_THaG0HelicityReader_h_

//////////////////////////////////////////////////////////////////////////
//
// THaG0HelicityReader
//
// Routines for decoding G0 helicity hardware
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

class THaEvData;
class TDatime;

class THaG0HelicityReader {
  
public:
  THaG0HelicityReader();
  virtual ~THaG0HelicityReader();
  
  Bool_t GetValidTime() const { return fValidTime; }

  // Get ROC helicity information (for debugging)
  Int_t  GetQrt()      const { return fQrt; }
  Int_t  GetGate()     const { return fGate; }
  Int_t  GetReading()  const { return fPresentReading; }
  
protected:

  // Used by ReadDatabase
  enum EROC { kHel = 0, kTime, kROC2, kROC3 };
  struct ROCinfo {
    Int_t  roc;               // ROC to read out
    Int_t  header;            // Headers to search for (0 = ignore)
    Int_t  index;             // Index into buffer
  };

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t ReadData( const THaEvData& evdata );
  Int_t         ReadDatabase( const char* dbfilename, const char* prefix,
			      const TDatime& date, int debug_flag = 0 );

  Bool_t   fPresentReading;   // Current helicity reading
  Bool_t   fQrt;              // Current QRT
  Bool_t   fGate;             // Current gate

  Double_t fTimestamp;        // Event time from 105 kHz clock
  Double_t fOldT1;            // Last event's timestamps
  Double_t fOldT2;
  Double_t fOldT3;
  Bool_t   fValidTime;        // fTimestamp valid

  // ROC information
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).
  ROCinfo  fROCinfo[kROC3+1]; // Primary readouts and two redundant clocks
                              // helroc, timeroc, time2roc, time3roc
  Int_t    fG0Debug;          // Debug level
  Bool_t   fHaveROCs;         // Required ROCs are defined
  Bool_t   fNegGate;          // Invert polarity of gate, so that 0=active

private:

  static Int_t FindWord( const THaEvData& evdata, const ROCinfo& info );

  ClassDef(THaG0HelicityReader,2) // Helper class for reading G0 helicity data

};

#endif
