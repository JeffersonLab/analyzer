#ifndef ROOT_THaSpectrometerDetector
#define ROOT_THaSpectrometerDetector

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
// Abstract base class for a generic Hall A spectrometer detector. 
//
// This is a specialized detector class that supports the concept of
// "tracking" and "PID" detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"

class THaSpectrometerDetector : public THaDetector {
  
public:
  virtual ~THaSpectrometerDetector();
  
  virtual Bool_t   IsTracking() = 0;
  virtual Bool_t   IsPid()      = 0;

protected:

  //Only derived classes may construct me

  THaSpectrometerDetector() {}
  THaSpectrometerDetector( const char* name, const char* description,
			   THaApparatus* a = NULL );

  ClassDef(THaSpectrometerDetector,0)   //ABC for a spectrometer detector
};

#endif
