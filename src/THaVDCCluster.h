#ifndef ROOT_THaVDCCluster
#define ROOT_THaVDCCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "TObject.h"

class THaVDCHit;
class THaVDCPlane;
//class THaVDCUVTrack;
//class THaTrack;

class THaVDCCluster : public THaCluster {

public:
  THaVDCCluster( THaVDCPlane* owner = NULL ) :
    fPlane(owner), fSlope(0.0), fSigmaSlope(0.0), fInt(0.0),
    fSigmaInt(0.0), fT0(0.0), fPivot(NULL) {}
  THaVDCCluster( const THaVDCCluster&);
  THaVDCCluster& operator=( const THaVDCCluster& );
  virtual ~THaVDCCluster() {}

  enum EMode { kSimple, kT0, kFull };

  virtual void   AddHit(THaVDCHit * hit);
  virtual void   EstTrackParameters();
  virtual void   ConvertTimeToDist();
  virtual void   FitTrack( EMode mode = kSimple );
  virtual void   ClearFit();

  // TObject functions redefined
  virtual void   Clear( Option_t* opt="" );
  virtual Int_t  Compare( const TObject* obj ) const;
  virtual Bool_t IsSortable() const        { return kTRUE; }
  virtual void   Print( Option_t* opt="" ) const;

  //Get and Set Functions
  THaVDCHit**    GetHits()                 { return fHits; } // Get array of pointers
  THaVDCHit *    GetHit(Int_t i)     const { return fHits[i]; }
  THaVDCPlane*   GetPlane()          const { return fPlane; }
  Double_t       GetSlope()          const { return fSlope; }
  Double_t       GetSigmaSlope()     const { return fSigmaSlope; }
  Double_t       GetIntercept()      const { return fInt; }
  Double_t       GetSigmaIntercept() const { return fSigmaInt; }
  THaVDCHit*     GetPivot()          const { return fPivot; }
  Int_t          GetPivotWireNum()   const;
  
  void           SetPlane( THaVDCPlane* plane )     { fPlane = plane; }
  void           SetIntercept( Double_t intercept ) { fInt = intercept; }
  void           SetSlope( Double_t slope)          { fSlope = slope;}
  void           SetPivot( THaVDCHit* piv)          { fPivot = piv; }
  //    void SetUVTrack(THaVDCUVTrack * uvtrack) {fUVTrack = uvtrack;} 
  //    void SetTrack(THaTrack * track) {fTrack = track;} 

protected:
  static const Int_t MAX_SIZE = 16;  // Assume no more than 20 hits per cluster
  static const Double_t kBig;

  THaVDCHit*     fHits[MAX_SIZE];    // [fSize] Hits associated w/this cluster
  THaVDCPlane*   fPlane;             // Plane the cluster belongs to
  //  THaVDCUVTrack * fUVTrack;      // UV Track the cluster belongs to
  //  THaTrack * fTrack;             // Track the cluster belongs to

  //Local Track Parameters 
  Double_t       fSlope, fSigmaSlope;// Slope and error in slope
  Double_t       fInt, fSigmaInt;    // Intercept and error
  Double_t       fT0;                // Fitted common timing offset
  THaVDCHit*     fPivot;             // Pivot - hit with smallest drift time

  virtual void   FitSimpleTrack();

  ClassDef(THaVDCCluster,0)          // A group of VDC hits
};

//////////////////////////////////////////////////////////////////////////////

#endif
