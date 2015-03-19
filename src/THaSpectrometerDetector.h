#ifndef ROOT_THaSpectrometerDetector
#define ROOT_THaSpectrometerDetector

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
// Abstract base class for a generic spectrometer detector.
//
// This is a specialized detector class that supports the concept of
// "tracking" and "PID" detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"

class THaTrack;

class THaSpectrometerDetector : public THaDetector {

public:
  virtual ~THaSpectrometerDetector();

  virtual Bool_t   IsTracking() = 0;
  virtual Bool_t   IsPid()      = 0;

          bool     CheckIntercept( THaTrack* track );
          bool     CalcInterceptCoords( THaTrack* track,
					Double_t& x, Double_t& y );
          bool     CalcPathLen( THaTrack* track, Double_t& t );

  THaSpectrometerDetector();       // for ROOT I/O only

protected:

          bool  CalcTrackIntercept( THaTrack* track, Double_t& t,
				    Double_t& ycross, Double_t& xcross);

  //Only derived classes may construct me
  THaSpectrometerDetector( const char* name, const char* description,
			   THaApparatus* a = NULL );

  ClassDef(THaSpectrometerDetector,1)   //ABC for a spectrometer detector
};

#endif
