#ifndef Podd_THaTrackProj_h_
#define Podd_THaTrackProj_h_

//////////////////////////////////////////////////////////////////////////
//
// THaTrackProj
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaTrackProj : public TObject {

public:
  static const Double_t kBig;

  THaTrackProj( Double_t x=kBig, Double_t y=kBig, Double_t pathl=kBig,
		Double_t dx=kBig, Int_t ch=-1 )
    : fX(x), fY(y), fPathl(pathl), fdX(dx), fChannel(ch),
      fIsOK(x<0.5*kBig&&y<0.5*kBig) {}
  virtual ~THaTrackProj() {}

  virtual void Clear( Option_t* opt="" );
  virtual void Print( Option_t* opt="" ) const;

  Double_t  GetX()       const { return fX; }
  Double_t  GetY()       const { return fY; }
  Double_t  GetdX()      const { return fdX; }
  Double_t  GetPathLen() const { return fPathl; }
  Int_t     GetChannel() const { return fChannel; }
  Bool_t    IsOK()       const { return fIsOK; }

  void      Set( Double_t x, Double_t y, Double_t pathl, Double_t dx, Int_t ch )
  { fX = x; fY = y; fPathl = pathl; fdX = dx; fChannel = ch; fIsOK = true; }
  void      SetdX( Double_t dx )   { fdX = dx; }
  void      SetChannel( Int_t ch ) { fChannel = ch; }
  void      SetOK( Bool_t ok )     { fIsOK = ok; }

protected:
  Double_t          fX;              // x-coord of track crossing in detector coords (m)
  Double_t          fY;              // y-coord of track crossing in detector coords (m)
  Double_t          fPathl;          // Pathlength from tracking reference plane (m)
  Double_t          fdX;             // x-pos diff betw hit and and track proj (m)
  Int_t             fChannel;        // Detector element number (starting at 0)
  Bool_t            fIsOK;           // True if data valid (within defined detector area)

 public:
  ClassDef(THaTrackProj,2)    // Track coordinates projected to detector plane
};

#endif
