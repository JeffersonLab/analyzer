#ifndef ROOT_THaTrackInfo
#define ROOT_THaTrackInfo

//////////////////////////////////////////////////////////////////////////
//
// THaTrackInfo
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"
class THaTrack;

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
    fP(p), fDp(dp), fX(x), fY(y), fTheta(th), fPhi(ph), fPvect(pvect), fOK(1) 
  {}

  THaTrackInfo( const THaTrackInfo& t ) :
    fP(t.fP), fDp(t.fDp), fX(t.fX), fY(t.fY), fTheta(t.fTheta), 
    fPhi(t.fPhi), fPvect(t.fPvect), fOK(t.fOK) {}
  THaTrackInfo& operator=( const THaTrackInfo& );
  THaTrackInfo& operator=( const THaTrack& );
  virtual ~THaTrackInfo() {}

  void      Clear( Option_t* opt="" ) { 
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

  Double_t  fP;         // Track momentum (GeV)
  Double_t  fDp;        // TRANSPORT delta
  Double_t  fX;         // TRANSPORT x (m)
  Double_t  fY;         // TRANSPORT y (m)
  Double_t  fTheta;     // Tangent of TRANSPORT theta
  Double_t  fPhi;       // Tangent of TRANSPORT phi
  TVector3  fPvect;     // Momentum vector in lab (GeV)
  Int_t     fOK;        // Data ok (0:no 1:yes)

private:
  static const Double_t kBig;

  ClassDef(THaTrackInfo,1)  // Track information
};

#endif
