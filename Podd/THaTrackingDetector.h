#ifndef Podd_THaTrackingDetector_h_
#define Podd_THaTrackingDetector_h_

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingDetector.h
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"

class TClonesArray;
class THaTrack;
class THaTrackID;

class THaTrackingDetector : public THaSpectrometerDetector {
  
public:
  virtual ~THaTrackingDetector();
  
  virtual Int_t    CoarseTrack( TClonesArray& tracks ) = 0;
  virtual Int_t    FineTrack( TClonesArray& tracks )  = 0;
  // For backward-compatibility
  virtual Int_t    FindVertices( TClonesArray& /* tracks */ ) { return 0; }
          Bool_t   IsTracking() { return true; }
          Bool_t   IsPid()      { return false; }

	  THaTrackingDetector();   // for ROOT I/O only
protected:

  virtual THaTrack* AddTrack( TClonesArray& tracks,
			      Double_t x, Double_t y, 
			      Double_t theta, Double_t phi,
			      THaTrackID* ID = nullptr );

  //Only derived classes may construct me

  THaTrackingDetector( const char* name, const char* description,
		       THaApparatus* a = nullptr );

  ClassDef(THaTrackingDetector,1)   //ABC for a generic tracking detector
};

#endif
