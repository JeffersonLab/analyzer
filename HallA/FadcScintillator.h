#ifndef ROOT_TriFadcScin
#define ROOT_TriFadcScin

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// FadcScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaScintillator.h"

namespace Decoder {
 class Fadc250Module;
}

namespace HallA {

class FadcScintillator : public THaScintillator {

public:
  explicit FadcScintillator( const char* name, const char* description = "",
                             THaApparatus* a = nullptr );
  FadcScintillator();
  virtual ~FadcScintillator();

  virtual void Clear( Option_t* = "" );
  virtual Int_t Decode( const THaEvData& );

  struct FADCData_t {
    FADCData_t() :
      fLPeak(0), fLT_FADC(0), fLT_FADC_c(0), floverflow(0), flunderflow(0), flpedq(0),
      fRPeak(0), fRT_FADC(0), fRT_FADC_c(0), froverflow(0), frunderflow(0), frpedq(0),
      fLNhits(0), fRNhits(0) {}
    void clear() {
      fLPeak = fLT_FADC = fLT_FADC_c = fRPeak = fRT_FADC = fRT_FADC_c = 0.0;
      floverflow = flunderflow = flpedq = froverflow = frunderflow = frpedq =
      fLNhits = fRNhits = 0;
    }
    Data_t fLPeak;        // Left PMT FADC ADC peak value
    Data_t fLT_FADC;      // Left PMT FADC TDC times (channels)
    Data_t fLT_FADC_c;    // Left PMT corrected FADC TDC times (s)
    Int_t  floverflow;    // FADC overflow bit
    Int_t  flunderflow;   // FADC underflow bit
    Int_t  flpedq;        // FADC pedestal quality bit

    Data_t fRPeak;        // Right PMT FADC ADC peak value
    Data_t fRT_FADC;      // Right PMT FADC TDC times (channels)
    Data_t fRT_FADC_c;    // Right PMT corrected FADC TDC times (s)
    Int_t  froverflow;    // FADC overflow bit Right PMT
    Int_t  frunderflow;   // FADC underflow bit Right PMT
    Int_t  frpedq;        // FADC pedestal quality bit Right PMT

    Int_t  fLNhits;       // Number of hits for each Left PMT
    Int_t  fRNhits;       // Number of hits for each Right PMT
  };

protected:

  // Calibration
  struct FADCConfig_t {
    FADCConfig_t() : nped(1), nsa(1), nsb(1), win(1), tflag(true) {}
    void clear() { nped = nsa = nsb = win = 1; tflag = true; }
    Int_t  nped;          // Number of samples included in FADC pedestal sum
    Int_t  nsa;           // Number of integrated samples after threshold crossing
    Int_t  nsb;           // Number of integrated samples before threshold crossing
    Int_t  win;           // Total number of samples in FADC window
    Bool_t tflag;         // If true, threshold on
  };
  FADCConfig_t  fFADCConfig;

  // Per-event data
  std::vector<FADCData_t>  fFADCData;

  Decoder::Fadc250Module* fFADC;     //pointer to FADC250Module class

  virtual Int_t ReadDatabase( const TDatime& date );
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(FadcScintillator, 1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif
