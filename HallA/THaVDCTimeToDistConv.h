#ifndef Podd_VDC_TimeToDistConv_h_
#define Podd_VDC_TimeToDistConv_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTimeToDistConv                                                      //
//                                                                           //
// Abstract base class for algorithms for converting TDC time into           //
// perpendicular  drift distance                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DataType.h"
#include <vector>

namespace VDC {

  class TimeToDistConv {

  public:
    explicit TimeToDistConv( UInt_t npar = 0 );
    TimeToDistConv( const TimeToDistConv& ) = default;
    TimeToDistConv( TimeToDistConv&& ) = default;
    TimeToDistConv& operator=( const TimeToDistConv& ) = default;
    TimeToDistConv& operator=( TimeToDistConv&& ) = default;
    virtual ~TimeToDistConv() = default;

    virtual Double_t ConvertTimeToDist( Double_t time, Double_t tanTheta,
					Double_t* ddist = nullptr ) const = 0;
    Double_t         GetDriftVel() const { return fDriftVel; }
    virtual Double_t GetParameter( UInt_t ) const { return kBig; }
    void             SetDriftVel( Double_t v );
    virtual Int_t    SetParameters( const std::vector<double>& );

protected:
    UInt_t   fNparam;     // Number of parameters
    Double_t fDriftVel;   // Drift velocity (m/s)
    Bool_t   fIsSet;      // Flag to indicate that all parameters are set

    ClassDef(TimeToDistConv,0)    // VDC time-to-distance converter
  };
}

////////////////////////////////////////////////////////////////////////////////

#endif
