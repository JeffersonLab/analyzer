#ifndef Podd_VDC_TimeToDistConv_h_
#define Podd_VDC_TimeToDistConv_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTimeToDistConv                                                      //
//                                                                           //
// Base class for algorithms for converting TDC time into perpendicular      //
// drift distance                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <vector>

namespace VDC {

  extern const Double_t kBig;

  class TimeToDistConv {

  public:
    TimeToDistConv();
    virtual ~TimeToDistConv() {}

    virtual Double_t ConvertTimeToDist( Double_t time, Double_t tanTheta,
					Double_t* ddist = 0 ) const = 0;
    Double_t         GetDriftVel() { return fDriftVel; }
    virtual Double_t GetParameter( UInt_t ) const { return kBig; }
    void             SetDriftVel( Double_t v );
    virtual Int_t    SetParameters( const std::vector<double>& );

protected:
    TimeToDistConv( UInt_t npar = 0 );

    UInt_t   fNparam;     // Number of parameters
    Double_t fDriftVel;   // Drift velocity (m/s)
    Bool_t   fIsSet;      // Flag to indicate that all parameters are set

    ClassDef(TimeToDistConv,0)    // VDC time-to-distance converter
  };
}

////////////////////////////////////////////////////////////////////////////////

#endif
