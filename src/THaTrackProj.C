//////////////////////////////////////////////////////////////////////////
//
// THaTrackProj
//    - reconstructed information for a track hit in a timing detector
//////////////////////////////////////////////////////////////////////////

#include "THaTrackProj.h"

const Double_t THaTrackProj::kBig = 1.e38;

THaTrackProj::THaTrackProj( Double_t x, Double_t y,
			    Double_t pathl,
			    Double_t dx, Int_t ch,
			    THaNonTrackingDetector *parent):
  fX(x), fY(y), fPathl(pathl), fdX(dx), fChannel(ch),
  fParent(parent) { }

THaTrackProj::~THaTrackProj() { }

ClassImp(THaTrackProj)

