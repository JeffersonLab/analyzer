////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  See implementation for description.
// 
// author: R. Michaels, Sept 2002
//
////////////////////////////////////////////////////////////////////////


#ifndef ROOT_THaHelicity
#define ROOT_THaHelicity

class THaEvData;

#define NCHAN 6
#include <iostream>
#ifndef STANDALONE
#include "THaDetMap.h"
#endif
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
  void   ClearEvent();

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

// The fgG0mode flag turns G0 mode on (1) or off (0)
// Variables in this section pertain to G0 helicity mode.
  static const Int_t fgG0mode = 1;
  static const Int_t fgG0delay = 8;  // delay of helicity (# windows)
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
  Int_t fArm;  // Which spectrometer = fgLarm or fgRarm

// The following variables used for data NOT in G0 helicity mode.
  Double_t  Ladc_helicity[NCHAN];     
  Double_t  Radc_helicity[NCHAN];     
  Int_t     Lhel,Rhel,Hel;
  Double_t  timestamp;                         
  static const Int_t OK       =  1;
  static const Int_t HCUT     = 5000;  
  static const Int_t Plus     =  1;    
  static const Int_t Minus    = -1;    
  static const Int_t Unknown  =  0;    
  static const Int_t HELDEBUG =  0;
// STANDALONE restriction is not too bad since it only affects
// old data prior to G0 mode.  But you can't run STANDALONE then.
#ifndef STANDALONE
  THaDetMap*  fDetMap;
#else
  Int_t fDetMap;  
#endif
  Int_t  indices[2][2];

  static const Int_t HELVERBOSE = 1;

  void   Init();

  ClassDef(THaHelicity,0)       // Beam helicity information.
};

#endif 







