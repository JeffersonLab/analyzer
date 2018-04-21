#ifndef Podd_THaExtTarCor_h_
#define Podd_THaExtTarCor_h_

//////////////////////////////////////////////////////////////////////////
//
// THaExtTarCor
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaTrackingModule.h"
#include "TString.h"

class THaVertexModule;

class THaExtTarCor : public THaPhysicsModule, public THaTrackingModule {
  
public:
  THaExtTarCor( const char* name, const char* description,
		const char* spectro="", const char* vertex="" );
  virtual ~THaExtTarCor();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetDeltaP()  const { return fDeltaP; }
  Double_t          GetDeltaDp() const { return fDeltaDp; }
  Double_t          GetDeltaTh() const { return fDeltaTh; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetModuleNames( const char* spectro, const char* vertex="" );

protected:

  Double_t                fThetaCorr;    // Theta correction coefficient
  Double_t                fDeltaCorr;    // Delta correction coefficient

  Double_t                fDeltaP;       // Size of momentum correction (GeV)
  Double_t                fDeltaDp;      // Size of delta correction
  Double_t                fDeltaTh;      // Size of angle corection (rad)

  TString                 fSpectroName;  // Name of spectrometer/tracking module
  TString                 fVertexName;   // Name of vertex module
  THaTrackingModule*      fTrackModule;  // Pointer to tracking module
  THaVertexModule*        fVertexModule; // Pointer to vertex module

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

  ClassDef(THaExtTarCor,0)   //Extended target corrections module
};

//_________ inlines __________________________________________________________
inline 
void THaExtTarCor::SetModuleNames( const char* spectro, const char* vertex ) 
{
  fSpectroName = spectro; 
  fVertexName  = vertex;
}

#endif
