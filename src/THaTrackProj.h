#ifndef ROOT_THaTrackProj
#define ROOT_THaTrackProj

//////////////////////////////////////////////////////////////////////////
//
// THaTrackProj
//     Holds the track-derived information, projecting to the non-tracking
//     detectors 
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class THaNonTrackingDetector;

class THaTrackProj : public TObject {

public:

  static const Double_t kBig;
  
  THaTrackProj( Double_t x=kBig, Double_t y=kBig, Double_t pathl=kBig,
		Double_t dx=kBig, Int_t ch=0,
		THaNonTrackingDetector *parent=0);
  
  virtual ~THaTrackProj();
  Double_t  GetX() const { return fX; }
  Double_t  GetY() const { return fY; }
  Double_t  GetdX() const { return fdX; }
  Double_t  GetPathLen() const { return fPathl; }
  Int_t     GetChannel() const { return fChannel; }
  THaNonTrackingDetector* GetParent() const { return fParent; }
  
protected:

  // Focal plane coordinates (TRANSPORT system projected to z=0)
  Double_t          fX;              // x position of track at plane (m)
  Double_t          fY;              // y position of track at plane (m)
  Double_t          fPathl;          // pathlength from ref. plane (m)
  Double_t          fdX;             // position difference bt hit and track proj.
  Int_t             fChannel;        // primary paddle (starting at 0)
  THaNonTrackingDetector *fParent;   // detector that contains hit

 public:
  ClassDef(THaTrackProj,1)    // track projected to detector plane
};


#endif
