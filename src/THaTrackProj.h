#ifndef ROOT_THaTrackProj
#define ROOT_THaTrackProj

//////////////////////////////////////////////////////////////////////////
//
// THaTrackProj
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class THaNonTrackingDetector;

class THaTrackProj : public TObject {

public:
  static const Double_t kBig;

  THaTrackProj( Double_t x, Double_t y, Double_t pathl,
		Double_t dx=kBig, Int_t ch=-1 )
    : fX(x), fY(y), fPathl(pathl), fdX(dx), fChannel(ch), fIsOK(true) {}
  THaTrackProj() : fIsOK(false) {}
  virtual ~THaTrackProj() {}

  Double_t  GetX() const { return fX; }
  Double_t  GetY() const { return fY; }
  Double_t  GetdX() const { return fdX; }
  Double_t  GetPathLen() const { return fPathl; }
  Int_t     GetChannel() const { return fChannel; }
  Bool_t    IsOK()  const { return fIsOK; }

  void      Set( Double_t x, Double_t y, Double_t pathl, Double_t dx, Int_t ch )
  { fX = x; fY = y; fPathl = pathl; fdX = dx; fChannel = ch; fIsOK = true; }
  void      SetdX( Double_t dx )   { fdX = dx; }
  void      SetChannel( Int_t ch ) { fChannel = ch; }
  void      SetOK( Bool_t ok )     { fIsOK = ok; }

protected:
  Double_t          fX;              // x position of track at plane (m)
  Double_t          fY;              // y position of track at plane (m)
  Double_t          fPathl;          // pathlength from ref. plane (m)
  Double_t          fdX;             // position difference bt hit and track proj.
  Int_t             fChannel;        // detector element number (starting at 0)
  Bool_t            fIsOK;           // True if data valid

 public:
  ClassDef(THaTrackProj,2)    // Track coordinates projected to detector plane
};

#endif
