#ifndef ROOT_THaExtTarCor
#define ROOT_THaExtTarCor

//////////////////////////////////////////////////////////////////////////
//
// THaExtTarCor
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"
#include "TVector3.h"

class THaTrackInfo : public TObject {
public:
  THaTrackInfo() : fP(kBig), fDp(kBig), fX(kBig), fY(kBig), fTheta(kBig), 
    fPhi(kBig), fPvect(kBig,kBig,kBig), fOK(0) {}

  THaTrackInfo( Double_t p, Double_t dp, Double_t x, Double_t y, Double_t th,
		Double_t ph, Double_t px, Double_t py, Double_t pz ) :
    fP(p), fDp(dp), fX(x), fY(y), fTheta(th), fPhi(ph), fOK(1)
  { fPvect.SetXYZ(px,py,pz); }

  THaTrackInfo( Double_t p, Double_t dp, Double_t x, Double_t y, Double_t th,
		Double_t ph, const TVector3& pvect ) :
    fP(p), fDp(dp), fX(x), fY(y), fTheta(th), fPhi(ph), fPvect(pvect), fOK(1) {}

  virtual ~THaTrackInfo() {}

  void      Clear() { 
    fPhi = fTheta = fY = fX = fDp = fP = kBig; fPvect.SetXYZ(kBig,kBig,kBig); 
    fOK = 0; 
  }

  Double_t  GetPx() const { return fPvect.X(); }
  Double_t  GetPy() const { return fPvect.Y(); }
  Double_t  GetPz() const { return fPvect.Z(); }

  void Set( Double_t p, Double_t dp, Double_t x, Double_t y, Double_t th,
	    Double_t ph, Double_t px, Double_t py, Double_t pz )
  { 
    fP = p; fDp = dp; fX = x; fY = y; fTheta = th; fPhi = ph; 
    fPvect.SetXYZ(px,py,pz); fOK = 1; 
  }
  void Set( Double_t p, Double_t dp, Double_t x, Double_t y, Double_t th,
	    Double_t ph, const TVector3& pvect )
  { 
    fP = p; fDp = dp; fX = x; fY = y; fTheta = th; fPhi = ph; fPvect = pvect; 
    fOK = 1;
  }

  Double_t  fP;
  Double_t  fDp;
  Double_t  fX;
  Double_t  fY;
  Double_t  fTheta;
  Double_t  fPhi;
  TVector3  fPvect;
  Int_t     fOK;

private:
  static const Double_t kBig;

  ClassDef(THaTrackInfo,1)  // Track information
};

class THaSpectrometer;
class THaVertexModule;

class THaExtTarCor : public THaPhysicsModule {
  
public:
  THaExtTarCor( const char* name, const char* description,
		const char* spectro="", const char* vertex="" );
  virtual ~THaExtTarCor();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetDeltaP()  const { return fDeltaP; }
  Double_t          GetDeltaDp() const { return fDeltaDp; }
  Double_t          GetDeltaTh() const { return fDeltaTh; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();
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
  virtual Int_t ReadRunDatabase( FILE* file, const TDatime& date );

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
