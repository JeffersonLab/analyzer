////////////////////////////////////////////////////////////////////////
//
// GenHelicity
//
// Helicity of the beam.  See implementation for description.
// 
// author: R. Michaels, Sept 2002
// updated: Jan 2006 by V. Sulkosky
//
////////////////////////////////////////////////////////////////////////


#ifndef ROOT_GenHelicity
#define ROOT_GenHelicity

class THaEvData;

#include <iostream>
#include "TString.h"

class GenHelicity  {
  
public:

  GenHelicity();
  virtual ~GenHelicity();
  Int_t GetHelicity() const;   // To get the helicity flag (-1,0,+1)
                               // -1 = minus, +1 = plus, 0 = unknown.
  Int_t GetHelicity(const TString& spec) const;  // Get from spec "left" or "right"
  Double_t GetTime()       const;   // Returns timestamp (~105 kHz)

// Get methods for ROC 10/11 (R/L) helicity information for debugging
  Int_t GetQrt( const int spec=-1 ) const
    {
      if ((spec<0) || (spec>2)) return fQrt[fWhichSpec];
      else return fQrt[spec];
    }
  Int_t GetQuad( const int spec=-1 ) const
    {
      if ((spec<0) || (spec>2)) return fQuad[fWhichSpec];
      else return fQuad[spec];
    }
  Double_t GetTdiff( const int spec=-1 ) const
    {
      if ((spec<0) || (spec>2)) return fTdiff[fWhichSpec];
      else return fTdiff[spec];
    }
  Int_t GetGate( const int spec=-1 ) const 
    { 
      if ((spec<0) || (spec>2)) return fGate[fWhichSpec];
      return fGate[spec]; 
    }
  Int_t GetReading( const int spec=-1 ) const 
    { 
      if ((spec<0) || (spec>2)) return present_reading[fWhichSpec];
      return present_reading[spec]; 
    }
  Int_t GetValidTime( const int spec=-1 ) const 
    { 
      if ((spec<0) || (spec>2)) return validTime[fWhichSpec];
      return validTime[spec]; 
    }
  Int_t GetValidHelicity( const int spec=-1 ) const 
    { 
      if ((spec<0) || (spec>2)) return validHel[fWhichSpec];
      return validHel[spec]; 
    }

  Int_t GetNumRead()      const { return fNumread; } // Lastest valid reading
  Int_t GetBadRead()      const { return fBadread; } // Latest problematic reading
// Get methods for Ring Buffer scalers
  Int_t GetRingClk()      const { return fRing_clock; }
  Int_t GetRingQrt()      const { return fRing_qrt; }
  Int_t GetRingHelicity() const { return fRing_helicity; }
  Int_t GetRingTrig()     const { return fRing_trig; }
  Int_t GetRingBCM()      const { return fRing_bcm; }
  Int_t GetRingl1a()      const { return fRing_l1a; }
  Int_t GetRingV2fh()     const { return fRing_v2fh; }

  Int_t Decode( const THaEvData& evdata );
  void ClearEvent();
// The user may set the state of this class (see implementation)
  void SetState(int mode, int delay, int sign, int spec, int redund);
  void SetROC (int arm, int roc,
	       int helheader, int helindex,
	       int timeheader, int timeindex);

private:

  GenHelicity( const GenHelicity& ) {}
  GenHelicity& operator=( const GenHelicity& ) { return *this; }
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
  Bool_t* fT0T9; // Was fT0 computed using fT9?
  Double_t *fTlastquad, *fTtol;
  Int_t *fQrt, *fGate, *fFirstquad, *fEvtype;
  Int_t *fQuad;
  Double_t *fTimestamp;
  Double_t *fLastTimestamp;
  Double_t *fTimeLastQ1;
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

  Int_t fNumread, fBadread;
  // ring buffer scalers
  Int_t fRing_clock, fRing_qrt, fRing_helicity;
  Int_t fRing_trig, fRing_bcm, fRing_l1a, fRing_v2fh;

// ADC data for helicity and gate (redundant).
  Double_t  Ladc_helicity, Ladc_gate;
  Double_t  Radc_helicity, Radc_gate;
  Int_t  Ladc_hbit, Radc_hbit;

  Int_t     Lhel,Rhel,Hel;
  Double_t  timestamp;                         

  // ROC information

  // Index is 0, 1 for (nominally) left, right arm
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).

  Int_t fRoc[2];                 // ROC for left, right arm
  Int_t fHelHeader[2];           // Header for helicity bit
  Int_t fHelIndex[2];            // Index from header
  Int_t fTimeHeader[2];          // Header for timestamp
  Int_t fTimeIndex[2];           // Index from header

  static const Int_t OK       =  1;
  static const Int_t HCUT     = 5000;  
  static const Int_t Plus     =  1;    
  static const Int_t Minus    = -1;    
  static const Int_t Unknown  =  0;    
  static const Int_t HELDEBUG =  0;
  static const Int_t HELVERBOSE = 1;

  void   InitG0();
  void   InitMemory();

  ClassDef(GenHelicity,0)       // Beam helicity information.
};

#endif 
