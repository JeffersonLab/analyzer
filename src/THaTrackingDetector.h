#ifndef ROOT_THaTrackingDetector
#define ROOT_THaTrackingDetector

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingDetector.h
//
// Abstract base class for a generic Hall A tracking detector. 
//
// This is a special THaSpectrometerDetector that is capable of
// finding particle tracks used for momentum analysis and target 
// reconstruction. This is usually a VDC.
//
// Note: The FPP is NOT a "tracking detector" in this scheme because,
// with the present FPP algorithms, it is not used for calculating the 
// fp coordinates. 
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"

class TClonesArray;

class THaTrackingDetector : public THaSpectrometerDetector {
  
public:
  virtual ~THaTrackingDetector();
  
  virtual Int_t    CoarseTrack( TClonesArray& tracks ) = 0;
  virtual Int_t    FineTrack( TClonesArray& tracks )  = 0;
          Bool_t   IsTracking() { return kTRUE; }
          Bool_t   IsPid()      { return kFALSE; }

protected:

  virtual Int_t    AddTrack( TClonesArray& tracks,
			     Double_t p, Double_t theta, Double_t phi,
			     Double_t x, Double_t y,
			     const TClonesArray* clusters = 0 );

  //Only derived classes may construct me

  THaTrackingDetector() {}
  THaTrackingDetector( const char* name, const char* description,
		       THaApparatus* a = NULL );

  ClassDef(THaTrackingDetector,0)   //ABC for a Hall A tracking detector
};

#endif
