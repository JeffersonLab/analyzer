////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  See implementation for description.
// 
// author: R. Michaels, Sept 2002
// updated: Sept 2004
//
////////////////////////////////////////////////////////////////////////


#ifndef ROOT_THaHelicity
#define ROOT_THaHelicity

class THaEvData;

#include <iostream>
#include "TString.h"

class THaHelicity  {
  
public:

  THaHelicity();
  virtual ~THaHelicity();
  Int_t GetHelicity() const;   // To get the helicity flag (-1,0,+1)
                               // -1 = minus, +1 = plus, 0 = unknown.
  Int_t GetHelicity(const TString& spec) const;  // Get from spec "left" or "right"
  Double_t GetTime() const;   // Returns timestamp (~105 kHz)
  Int_t Decode( const THaEvData& evdata );
  void ClearEvent();
// The user may set the state of this class (see implementation)
  void SetState(int mode, int delay, int sign, int spec, int redund);

private:

  THaHelicity( const THaHelicity& ) {}
  THaHelicity& operator=( const THaHelicity& ) { return *this; }
  void ReadData ( const THaEvData& evdata);
  void QuadCalib();
  void LoadHelicity();
  void QuadHelicity(Int_t cond=0);
  Int_t RanBit(Int_t i);
  UInt_t GetSeed();
  Bool_t CompHel();

// These variables define the state of this object ---------
// The fgG0mode flag turns G0 mode on (1) or off (0)
// G0 mode is the delayed helicity mode, else it is in-time.
  Int_t fgG0mode;
  Int_t fgG0delay;  // delay of helicity (# windows)
// Overall sign (as determined by Moller)
  Int_t fSign;
// Which spectrometer do we believe ?
  Int_t fWhichSpec;  // = fgLarm or fgRarm
// Check redundancy ? (yes=1, no=0)
  Int_t fCheckRedund;
// ---- end of state variables -----------------

  static const Int_t fgNbits = 24;   
  static const Int_t fgLarm = 0;
  static const Int_t fgRarm = 1;
  static const Double_t fgTdiff;
  Double_t *fTdavg, *fTdiff, *fT0, *fT9;
  Double_t *fTlastquad, *fTtol;
  Int_t *fQrt, *fGate, *fFirstquad, *fEvtype;
  Double_t *fTimestamp;
  Int_t *validTime, *validHel;
  Int_t *t9count;
  Int_t *present_reading, *predicted_reading; 
  Int_t *q1_reading, *present_helicity, *saved_helicity;
  Int_t *q1_present_helicity, *quad_calibrated;
  Int_t *hbits;
  UInt_t *fNqrt;
  Int_t recovery_flag;
  int nb;
  UInt_t iseed, iseed_earlier, inquad;
  Int_t fArm;  // Spectrometer = fgLarm or fgRarm

// ADC data for helicity and gate (redundant).
  Double_t  Ladc_helicity, Ladc_gate;
  Double_t  Radc_helicity, Radc_gate;
  Int_t  Ladc_hbit, Radc_hbit;

  Int_t     Lhel,Rhel,Hel;
  Double_t  timestamp;                         
  static const Int_t OK       =  1;
  static const Int_t HCUT     = 5000;  
  static const Int_t Plus     =  1;    
  static const Int_t Minus    = -1;    
  static const Int_t Unknown  =  0;    
  static const Int_t HELDEBUG =  0;
  static const Int_t HELVERBOSE = 1;

  void   InitG0();
  void   InitMemory();

  ClassDef(THaHelicity,0)       // Beam helicity information.
};

#endif 







