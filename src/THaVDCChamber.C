///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCChamber                                                             //
//                                                                           //
// Representation of a VDC chamber, consisting of one U plane and one        //
// V plane in close proximity. With the Hall A VDCs, the two planes of a     //
// chamber share the same mechanical frame and gas system.                   //
//                                                                           //
// This particular implementation takes the U plane as the reference for     //
// specifying coordinates. It was also written assuming that the U plane is  //
// below the V plane (smaller z). It should work the other way around as     //
// well but that configuration has not been tested. If in doubt, one can     //
// always swap the meaning of "U" and "V" such that "U" always means the     //
// first (bottom) plane.                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCChamber.h"
#include "THaVDCPlane.h"
#include "THaVDCPoint.h"
#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "TMath.h"

#include <cstring>
#include <cstdio>
#include <cassert>

//_____________________________________________________________________________
THaVDCChamber::THaVDCChamber( const char* name, const char* description, // @suppress("Class members should be properly initialized")
			      THaDetectorBase* parent )
  : THaSubDetector(name,description,parent),
    fSpacing(0), fSin_u(0), fCos_u(1), fSin_v(1), fCos_v(0), fInv_sin_vu(0)
{
  // Constructor

  // Create the U and V planes
  fU = new THaVDCPlane( "u", "U plane", this );
  fV = new THaVDCPlane( "v", "V plane", this );

  // Create array for cluster pairs (points) representing hits
  fPoints = new TClonesArray("THaVDCPoint", 10); // 10 is arbitrary
}

//_____________________________________________________________________________
THaVDCChamber::~THaVDCChamber()
{
  // Destructor. Delete plane objects and point array.

  if( fIsSetup )
    RemoveVariables();
  delete fU;
  delete fV;
  delete fPoints;
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaVDCChamber::Init( const TDatime& date )
{
  // Initialize the chamber class. Calls its own Init(), then initializes
  // subdetectors, then calculates local geometry data.

  if( IsZombie() || !fV || !fU )
    return fStatus = kInitError;

  EStatus status;
  if( (status = THaSubDetector::Init( date )) ||
      (status = fU->Init( date )) ||
      (status = fV->Init( date )))
    return fStatus = status;

  fSpacing = fV->GetZ() - fU->GetZ();  // Space between U & V wire planes

  // Precompute and store values for efficiency
  fSin_u   = fU->GetSinWAngle();
  fCos_u   = fU->GetCosWAngle();
  fSin_v   = fV->GetSinWAngle();
  fCos_v   = fV->GetCosWAngle();
  fInv_sin_vu = 1.0/TMath::Sin( fV->GetWAngle() - fU->GetWAngle() );

  // Use the same coordinate system as the planes
  fXax = fU->GetXax();
  fYax = fU->GetYax();
  fZax = fU->GetZax();

  // Calculate plane center x/y coordinates, assuming that the plane's
  // wires are arranged symmetrically around the center
  Double_t ubeg = fU->GetWBeg();
  Double_t vbeg = fV->GetWBeg();
  Double_t uend = fU->GetWBeg() + (fU->GetNWires()-1)*fU->GetWSpac();
  Double_t vend = fV->GetWBeg() + (fV->GetNWires()-1)*fV->GetWSpac();;
  Double_t xc = 0.5 * ( UVtoX(ubeg,vbeg) + UVtoX(uend,vend) );
  Double_t yc = 0.5 * ( UVtoY(ubeg,vbeg) + UVtoY(uend,vend) );
  fU->UpdateGeometry(xc,yc);
  fV->UpdateGeometry(xc,yc);

  // Take the u plane as our reference plane
  fOrigin = fU->GetOrigin();

  // Estimate our size
  fSize[0] = 0.5*TMath::Max( fU->GetXSize(), fU->GetXSize() );
  fSize[1] = 0.5*TMath::Max( fU->GetYSize(), fU->GetYSize() );
  fSize[2] = fSpacing + 0.5*fU->GetZSize() + 0.5*fV->GetZSize();

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaVDCChamber::DefineVariables( EMode mode )
{
  // initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "npt",       "Number of space points", "GetNPoints()" },
    { "pt.x",      "Point center x (m)",     "fPoints.THaVDCPoint.X()" },
    { "pt.y",      "Point center y (m)",     "fPoints.THaVDCPoint.Y()" },
    { "pt.z",      "Point center z (m)",     "fPoints.THaVDCPoint.Z()" },
    { "pt.d_x",    "Point VDC x (m)",        "fPoints.THaVDCPoint.fX" },
    { "pt.d_y",    "Point VDC y (m)",        "fPoints.THaVDCPoint.fY" },
    { "pt.d_th",   "Point VDC tan(theta)",   "fPoints.THaVDCPoint.fTheta" },
    { "pt.d_ph",   "Point VDC tan(phi)",     "fPoints.THaVDCPoint.fPhi" },
    { "pt.paired", "Point is paired",        "fPoints.THaVDCPoint.HasPartner()" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void THaVDCChamber::SetDebug( Int_t level )
{
  // Set debug level of this chamber and its plane subdetectors

  THaSubDetector::SetDebug(level);
  fU->SetDebug(level);
  fV->SetDebug(level);
}

//_____________________________________________________________________________
PointCoords_t THaVDCChamber::CalcDetCoords( const THaVDCCluster* ucl,
					    const THaVDCCluster* vcl ) const
{
  // Convert U,V coordinates of the given uv cluster pair to the detector
  // coordinate system of this chamber. Takes u as the reference plane.

  Double_t u  = ucl->GetIntercept();  // Intercept in U plane
  Double_t v0 = vcl->GetIntercept();  // Intercept in V plane
  Double_t mu = ucl->GetSlope();      // Slope of U cluster
  Double_t mv = vcl->GetSlope();      // Slope of V cluster

  // Project v0 into the u plane
  Double_t v = v0 - mv * GetSpacing();

  PointCoords_t c;
  c.x     = UVtoX(u,v);
  c.y     = UVtoY(u,v);
  c.theta = UVtoX(mu,mv);
  c.phi   = UVtoY(mu,mv);

  return c;
}

//_____________________________________________________________________________
Int_t THaVDCChamber::MatchUVClusters()
{
  // Match clusters in the U plane with cluster in the V plane by t0 and
  // geometry. Fills fPoints with good pairs.

  Int_t nu = fU->GetNClusters();
  Int_t nv = fV->GetNClusters();

  // Match best in-time clusters (t_0 close to zero).
  // Discard any out-of-time clusters (|t_0| > N sigma, N = 3, configurable).

  Double_t max_u_t0 = fU->GetT0Resolution();
  Double_t max_v_t0 = fV->GetT0Resolution();

  Int_t nuv = 0;

  // Consider all possible uv cluster combinations
  for( Int_t iu = 0; iu < nu; ++iu ) {
    THaVDCCluster* uClust = fU->GetCluster(iu);
    if( TMath::Abs(uClust->GetT0()) > max_u_t0 )
      continue;

    for( Int_t iv = 0; iv < nv; ++iv ) {
      THaVDCCluster* vClust = fV->GetCluster(iv);
      if( TMath::Abs(vClust->GetT0()) > max_v_t0 )
	continue;

      // Test position to be within drift chambers
      const PointCoords_t c = CalcDetCoords(uClust,vClust);
      if( !fU->IsInActiveArea( c.x, c.y ) ) {
	continue;
      }

      // This cluster pair passes preliminary tests, so we save it, regardless
      // of possible ambiguities, which will be sorted out later
      new( (*fPoints)[nuv++] ) THaVDCPoint( uClust, vClust, this );
    }
  }

  // return the number of UV pairs found
  return nuv;
}

//_____________________________________________________________________________
Int_t THaVDCChamber::CalcPointCoords()
{
  // Compute track info (x, y, theta, phi) for the all matched points.
  // Uses TRANSPORT coordinates.

  Int_t nPoints = GetNPoints();
  for (int i = 0; i < nPoints; i++) {
    THaVDCPoint* point = GetPoint(i);
    if( point )
      point->CalcDetCoords();
  }
  return nPoints;
}

//_____________________________________________________________________________
void THaVDCChamber::Clear( Option_t* opt )
{
  // Clear event-by-event data
  THaSubDetector::Clear(opt);
  fU->Clear(opt);
  fV->Clear(opt);
  fPoints->Clear();
}

//_____________________________________________________________________________
Int_t THaVDCChamber::Decode( const THaEvData& evData )
{
  // Decode both wire planes

  fU->Decode(evData);
  fV->Decode(evData);
  return 0;
}

//_____________________________________________________________________________
void THaVDCChamber::FindClusters()
{
  // Find clusters in U & V planes

  fU->FindClusters();
  fV->FindClusters();
}

//_____________________________________________________________________________
void THaVDCChamber::FitTracks()
{
  // Fit data to recalculate cluster position

  fU->FitTracks();
  fV->FitTracks();
}

//_____________________________________________________________________________
Int_t THaVDCChamber::CoarseTrack()
{
  // Coarse computation of tracks

  // Find clusters and estimate their position/slope
  FindClusters();

  // Fit "local" tracks through the hit coordinates of each cluster
  FitTracks();

  // FIXME: The fit may fail, so we might want to call a function here
  // that deletes points whose clusters have bad fits or are too small
  // (e.g. 1 hit), etc.
  // Right now, we keep them, preferring efficiency over precision.

  // Pair U and V clusters
  MatchUVClusters();

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCChamber::FineTrack()
{
  // High precision computation of tracks

  //TODO:
  //- Redo time-to-distance conversion based on "global" track slope
  //- Refit clusters of the track (if any), using new drift distances
  //- Recalculate cluster UV coordinates, so new "gobal" slopes can be
  //  computed

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaVDCChamber)
