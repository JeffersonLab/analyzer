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
  THaHRS( const char* name, const char* description );
  virtual ~THaHRS();

  virtual Int_t   FindVertices( TClonesArray& tracks );
  virtual Int_t   TrackCalc();

protected:

  ClassDef(THaHRS,0) //A Hall A High Resolution Spectrometer
};

#endif

