#ifndef ROOT_THaPhysElectronKine
#define ROOT_THaPhysElectronKine

//////////////////////////////////////////////////////////////////////////
//
// THaPhysElectronKine
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;

class THaPhysElectronKine : public THaPhysicsModule {
  
public:
  static const Double_t kMp; // Proton mass

  THaPhysElectronKine( const char* name, const char* description );
  virtual ~THaPhysElectronKine();
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();
          void      SetSpectrometer( TString& name );

protected:

  Double_t                fQ2;           // 4-momentum transfer squared (GeV^2)
  Double_t                fOmega;        // Energy transfer (GeV)
  Double_t                fW2;           // Invariant mass of recoil system (GeV^2)
  Double_t                fXbj;          // x Bjorken
  Double_t                fScatAngle;    // Scattering angle (rad)
  Double_t                fEpsilon;      // Virtual photon polarization factor
  Double_t                fQ3mag;        // Magnitude of 3-momentum transfer
  Double_t                fThetaQ;       // Theta of 3-momentum vector (rad)
  Double_t                fPhiQ;         // Phi of 3-momentum transfer (rad)

  Double_t                fMA;           // Effective mass of target

  TString                 fSpectroName;  // Name of spectrom. to consider
  const THaSpectrometer*  fSpectro;      // Pointer to spectrometer object

  THaPhysElectronKine() : fMA(kMp), fSpectro(NULL) {}
  THaPhysElectronKine( const THaPhysElectronKine& ) {}
  THaPhysElectronKine& operator=( const THaPhysElectronKine& ) 
    { return *this; }

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual void  MakePrefix() { THaAnalysisObject::MakePrefix( NULL ); }

  ClassDef(THaPhysElectronKine,0)   //Single arm kinematics module
};

//_________ inlines __________________________________________________________

inline void THaPhysElectronKine::SetSpectrometer( TString& name ) {
  fSpectroName = name; 
}

#endif
