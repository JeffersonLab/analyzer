#ifndef ROOT_THaVDCPointPair
#define ROOT_THaVDCPointPair

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPointPair                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaVDCCluster.h"   // for chi2_t

class THaVDCPoint;
class THaTrack;

class THaVDCPointPair : public TObject {

public:
  THaVDCPointPair( THaVDCPoint* lt, THaVDCPoint* ut ) :
    fLowerPoint(lt), fUpperPoint(ut), fError(1e307), fStatus(0) {}

  virtual ~THaVDCPointPair() {}

  void            Analyze( Double_t spacing );
  void            Associate( THaTrack* track );
  chi2_t          CalcChi2() const;
  virtual Int_t   Compare( const TObject* ) const;
  Double_t        GetError()   const { return fError; }
  THaVDCPoint*    GetLower()   const { return fLowerPoint; }
  THaVDCPoint*    GetUpper()   const { return fUpperPoint; }
  Int_t           GetStatus()  const { return fStatus; }
  THaTrack*       GetTrack()   const;
  virtual Bool_t  IsSortable() const { return kTRUE; }
  virtual void    Print( Option_t* opt="" ) const;
  void            Release();
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
  Double_t        fError;       // Goodness of match between the points
  Int_t           fStatus;      // Status flag

private:
  THaVDCPointPair();

  ClassDef(THaVDCPointPair,0)     // Pair of lower/upper VDC points
};

//////////////////////////////////////////////////////////////////////////////

#endif
