#ifndef ROOT_THaHRS
#define ROOT_THaHRS

//////////////////////////////////////////////////////////////////////////
//
// THaHRS
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometer.h"

class THaScintillator;

class THaHRS : public THaSpectrometer {
  
public:
  THaHRS( const char* name, const char* description );
  virtual ~THaHRS();

  virtual Int_t   FindVertices( TClonesArray& tracks );
  virtual Int_t   TrackCalc();
  virtual Int_t   TrackTimes( TClonesArray* tracks );
  
protected:
  THaScintillator *sc_ref;  // calculate time track hits this plane
  
  ClassDef(THaHRS,0) //A Hall A High Resolution Spectrometer
};

#endif

