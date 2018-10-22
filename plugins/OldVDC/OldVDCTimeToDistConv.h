#ifndef Podd_OldVDCTimeToDistConv_h_
#define Podd_OldVDCTimeToDistConv_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCTimeToDistConv                                                      //
//                                                                           //
// Base class for algorithms for converting TDC time into perpendicular      //
// drift distance                                                            //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class OldVDCTimeToDistConv : public TObject {

public:
  OldVDCTimeToDistConv() {}
  virtual ~OldVDCTimeToDistConv();

  virtual Double_t ConvertTimeToDist(Double_t time, Double_t tanTheta,
				     Double_t *ddist=0) = 0;
private:

  OldVDCTimeToDistConv( const OldVDCTimeToDistConv& );
  OldVDCTimeToDistConv& operator=( const OldVDCTimeToDistConv& );

  ClassDef(OldVDCTimeToDistConv,0)             // VDCTimeToDistConv class
};

////////////////////////////////////////////////////////////////////////////////

#endif
