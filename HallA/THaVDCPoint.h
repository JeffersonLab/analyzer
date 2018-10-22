#ifndef Podd_THaVDCPoint_h_
#define Podd_THaVDCPoint_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPoint                                                               //
//                                                                           //
// A pair of one U and one V VDC cluster in a given VDC chamber              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include "THaVDCCluster.h"

class THaVDCChamber;
class THaTrack;

class THaVDCPoint : public THaCluster {

public:
  THaVDCPoint( THaVDCCluster* u_cl, THaVDCCluster* v_cl,
	       THaVDCChamber* chamber );
  virtual ~THaVDCPoint() {}

  void CalcDetCoords();

  // Get and Set Functions
  THaVDCCluster* GetUCluster() const { return fUClust; }
  THaVDCCluster* GetVCluster() const { return fVClust; }
  THaVDCChamber* GetChamber()  const { return fChamber; }
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
  Bool_t         HasPartner()  const { return (fPartner != 0); }

  void CalcChisquare(Double_t &chi2, Int_t &nhits) const;

  void SetTrack( THaTrack* track);
  void SetPartner( THaVDCPoint* partner) { fPartner = partner;}

  void SetSlopes( Double_t mu, Double_t mv );

protected:
  THaVDCCluster* fUClust;       // Cluster in the U plane
  THaVDCCluster* fVClust;       // Cluster in the V plane
  THaVDCChamber* fChamber;      // Chamber of this cluster pair
  THaTrack*      fTrack;        // Track that this point is associated with
  THaVDCPoint*   fPartner;      // Point associated with this one in
				//  the other chamber
  // Detector system coordinates derived from fUClust and fVClust
  // at the U plane (z = GetZ()).  X,Y in m; theta, phi in tan(angle)
  Double_t fX;     // X position of point in U wire plane
  Double_t fY;     // Y position of point in U wire plane
  Double_t fTheta; // tan(angle between z-axis and track proj onto xz plane)
  Double_t fPhi;   // tan(angle between z-axis and track proj onto yz plane)

  void Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }

private:
  // Hide copy ctor and op=
  THaVDCPoint( const THaVDCPoint& );
  THaVDCPoint& operator=( const THaVDCPoint& );

  ClassDef(THaVDCPoint,0)    // Pair of one U and one V cluster in a VDC chamber
};

//-------------------- inlines ------------------------------------------------
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
