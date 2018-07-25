#ifndef Podd_THaPhotoReaction_h_
#define Podd_THaPhotoReaction_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPhotoReaction
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TLorentzVector.h"
#include "TString.h"

class THaTrackingModule;
class THaBeamModule;

class THaPhotoReaction : public THaPhysicsModule {
  
public:
  THaPhotoReaction( const char* name, const char* description,
		    const char* spectro = "");
  //                Double_t target_mass = 0.0 /* GeV/c2 */ );
  THaPhotoReaction( const char* name, const char* description,
		    const char* spectro, const char* beam );
  //                Double_t target_mass = 0.0 /* GeV/c2 */ );
  virtual ~THaPhotoReaction();
  
  virtual void      Clear( Option_t* opt="" );

  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
  //          void        SetTargetMass( Double_t m );
  void              SetSpectrometer( const char* name );
  void              SetBeam( const char* name );

protected:

  TLorentzVector    fP1;           // Detected proton 4-momentum
  Double_t          fEGamma;       // Energy of incident REAL photon (GeV)
  Double_t          fScatAngle;    // Proton scattering angle (rad)
  Double_t          fScatAngleCM;  // Proton CM scattering angle (rad)

  Double_t          fMA;           // Target mass (GeV/c^2)

  virtual Int_t     DefineVariables( EMode mode = kDefine );
  //  virtual Int_t     ReadRunDatabase( const TDatime& date );

  TString                 fSpectroName;  // Name of module providing tracks
  TString                 fBeamName;     // Name of module providing beam info
  THaTrackingModule*      fSpectro;      // Pointer to tracking module
  THaBeamModule*          fBeam;         // Pointer to beam module

  ClassDef(THaPhotoReaction,0)  //deuterium photodisintegration kinematics
};

#endif
