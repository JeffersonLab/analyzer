#ifndef ROOT_THaVDCPoint
#define ROOT_THaVDCPoint

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPoint                                                               //
//                                                                           //
// A pair of one U and one V VDC cluster in a given VDC chamber              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "THaVDCCluster.h"
#include "TVector3.h"
#include <cassert>

class THaVDCUVPlane;
class THaTrack;

class THaVDCPoint : public THaCluster {

public:
  THaVDCPoint( THaVDCCluster* u_cl, THaVDCCluster* v_cl,
		 THaVDCUVPlane* plane );
  virtual ~THaVDCPoint() {}

  void CalcDetCoords();

  // Get and Set Functions
  THaVDCCluster* GetUCluster() const { return fUClust; }
  THaVDCCluster* GetVCluster() const { return fVClust; }
  THaVDCUVPlane* GetUVPlane()  const { return fUVPlane; }
  THaVDCPoint*   GetPartner()  const { return fPartner; }
  THaTrack*      GetTrack()    const { return fTrack; }
  Double_t       GetU()        const;
  Double_t       GetV()        const;
  Double_t       GetX()        const { return fX; }
  Double_t       GetY()        const { return fY; }
  Double_t       GetTheta()    const { return fTheta; }
  Double_t       GetPhi()      const { return fPhi; }
  Int_t          GetTrackIndex() const;
  Double_t       GetZU()       const;
  Double_t       GetZV()       const;
  Double_t       GetZ()        const { return GetZU(); }

  void CalcChisquare(Double_t &chi2, Int_t &nhits) const;

  void SetTrack( THaTrack* track);
  void SetPartner( THaVDCPoint* partner) { fPartner = partner;}

  void SetSlopes( Double_t mu, Double_t mv );

protected:
  static const Double_t kBig;

  THaVDCCluster* fUClust;       // Cluster in the U plane
  THaVDCCluster* fVClust;       // Cluster in the V plane
  THaVDCUVPlane* fUVPlane;      // Chamber of this cluster pair
  THaTrack*      fTrack;        // Track that this point is associated with
  THaVDCPoint*   fPartner;      // Point associated with this one in
                                //  the other UV plane
  // Detector coordinates derived from the cluster coordinates
  // at the U plane (z = GetZ()).  X,Y in m; theta, phi in tan(angle)
  Double_t fX;     // X position of point in U wire-plane
  Double_t fY;     // Y position of point in U wire-plane
  Double_t fTheta; // Angle between z-axis and projection of track into xz plane
  Double_t fPhi;   // Angle between z-axis and projection of track into yz plane

  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    const TVector3& offset );

private:
  // Hide copy ctor and op=
  THaVDCPoint( const THaVDCPoint& );
  THaVDCPoint& operator=( const THaVDCPoint& );

  ClassDef(THaVDCPoint,0)    // Pair of one U and one V cluster in a VDC chamber
};

//-------------------- inlines ------------------------------------------------
inline
void THaVDCPoint::Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
			 const TVector3& offset )
{
  // Set coordinates of this point. Also set absolute position vector.

  Set( x, y, theta, phi );
  fCenter.SetXYZ( x, y, 0.0 );
  fCenter += offset;
}

//_____________________________________________________________________________
inline
Double_t THaVDCPoint::GetU() const
{
  // Return intercept of u cluster

  return fUClust->GetIntercept();
}

//_____________________________________________________________________________
inline
Double_t THaVDCPoint::GetV() const
{
  // Return intercept of v cluster

  return fVClust->GetIntercept();
}

///////////////////////////////////////////////////////////////////////////////

#endif
