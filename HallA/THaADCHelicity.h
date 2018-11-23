#ifndef Podd_THaADCHelicity_h_
#define Podd_THaADCHelicity_h_

////////////////////////////////////////////////////////////////////////
//
// THaADCHelicity
//
// Helicity of the beam - from ADC
// 
////////////////////////////////////////////////////////////////////////


#include "THaHelicityDet.h"
#include <vector>
#include <stdexcept>
#include <limits>

class THaADCHelicity : public THaHelicityDet {

public:

  THaADCHelicity( const char* name, const char* description, 
		  THaApparatus* a = NULL );
  virtual ~THaADCHelicity();

  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );

  THaADCHelicity()
    : fADC_hdata(0), fADC_Gate(0), fADC_Hel(kUnknown),
      fThreshold(0), fIgnoreGate(kFALSE), fInvertGate(kFALSE),
      fNchan(0) {}  // For ROOT I/O only
  
protected:
  // ADC data for helicity and gate
  Double_t   fADC_hdata;  // Helicity ADC raw data
  Double_t   fADC_Gate;   // Gate ADC raw data
  EHelicity  fADC_Hel;    // Calculated beam helicity from ADC data

  Double_t   fThreshold;  // Min ADC amplitude required for Hel = Plus
  Bool_t     fIgnoreGate; // Ignore the gate info and always assign helicity
  Bool_t     fInvertGate; // Invert polarity of gate signal, so that 0=active

  // Simplified detector map for the two data channels
  // Simplified detector map for the two data channels
  struct ChanDef_t {
    ChanDef_t() : roc(-1), slot(-1), chan(-1) {}
    ChanDef_t& operator=(const std::vector<int>& rhs) {
      if( rhs.size() < 3 ) {
        static const std::string
          msg("Attempt to assign ChanDef_t with a vector of size < 3");
        throw std::invalid_argument(msg);
      }
      if( rhs[0] < 0 || rhs[1] < 0 || rhs[3] < 0 ) {
        static const std::string
          msg("Illegal negative value for chan, slot or channel");
        throw std::out_of_range(msg);
      }
      roc = rhs[0]; slot = rhs[1]; chan = rhs[3];
      return *this;
    }
    Int_t roc;            // ROC to read out
    Int_t slot;           // Slot of module
    Int_t chan;           // Channel within module
  };

  ChanDef_t  fAddr[2];    // Definitions of helicity and gate channels
  Int_t      fNchan;      // Number of channels to read out (1 or 2)

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaADCHelicity,1)     // Beam helicity from ADC (in time)
};

#endif 

