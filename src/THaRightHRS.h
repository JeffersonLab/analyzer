#ifndef ROOT_THaRightHRS
#define ROOT_THaRightHRS

//////////////////////////////////////////////////////////////////////////
//
// THaRightHRS
//
// The standard Hall A right arm HRS.
// Special configurations of the HRS (e.g. more detectors, different 
// detectors) can be supported in on e of three ways:
//
//   1. Use the AddDetector() method to include a new detector
//      in this apparatus.  The detector will be decoded properly,
//      and its variables will be available for cuts and histograms.
//      Its processing methods will also be called by the generic Reconstruct()
//      algorithm implemented in THaSpectrometer::Reconstruct() and should
//      be correctly handled if the detector class follows the standard 
//      interface design.
//
//   2. Write a derived class that creates the detector in the
//      constructor.  Write a new Reconstruct() method or extend the existing
//      one if necessary.
//
//   3. Write a new class inheriting from THaSpectrometer, using this
//      class as an example.  This is appropriate if your HRS 
//      configuration has fewer or different detectors than the 
//      standard HRS. (It might not be sensible to provide a RemoveDetector() 
//      method since Reconstruct() relies on the presence of the 
//      standard detectors to some extent.)
//
//////////////////////////////////////////////////////////////////////////

#include "THaHRS.h"

class THaRightHRS : public THaHRS {
  
public:
  THaRightHRS( const char* description="" );
  virtual ~THaRightHRS() {}

  ClassDef(THaRightHRS,0)   //Hall A Right-arm High-Resolution Spectrometer
};

#endif

