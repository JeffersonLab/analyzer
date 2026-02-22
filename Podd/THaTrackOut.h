#ifndef Podd_THaTRACKOUT_h_
#define Podd_THaTRACKOUT_h_

///////////////////////////////////////////////////////////////////////////
//
//  Class to put the four-vector for tracks from a THaTrackingModule
//
///////////////////////////////////////////////////////////////////////////
#include "THaPhysicsModule.h"

#include <TLorentzVector.h>
#include <TString.h>

class THaTrackingModule;

class THaTrackOut : public THaPhysicsModule {
 public:
  THaTrackOut(const char* name, const char* description,
	      const char* src="", Double_t pmass=0.0 /* GeV */ );
  ~THaTrackOut() override;

  void         Clear( Option_t* opt="" ) override;
  EStatus      Init( const TDatime& run_time ) override;
  Int_t        InitOutput( THaOutput* output ) override;
  void         SetMass ( Double_t m );
  void         SetSpectrometer( const char* name );
  Int_t        Process( const THaEvData& evdata ) override;
	  
 protected:
  Double_t             fM;          // Mass of detected particle
  TString              fSrcName;    // Name of module providing the track
  
  TLorentzVector*      fP4;         // momentum four-vector
  
  THaTrackingModule*   fSrc;        // Pointer to trackingmodule
  
 public:
  ClassDefOverride(THaTrackOut,0)   // lorentz-vector output module
};

#endif
