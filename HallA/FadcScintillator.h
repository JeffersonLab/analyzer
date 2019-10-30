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
//  virtual Int_t Decode( const THaEvData& );

  struct FADCData_t {
    FADCData_t() : fPeak(0), fT(0), fT_c(0), fOverflow(0), fUnderflow(0), fPedq(0) {}
    void clear() { fPeak = fT = fT_c = 0.0; fOverflow = fUnderflow = fPedq = 0; }
    Data_t fPeak;      // ADC peak value
    Data_t fT;         // TDC time (channels)
    Data_t fT_c;       // Offset-corrected TDC time (s)
    Int_t  fOverflow;  // FADC overflow bit
    Int_t  fUnderflow; // FADC underflow bit
    Int_t  fPedq;      // FADC pedestal quality bit
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
  std::vector<FADCData_t>  fFADCDataR;
  std::vector<FADCData_t>  fFADCDataL;

  Decoder::Fadc250Module*  fFADC; // FADC module currently being decoded

  virtual void     SetupModule( const THaEvData& evdata, DetMapItem* pModule );
  virtual OptInt_t LoadData( const THaEvData& evdata, DetMapItem* pModule,
    Bool_t adc, Int_t chan, Int_t hit, Int_t pad, ESide side );

  virtual Int_t ReadDatabase( const TDatime& date );
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(FadcScintillator, 1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif
