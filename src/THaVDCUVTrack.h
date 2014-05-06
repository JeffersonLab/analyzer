#ifndef ROOT_THaVDCUVTrack
#define ROOT_THaVDCUVTrack

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// NB: this is not really a "track", but rather a "cluster pair". However,   //
// a cluster pair alone already fully defines a track, albeit with low       //
// angular resolution.                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "THaVDCCluster.h"
#include "TVector3.h"
#include <cassert>

class THaVDCCluster;
class THaVDCUVPlane;
class THaTrack;

class THaVDCUVTrack : public THaCluster {

public:
  THaVDCUVTrack( THaVDCCluster* u_cl, THaVDCCluster* v_cl,
		 THaVDCUVPlane* plane ) :
    fUClust(u_cl), fVClust(v_cl), fUVPlane(plane), fTrack(0), fPartner(0),
    fX(kBig), fY(kBig), fTheta(kBig), fPhi(kBig)
  { assert( fUClust && fVClust && fUVPlane ); }

  virtual ~THaVDCUVTrack() {}

  void CalcDetCoords();

  // Get and Set Functions  
  THaVDCCluster* GetUCluster() const { return fUClust; }
  THaVDCCluster* GetVCluster() const { return fVClust; }
  THaVDCUVPlane* GetUVPlane()  const { return fUVPlane; }
  THaVDCUVTrack* GetPartner()  const { return fPartner; }
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
  void SetPartner( THaVDCUVTrack* partner) { fPartner = partner;}

  void SetSlopes( Double_t mu, Double_t mv );

protected:
  static const Double_t kBig;

  THaVDCCluster* fUClust;       // Cluster in the U plane
  THaVDCCluster* fVClust;       // Cluster in the V plane
  THaVDCUVPlane* fUVPlane;      // UV plane that owns this track
  THaTrack*      fTrack;        // Track this UV Track is associated with
  THaVDCUVTrack* fPartner;      // UV track associated with this one in 
                                //  the other UV plane
  // Detector coordinates derived from the cluster coordinates
  // at the U plane (z = GetZ()).  X,Y in m; theta, phi in tan(angle)
  Double_t fX;     // X position of track in U wire-plane 
  Double_t fY;     // Y position of track in U wire-plane 
  Double_t fTheta; // Angle between z-axis and projection of track into xz plane
  Double_t fPhi;   // Angle between z-axis and projection of track into yz plane

  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    const TVector3& offset );

private:
  // Hide copy ctor and op=
  THaVDCUVTrack( const THaVDCUVTrack& );
  THaVDCUVTrack& operator=( const THaVDCUVTrack& );

  ClassDef(THaVDCUVTrack,0)             // VDCUVTrack class
};

//-------------------- inlines ------------------------------------------------
inline
void THaVDCUVTrack::Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
			 const TVector3& offset )
{
  // Set coordinates for this track. Also set absolute position vector.

  Set( x, y, theta, phi );
  fCenter.SetXYZ( x, y, 0.0 );
  fCenter += offset;
}
//_____________________________________________________________________________
inline
Double_t THaVDCUVTrack::GetU() const
{
  // Return intercept of u cluster

  return fUClust->GetIntercept();
}

//_____________________________________________________________________________
inline
Double_t THaVDCUVTrack::GetV() const
{
  // Return intercept of v cluster

  return fVClust->GetIntercept();
}

///////////////////////////////////////////////////////////////////////////////

#endif
