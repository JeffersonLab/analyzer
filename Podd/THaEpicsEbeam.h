#ifndef Podd_THaEpicsEbeam_h_
#define Podd_THaEpicsEbeam_h_

//////////////////////////////////////////////////////////////////////////
//
// THaEpicsEbeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaBeamModule.h"
#include "TString.h"

class THaEpicsEbeam : public THaPhysicsModule, public THaBeamModule {
  
public:
  THaEpicsEbeam( const char* name, const char* description,
		 const char* beam, const char* epics_var, 
		 Double_t scale_factor = 1.0 ) ;
  virtual ~THaEpicsEbeam();
  
  virtual void      Clear( Option_t* opt="" );

  // computed data
  Double_t          GetEcorr()   const { return fEcorr; }

  // module parameters
  Bool_t            EpicsIsMomentum() const { return fEpicsIsMomentum; }
  const char*       GetEpicsVar()     const { return fEpicsVar.Data(); }
  Double_t          GetScaleFactor()  const { return fScaleFactor; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetBeam( const char* beam );
          void      SetEpicsVar( const char* epics_var );
          void      SetEpicsIsMomentum( Bool_t mode=kTRUE );
          void      SetScaleFactor( Double_t fact );

protected:

  virtual Int_t  DefineVariables( EMode mode = kDefine );

  Double_t       fEcorr;       // Correction to beam energy (output-input) (GeV) 
  Bool_t         fEpicsIsMomentum; // If true, interpret EPICS data as momentum
  Double_t       fScaleFactor; // Scale factor for EPICS data (to conv to GeV)

  TString        fBeamName;    // Name of input beam module
  TString        fEpicsVar;    // Name of EPICS variable to use for beam energy
  THaBeamModule* fBeamModule;  // Pointer to input beam module

  ClassDef(THaEpicsEbeam,0)    // Beam module using beam energy from EPICS
};

#endif

