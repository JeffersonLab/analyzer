#ifndef ROOT_THaHRS
#define ROOT_THaHRS

//////////////////////////////////////////////////////////////////////////
//
// THaHRS
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometer.h"

class THaHRS : public THaSpectrometer {
  
public:
  virtual ~THaHRS() {}

  virtual Int_t   FindVertices( TClonesArray& tracks );
  virtual Int_t   TrackCalc();

protected:
  THaHRS( const char* name, const char* description )
    : THaSpectrometer( name, description ) {}

  ClassDef(THaHRS,0) //ABC for the Hall A High-Resolution Spectrometers
};

#endif

