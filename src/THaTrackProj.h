#ifndef ROOT_THaTrackProj
#define ROOT_THaTrackProj

//////////////////////////////////////////////////////////////////////////
//
// THaTrackProj
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class THaTrackProj : public TObject {

public:
  static const Double_t kBig;

  THaTrackProj( Double_t x=kBig, Double_t y=kBig, Double_t pathl=kBig,
		Double_t dx=kBig, Int_t ch=-1 )
    : fX(x), fY(y), fXAbs(kBig), fYAbs(kBig), fPathl(pathl), fdX(dx), fdY(kBig),
      fChannel(ch), fIsOK(false) {}
  virtual ~THaTrackProj() {}

  Double_t  GetX()       const { return fX; }
  Double_t  GetY()       const { return fY; }
  Double_t  GetXAbs()    const { return fXAbs; }
  Double_t  GetYAbs()    const { return fYAbs; }
  Double_t  GetdX()      const { return fdX; }
  Double_t  GetdY()      const { return fdY; }
  Double_t  GetPathLen() const { return fPathl; }
  Int_t     GetChannel() const { return fChannel; }
  Bool_t    IsOK()       const { return fIsOK; }

  void      Set( Double_t x, Double_t y, Double_t pathl, Double_t dx, Int_t ch )
  { fX = x; fY = y; fPathl = pathl; fdX = dx; fChannel = ch; fIsOK = true; }
  void      Set( Double_t x, Double_t y, Double_t xabs, Double_t yabs,
		 Double_t pathl, Double_t dx, Double_t dy, Int_t ch )
  { fX = x; fY = y; fXAbs = xabs; fYAbs = yabs; fPathl = pathl;
    fdX = dx; fdY = dy, fChannel = ch; fIsOK = true; }
  void      SetdX( Double_t dx )   { fdX = dx; }
  void      SetdY( Double_t dy )   { fdY = dy; }
  void      SetChannel( Int_t ch ) { fChannel = ch; }
  void      SetOK( Bool_t ok )     { fIsOK = ok; }
  void      SetOffset( const TVector3& origin )
  { fXAbs = fX+origin.X(); fYAbs = fY+origin.Y(); }

protected:
  Double_t          fX;              // Track x at plane relative to plane origin (m)
  Double_t          fY;              // Track y at plane relative to plane origin (m)
  Double_t          fXAbs;           // Track x at plane in tracking system coords (m)
  Double_t          fYAbs;           // Track y at plane in tracking system coords (m)
  Double_t          fPathl;          // Pathlength from tracking reference plane (m)
  Double_t          fdX;             // x-pos diff betw det elem center and track (m)
  Double_t          fdY;             // y-pos diff betw det elem center and track (m)
  Int_t             fChannel;        // Detector element number (starting at 0)
  Bool_t            fIsOK;           // True if data valid (within defined detector area)

 public:
  ClassDef(THaTrackProj,3)    // Track coordinates projected to detector plane
};

#endif
