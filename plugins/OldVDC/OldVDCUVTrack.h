#ifndef Podd_OldVDCUVTrack_h_
#define Podd_OldVDCUVTrack_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCUVTrack                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "TVector3.h"

class OldVDCCluster;
class OldVDCUVPlane;
class THaTrack;

class OldVDCUVTrack : public THaCluster {

public:
  OldVDCUVTrack() :
    fUClust(NULL), fVClust(NULL), fUVPlane(NULL), fTrack(NULL), fPartner(NULL),
    fX(0.0), fY(0.0), fTheta(0.0), fPhi(0.0) {}

  virtual ~OldVDCUVTrack() {}

  void CalcDetCoords();

  // Get and Set Functions  
  OldVDCCluster* GetUCluster() const { return fUClust; }
  OldVDCCluster* GetVCluster() const { return fVClust; }
  OldVDCUVPlane* GetUVPlane()  const { return fUVPlane; }
  OldVDCUVTrack* GetPartner()  const { return fPartner; }
  Double_t       GetX()        const { return fX; }
  Double_t       GetY()        const { return fY; }
  Double_t       GetTheta()    const { return fTheta; }
  Double_t       GetPhi()      const { return fPhi; } 

  void CalcChisquare(Double_t &chi2, Int_t &nhits) const;

  void SetUCluster( OldVDCCluster* clust)  { fUClust = clust;}
  void SetVCluster( OldVDCCluster* clust)  { fVClust = clust;}
  void SetUVPlane( OldVDCUVPlane* plane)   { fUVPlane = plane;}
  void SetTrack( THaTrack* track)          { fTrack = track; }
  void SetPartner( OldVDCUVTrack* partner) { fPartner = partner;}

  void SetX( Double_t x )                  { fX = x;}
  void SetY( Double_t y )                  { fY = y;}
  void SetTheta( Double_t theta )          { fTheta = theta;}
  void SetPhi( Double_t phi )              { fPhi = phi;} 
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    const TVector3& offset );


protected:
  OldVDCCluster* fUClust;       // Cluster in the U plane
  OldVDCCluster* fVClust;       // Cluster in the V plane
  OldVDCUVPlane* fUVPlane;      // UV plane that own's this track
  THaTrack*      fTrack;        // Track this UV Track is associated with
  OldVDCUVTrack* fPartner;      // UV track associated with this one in 
                                //  the other UV plane
  // Track Info (Detector cs - X,Y in m; theta, phi in tan(angle) )
  Double_t fX;     // X position of track in U wire-plane 
  Double_t fY;     // Y position of track in U wire-plane 
  Double_t fTheta; // Angle between z-axis and projection of track into xz plane
  Double_t fPhi;   // Angle between z-axis and projection of track into yz plane
  
private:
  // Hide copy ctor and op=
  OldVDCUVTrack( const OldVDCUVTrack& );
  OldVDCUVTrack& operator=( const OldVDCUVTrack& );

  ClassDef(OldVDCUVTrack,0)             // VDCUVTrack class
};

//-------------------- inlines -------------------------------------------------
inline
void OldVDCUVTrack::Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
			 const TVector3& offset )
{
  // Set coordinates for this track. Also set absolute position vector.

  Set( x, y, theta, phi );
  fCenter.SetXYZ( x, y, 0.0 );
  fCenter += offset;
}

////////////////////////////////////////////////////////////////////////////////

#endif
