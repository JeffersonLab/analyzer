#ifndef ROOT_THaVDCTimeToDistConv
#define ROOT_THaVDCTimeToDistConv

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTimeToDistConv                                                      //
//                                                                           //
// Base class for algorithms for converting TDC time into perpendicular      //
// drift distance                                                            //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaVDCTimeToDistConv : public TObject {

public:
  THaVDCTimeToDistConv() {}
  virtual ~THaVDCTimeToDistConv();

  virtual Double_t ConvertTimeToDist(Double_t time, Double_t tanTheta,
				     Double_t *ddist=0) = 0;

protected:

  THaVDCTimeToDistConv( const THaVDCTimeToDistConv& ) {}
  THaVDCTimeToDistConv& operator=( const THaVDCTimeToDistConv& ) { return *this; }

  ClassDef(THaVDCTimeToDistConv,0)             // VDCTimeToDistConv class
};

////////////////////////////////////////////////////////////////////////////////

#endif
