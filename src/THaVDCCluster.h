#ifndef ROOT_THaVDCCluster
#define ROOT_THaVDCCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include <utility>

class THaVDCHit;
class THaVDCPlane;
class THaVDCPointPair;
class THaTrack;

typedef std::pair<Double_t,Int_t>  chi2_t;
inline chi2_t operator+( chi2_t a, const chi2_t& b ) {
  a.first  += b.first;
  a.second += b.second;
  return a;
};

typedef THaVDCPointPair VDCpp_t;

class THaVDCCluster : public TObject {

public:
  THaVDCCluster( THaVDCPlane* owner = 0 ) :
    fSize(0), fPlane(owner), fPointPair(0), fTrack(0), fTrkNum(0),
    fSlope(kBig), fLocalSlope(kBig), fSigmaSlope(kBig),
    fInt(kBig), fSigmaInt(kBig), fT0(kBig), fSigmaT0(kBig),
    fPivot(0), fIPivot(-1), fTimeCorrection(0),
    fFitOK(false), fChi2(kBig), fNDoF(0.0), fClsBeg(-1), fClsEnd(-1)
    {}
  virtual ~THaVDCCluster() {}

  enum EMode { kSimple, kT0, kFull };

  virtual void   AddHit(THaVDCHit * hit);
  virtual void   EstTrackParameters();
  virtual void   ConvertTimeToDist();
  virtual void   FitTrack( EMode mode = kSimple );
  virtual void   ClearFit();
  virtual void   CalcChisquare(Double_t& chi2, Int_t& nhits) const;
  chi2_t         CalcDist();    // calculate global track to wire distances

  // TObject functions redefined
  virtual void   Clear( Option_t* opt="" );
  virtual Int_t  Compare( const TObject* obj ) const;
  virtual Bool_t IsSortable() const        { return kTRUE; }
  virtual void   Print( Option_t* opt="" ) const;

  //Get and Set Functions
  THaVDCHit**    GetHits()                 { return fHits; } // Get array of pointers
  THaVDCHit *    GetHit(Int_t i)     const { return fHits[i]; }
  THaVDCPlane*   GetPlane()          const { return fPlane; }
  Int_t          GetSize ()          const { return fSize; }
  Double_t       GetSlope()          const { return fSlope; }
  Double_t       GetLocalSlope()     const { return fLocalSlope; }
  Double_t       GetSigmaSlope()     const { return fSigmaSlope; }
  Double_t       GetIntercept()      const { return fInt; }
  Double_t       GetSigmaIntercept() const { return fSigmaInt; }
  THaVDCHit*     GetPivot()          const { return fPivot; }
  Int_t          GetPivotWireNum()   const;
  Double_t       GetTimeCorrection() const { return fTimeCorrection; }
  Double_t       GetT0()             const { return fT0; }
  VDCpp_t*       GetPointPair()      const { return fPointPair; }
  THaTrack*      GetTrack()          const { return fTrack; }
  Int_t          GetTrackIndex()     const;
  Int_t          GetTrkNum()         const { return fTrkNum; }
  Double_t       GetSigmaT0()        const { return fSigmaT0; }
  Double_t       GetChi2()           const { return fChi2; }
  Double_t       GetNDoF()           const { return fNDoF; }
  Bool_t         IsFitOK()           const { return fFitOK; }
  Bool_t         IsUsed()            const { return (fTrack != 0); }

  void           SetPlane( THaVDCPlane* plane )     { fPlane = plane; }
  void           SetIntercept( Double_t intercept ) { fInt = intercept; }
  void           SetSlope( Double_t slope)          { fSlope = slope;}
  void           SetPivot( THaVDCHit* piv)          { fPivot = piv; }
  void           SetTimeCorrection( Double_t dt )   { fTimeCorrection = dt; }
  void           SetPointPair( VDCpp_t* pp )        { fPointPair = pp; }
  void           SetTrack( THaTrack* track );

protected:
  static const Int_t MAX_SIZE = 16;  // Assume no more than 16 hits per cluster
  static const Double_t kBig;

  Int_t          fSize;              // Size of cluster (no. of hits)
  THaVDCHit*     fHits[MAX_SIZE];    // [fSize] Hits associated w/this cluster
  THaVDCPlane*   fPlane;             // Plane the cluster belongs to
  VDCpp_t*       fPointPair;         // Lower/upper combo we're assigned to
  THaTrack*      fTrack;             // Track the cluster belongs to
  Int_t          fTrkNum;            // Number of the track using this cluster

  //Track Parameters
  Double_t       fSlope;             // Current best estimate of actual slope
  Double_t       fLocalSlope;        // Fitted slope, from FitTrack()
  Double_t       fSigmaSlope;        // Error estimate of fLocalSlope from fit
  Double_t       fInt, fSigmaInt;    // Intercept and error estimate
  Double_t       fT0, fSigmaT0;      // Fitted common timing offset and error
  THaVDCHit*     fPivot;             // Pivot - hit with smallest drift time
  Int_t          fIPivot;            // Drift sign flips at this hit index
  //FIXME: in the code, this is used as a distance correction!!
  Double_t       fTimeCorrection;    // correction to be applied when fitting
                                     // drift times
  Bool_t         fFitOK;             // Flag indicating that fit results valid
  Double_t       fChi2;              // chi2 for the cluster (using fSlope)
  Double_t       fNDoF;              // NDoF in local chi2 calculation
  Int_t          fClsBeg; 	     // Starting wire number
  Int_t          fClsEnd;            // Ending wire number

  void   CalcLocalDist();     // calculate the local track to wire distances

  void   FitSimpleTrack();
  void   FitSimpleTrackWgt(); // present for testing
  void   FitNLTrack();        // Non-linear 3-parameter fit

  chi2_t CalcChisquare( const Double_t* x, const Double_t* y,
			const Int_t* s, const Double_t* w,
			Double_t slope, Double_t icpt, Double_t d0 ) const;
  void   Linear3DFit( const Double_t* x, const Double_t* y,
		      const Int_t* s, const Double_t* w,
		      Double_t& slope, Double_t& icpt, Double_t& d0 ) const;
  Int_t  LinearClusterFitWithT0();

  ClassDef(THaVDCCluster,0)          // A group of VDC hits
};

//////////////////////////////////////////////////////////////////////////////

#endif
