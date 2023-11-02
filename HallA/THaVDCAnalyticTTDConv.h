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
    virtual ~AnalyticTTDConv() = default;

    Double_t ConvertTimeToDist( Double_t time, Double_t tanTheta,
                                Double_t* ddist = nullptr ) const override;
    Double_t GetParameter( UInt_t i ) const override;
    Int_t    SetParameters( const std::vector<double>& param ) override;

protected:

    // Coefficients for a polynomial yielding correction parameters
    Double_t fA1tdcCor[4];
    Double_t fA2tdcCor[4];

    Double_t fdtime;      // uncertainty in the measured time

    ClassDefOverride(AnalyticTTDConv,0)   // VDC Analytic TTD Conv class
  };
}

////////////////////////////////////////////////////////////////////////////////

#endif
