#ifndef ROOT_THaVDCUVTrack
#define ROOT_THaVDCUVTrack

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "TVector3.h"

class THaVDCCluster;
class THaVDCUVPlane;
class THaTrack;

class THaVDCUVTrack : public THaCluster {

public:
  THaVDCUVTrack() :
    fUClust(NULL), fVClust(NULL), fUVPlane(NULL), fTrack(NULL), fPartner(NULL),
    fX(0.0), fY(0.0), fTheta(0.0), fPhi(0.0) {}

  virtual ~THaVDCUVTrack() {}

  void CalcDetCoords();

  // Get and Set Functions  
  THaVDCCluster* GetUCluster() const { return fUClust; }
  THaVDCCluster* GetVCluster() const { return fVClust; }
  THaVDCUVPlane* GetUVPlane()  const { return fUVPlane; }
  THaVDCUVTrack* GetPartner()  const { return fPartner; }
  Double_t       GetX()        const { return fX; }
  Double_t       GetY()        const { return fY; }
  Double_t       GetTheta()    const { return fTheta; }
  Double_t       GetPhi()      const { return fPhi; } 

  void CalcChisquare(Double_t &chi2, Int_t &nhits) const;

  void SetUCluster( THaVDCCluster* clust)  { fUClust = clust;}
  void SetVCluster( THaVDCCluster* clust)  { fVClust = clust;}
  void SetUVPlane( THaVDCUVPlane* plane)   { fUVPlane = plane;}
  void SetTrack( THaTrack* track)          { fTrack = track; }
  void SetPartner( THaVDCUVTrack* partner) { fPartner = partner;}

  void SetX( Double_t x )                  { fX = x;}
  void SetY( Double_t y )                  { fY = y;}
  void SetTheta( Double_t theta )          { fTheta = theta;}
  void SetPhi( Double_t phi )              { fPhi = phi;} 
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }
  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    const TVector3& offset );


protected:
  THaVDCCluster* fUClust;       // Cluster in the U plane
  THaVDCCluster* fVClust;       // Cluster in the V plane
  THaVDCUVPlane* fUVPlane;      // UV plane that own's this track
  THaTrack*      fTrack;        // Track this UV Track is associated with
  THaVDCUVTrack* fPartner;      // UV track associated with this one in 
                                //  the other UV plane
  // Track Info (Detector cs - X,Y in m; theta, phi in tan(angle) )
  Double_t fX;     // X position of track in U wire-plane 
  Double_t fY;     // Y position of track in U wire-plane 
  Double_t fTheta; // Angle between z-axis and projection of track into xz plane
  Double_t fPhi;   // Angle between z-axis and projection of track into yz plane
  
private:
  // Hide copy ctor and op=
  THaVDCUVTrack( const THaVDCUVTrack& );
  THaVDCUVTrack& operator=( const THaVDCUVTrack& );

  ClassDef(THaVDCUVTrack,0)             // VDCUVTrack class
};

//-------------------- inlines -------------------------------------------------
inline
void THaVDCUVTrack::Set( Double_t x, Double_t y, Double_t theta, Double_t phi,
			 const TVector3& offset )
{
  // Set coordinates for this track. Also set absolute position vector.

  Set( x, y, theta, phi );
  fCenter.SetXYZ( x, y, 0.0 );
  fCenter += offset;
}

////////////////////////////////////////////////////////////////////////////////

#endif
