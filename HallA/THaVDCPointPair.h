#ifndef Podd_THaVDCPointPair_h_
#define Podd_THaVDCPointPair_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPointPair                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaVDCCluster.h"   // for chi2_t
#include "DataType.h"        // for kBig

class THaVDCPoint;
class THaTrack;

class THaVDCPointPair : public TObject {

public:
  THaVDCPointPair( THaVDCPoint* lp, THaVDCPoint* up, Double_t spacing )
    : fLowerPoint(lp), fUpperPoint(up), fSpacing(spacing), fError(kBig),
      fStatus(0) {}
  THaVDCPointPair() = delete;
  virtual ~THaVDCPointPair() = default;

  void            Analyze();
  void            Associate( THaTrack* track );
  VDC::chi2_t     CalcChi2() const;
  virtual Int_t   Compare( const TObject* ) const;
  Double_t        GetError()   const { return fError; }
  THaVDCPoint*    GetLower()   const { return fLowerPoint; }
  THaVDCPoint*    GetUpper()   const { return fUpperPoint; }
  Double_t        GetSpacing() const { return fSpacing; }
  Int_t           GetStatus()  const { return fStatus; }
  THaTrack*       GetTrack()   const;
  Bool_t          HasUsedCluster() const;
  virtual Bool_t  IsSortable() const { return true; }
  virtual void    Print( Option_t* opt="" ) const;
//  void            Release();
  void            SetStatus( Int_t i ) { fStatus = i; }
  void            Use();

  static Double_t CalcError( THaVDCPoint* lowerPoint,
			     THaVDCPoint* upperPoint,
			     Double_t spacing );

  static Double_t GetProjectedDistance( THaVDCPoint* here,
					THaVDCPoint* there,
					Double_t spacing );

protected:

  THaVDCPoint*    fLowerPoint;  // Lower UV point
  THaVDCPoint*    fUpperPoint;  // Upper UV point
  Double_t        fSpacing;     // Spacing between lower and upper chambers [m]
  Double_t        fError;       // Goodness of match between the points
  Int_t           fStatus;      // Status flag

  ClassDef(THaVDCPointPair,0)     // Pair of lower/upper VDC points
};

//////////////////////////////////////////////////////////////////////////////

#endif
