#ifndef ROOT_THaElectronKine
#define ROOT_THaElectronKine

//////////////////////////////////////////////////////////////////////////
//
// THaElectronKine
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;

class THaElectronKine : public THaPhysicsModule {
  
public:
  THaElectronKine( const char* name, const char* description,
		   const char* spectro = "", Double_t mass = 0.0 );
  virtual ~THaElectronKine();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetQ2()         const { return fQ2; }
  Double_t          GetOmega()      const { return fOmega; }
  Double_t          GetNu()         const { return fOmega; }
  Double_t          GetW2()         const { return fW2; }
  Double_t          GetXbj()        const { return fXbj; }
  Double_t          GetScatAngle()  const { return fScatAngle; }
  Double_t          GetEpsilon()    const { return fEpsilon; }
  Double_t          GetQ3mag()      const { return fQ3mag; }
  Double_t          GetThetaQ()     const { return fThetaQ; }
  Double_t          GetPhiQ()       const { return fPhiQ; }
  Double_t          GetTargetMass() const { return fMA; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetSpectrometer( const char* name );

protected:

  Double_t          fQ2;           // 4-momentum transfer squared (GeV^2)
  Double_t          fOmega;        // Energy transfer (GeV)
  Double_t          fW2;           // Invariant mass of recoil system (GeV^2)
  Double_t          fXbj;          // x Bjorken
  Double_t          fScatAngle;    // Scattering angle (rad)
  Double_t          fEpsilon;      // Virtual photon polarization factor
  Double_t          fQ3mag;        // Magnitude of 3-momentum transfer
  Double_t          fThetaQ;       // Theta of 3-momentum vector (rad)
  Double_t          fPhiQ;         // Phi of 3-momentum transfer (rad)

  Double_t          fMA;           // Effective mass of target

  THaElectronKine() : fMA(0.0), fSpectro(NULL) {}
  THaElectronKine( const THaElectronKine& ) {}
  THaElectronKine& operator=( const THaElectronKine& ) 
    { return *this; }

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

  TString                 fSpectroName;  // Name of spectrom. to consider
  const THaSpectrometer*  fSpectro;      // Pointer to spectrometer object

  ClassDef(THaElectronKine,0)   //Single arm kinematics module
};

//_________ inlines __________________________________________________________

inline void THaElectronKine::SetSpectrometer( const char* name ) {
  fSpectroName = name; 
}

#endif
