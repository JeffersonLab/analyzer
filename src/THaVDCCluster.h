#ifndef ROOT_THaVDCCluster
#define ROOT_THaVDCCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

class THaVDCHit;
class THaVDCPlane;
//class THaVDCUVTrack;
//class THaTrack;

class THaVDCCluster : public TObject {

public:
  THaVDCCluster( THaVDCPlane* owner = NULL ) :
    fSize(0), fPlane(owner), fSlope(0.0), fSigmaSlope(0.0), fInt(0.0),
    fSigmaInt(0.0), fT0(0.0), fPivot(NULL) {}
  THaVDCCluster( const THaVDCCluster&);
  THaVDCCluster& operator=( const THaVDCCluster& );

  virtual ~THaVDCCluster() {}

  virtual void AddHit(THaVDCHit * hit);
  virtual void EstTrackParameters();
  virtual void ConvertTimeToDist();
  virtual void FitTrack();
  virtual void FitSimpleTrack();
  virtual void Clear( Option_t* opt="" );
    
  //Get and Set Functions
  Int_t        GetSize ()          const { return fSize; }
  THaVDCHit**  GetHits()                 { return fHits; } // Get array of pointers
  THaVDCHit *  GetHit(Int_t i)     const { return fHits[i]; }
  THaVDCPlane* GetPlane()          const { return fPlane; }
  Double_t     GetSlope()          const { return fSlope; }
  Double_t     GetSigmaSlope()     const { return fSigmaSlope; }
  Double_t     GetIntercept()      const { return fInt; }
  Double_t     GetSigmaIntercept() const { return fSigmaInt; }
  THaVDCHit*   GetPivot()          const { return fPivot; }
  Int_t        GetPivotWireNum()   const;
  
  void SetPlane( THaVDCPlane* plane )     { fPlane = plane; }
  void SetIntercept( Double_t intercept ) { fInt = intercept; }
  void SetSlope( Double_t slope)          { fSlope = slope;}
  void SetPivot( THaVDCHit* piv)          { fPivot = piv; }
  //    void SetUVTrack(THaVDCUVTrack * uvtrack) {fUVTrack = uvtrack;} 
  //    void SetTrack(THaTrack * track) {fTrack = track;} 

protected:
  static const int MAX_SIZE = 20;    // Assume no more than 20 hits per cluster

  Int_t         fSize;               // Size of cluster (no. of hits)
  THaVDCHit*    fHits[MAX_SIZE];     // [fSize] Hits associated w/this cluster
  THaVDCPlane*  fPlane;              // Plane the cluster belongs to
  //  THaVDCUVTrack * fUVTrack;      // UV Track the cluster belongs to
  //  THaTrack * fTrack;             // Track the cluster belongs to

  //Local Track Parameters 
  Double_t      fSlope, fSigmaSlope;  // Slope and error in slope
  Double_t      fInt, fSigmaInt;      // Intercept and error
  Double_t      fT0;                  // Fitted common timing offset
  THaVDCHit*    fPivot;               // Pivot - hit with smallest drift time

  ClassDef(THaVDCCluster,0)           // A group of VDC hits
};

//////////////////////////////////////////////////////////////////////////////

#endif
