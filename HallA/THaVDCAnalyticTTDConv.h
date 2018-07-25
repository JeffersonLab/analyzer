#ifndef Podd_VDC_AnalyticTTDConv_h_
#define Podd_VDC_AnalyticTTDConv_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCAnalyticTTDConv                                                     //
//                                                                           //
// Uses a drift velocity (um/ns) to convert time (ns) into distance (cm)     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCTimeToDistConv.h"

namespace VDC {

  class AnalyticTTDConv : public TimeToDistConv {

  public:
    AnalyticTTDConv();
    virtual ~AnalyticTTDConv() {}

    virtual Double_t ConvertTimeToDist( Double_t time, Double_t tanTheta,
				        Double_t* ddist=0 ) const;
    virtual Double_t GetParameter( UInt_t i ) const;
    virtual Int_t    SetParameters( const std::vector<double>& param );

protected:

    // Coefficients for a polynomial yielding correction parameters
    Double_t fA1tdcCor[4];
    Double_t fA2tdcCor[4];

    Double_t fdtime;      // uncertainty in the measured time

    ClassDef(AnalyticTTDConv,0)   // VDC Analytic TTD Conv class
  };
}

////////////////////////////////////////////////////////////////////////////////

#endif
