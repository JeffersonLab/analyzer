//////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  If the helicity flag arrives in-time, the flag
// is simply an ADC value (high value = plus, pedestal value = minus).  
// If the helicity flag is in delayed mode, it is more complicated.
// Then we must calibrate time and use the time stamp info to deduce the 
// helicity.  The code for the delayed mode is not written yet.
//
// Author:  R. Michaels  rom@jlab.org   (Aug 2000)
//
//////////////////////////////////////////////////////////////////////////

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
  Double_t GetTime() const { return timestamp; };   // 100 kHz clock
  Int_t Decode( const THaEvData& evdata );
  void   ClearEvent();

private:

  THaHelicity( const THaHelicity& ) {}
  THaHelicity& operator=( const THaHelicity& ) { return *this; }
  Double_t  Ladc_helicity[NCHAN];     
  Double_t  Radc_helicity[NCHAN];     
  Int_t     Lhel,Rhel,Hel;
  Double_t  timestamp;                         
  static const Int_t OK      =  1;
  static const Int_t HCUT    = 5000;  
  static const Int_t Plus    =  1;    
  static const Int_t Minus   = -1;    
  static const Int_t Unknown =  0;    
  static const Int_t DEBUG   =  0;
#ifndef STANDALONE
  THaDetMap*  fDetMap;
#endif
  Int_t  indices[2][2];
  void   Init();

  ClassDef(THaHelicity,0)       // Beam helicity information.
};

#endif 







