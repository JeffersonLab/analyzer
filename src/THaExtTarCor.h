#ifndef ROOT_THaExtTarCor
#define ROOT_THaExtTarCor

//////////////////////////////////////////////////////////////////////////
//
// THaExtTarCor
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;
class THaVertexModule;
class THaTrackInfo;

class THaExtTarCor : public THaPhysicsModule {
  
public:
  THaExtTarCor( const char* name, const char* description,
		const char* spectro="", const char* vertex="" );
  virtual ~THaExtTarCor();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetDeltaP()  const { return fDeltaP; }
  Double_t          GetDeltaDp() const { return fDeltaDp; }
  Double_t          GetDeltaTh() const { return fDeltaTh; }
  THaTrackInfo*     GetTrackInfo() { return fTrkIfo; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetModuleNames( const char* spectro, const char* vertex="" );

protected:

  THaExtTarCor() : fThetaCorr(0.0), fDeltaCorr(0.0), fTrkIfo(NULL),
		   fSpectro(NULL), fVertexModule(NULL) {}
  THaExtTarCor( const THaExtTarCor& ) {}
  THaExtTarCor& operator=( const THaExtTarCor& ) { return *this; }

  Double_t                fThetaCorr;    // Theta correction coefficient
  Double_t                fDeltaCorr;    // Delta correction coefficient

  THaTrackInfo*           fTrkIfo;       // Corrected track data (single track)

  Double_t                fDeltaP;       // Size of momentum correction (GeV)
  Double_t                fDeltaDp;      // Size of delta correction
  Double_t                fDeltaTh;      // Size of angle corection (rad)

  TString                 fSpectroName;  // Name of spectrometer
  const THaSpectrometer*  fSpectro;      // Pointer to spectrometer object
  TString                 fVertexName;   // Name of vertex module
  const THaVertexModule*  fVertexModule; // Pointer to vertex module

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

  ClassDef(THaExtTarCor,1)   //Extended target corrections module
};

//_________ inlines __________________________________________________________
inline 
void THaExtTarCor::SetModuleNames( const char* spectro, const char* vertex ) 
{
  fSpectroName = spectro; 
  fVertexName  = vertex;
}

#endif
