#ifndef ROOT_THaEpicsEbeam
#define ROOT_THaEpicsEbeam

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
		 const char* beam, const char* epics_var ) ;
  virtual ~THaEpicsEbeam();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetEcorr()   const { return fEcorr; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetBeam( const char* beam );
          void      SetEpicsVar( const char* epics_var );
          void      SetEpicsIsMomentum( Bool_t mode=kTRUE );

protected:

  virtual Int_t  DefineVariables( EMode mode = kDefine );

  Double_t       fEcorr;       // Correction to beam energy (output-input) (GeV) 
  Bool_t         fEpicsIsMomentum; // If true, interpret EPICS data as momentum

  TString        fBeamName;    // Name of input beam module
  TString        fEpicsVar;    // Name of EPICS variable to use for beam energy
  THaBeamModule* fBeamModule;  // Pointer to input beam module

  ClassDef(THaEpicsEbeam,0)    // ABC for an apparatus providing beam information
};

#endif

