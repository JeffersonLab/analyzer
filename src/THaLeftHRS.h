#ifndef ROOT_THaLeftHRS
#define ROOT_THaLeftHRS

//////////////////////////////////////////////////////////////////////////
//
// THaLeftHRS
//
//////////////////////////////////////////////////////////////////////////

#include "THaHRS.h"

class THaLeftHRS : public THaHRS {

public:
  THaLeftHRS( const char* description="" );
  virtual ~THaLeftHRS() {}

  ClassDef(THaLeftHRS,0)   //Hall A Left-arm High-Resolution Spectrometer
};

#endif

