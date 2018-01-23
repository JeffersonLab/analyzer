//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"
#include "THaTrack.h"
#include "TMath.h"

ClassImp(THaSpectrometerDetector)

//______________________________________________________________________________
THaSpectrometerDetector::THaSpectrometerDetector( const char* name,
						  const char* description,
						  THaApparatus* apparatus )
  : THaDetector(name,description,apparatus)
{
  // Constructor

}

//______________________________________________________________________________
THaSpectrometerDetector::THaSpectrometerDetector( )
{
  // Constructor for ROOT I/O only
}

//______________________________________________________________________________
THaSpectrometerDetector::~THaSpectrometerDetector()
{
  // Destructor

}

//_____________________________________________________________________________
Bool_t THaSpectrometerDetector::CalcTrackIntercept( THaTrack* theTrack,
						    TVector3& icept,
						    Double_t& pathl )
{
  // Find intercept coordinates of given track with the plane of
  // this detector. Results are in 'icept' and 'pathl'
  //
  // icept:  Vector to track crossing point in track coordinate system
  // pathl:  pathlength from track origin (in the track coordinate system)
  //         to intersection point (m). This is identical to the length
  //         of the vector icept-t0

  TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
  TVector3 td( theTrack->GetTheta(), theTrack->GetPhi(), 1.0 );
  td = td.Unit();

  return IntersectPlaneWithRay( fXax, fYax, fOrigin, t0, td, pathl, icept );
}

//_____________________________________________________________________________
Bool_t THaSpectrometerDetector::CalcTrackIntercept( THaTrack* theTrack,
						    Double_t& pathl,
						    Double_t& xdet,
						    Double_t& ydet )
{
  // Find intercept coordinates of given track with the plane of
  // this detector. Results are in xdet, ydet and pathl.
  //
  // pathl: pathlength from track origin (in the track coordinate system)
  //        to intersection point (m). This is identical to the length
  //        of the vector icept-track_origin
  // xdet:  x-coordinate of intercept in detector coordinate system (m)
  // ydet:  y-coordinate of intercept in detector coordinate system (m)

  // Does not check if the intercept coordinates are actually within
  // the active area of the detector; use IsInActiveArea(val[1],val[2])
  // to find out.

  TVector3 icept;
  if( !CalcTrackIntercept(theTrack, icept, pathl) )
    return false;
  TVector3 v = TrackToDetCoord(icept);
  xdet = v.X();
  ydet = v.Y();
  return true;
}

//_____________________________________________________________________________
Bool_t THaSpectrometerDetector::CheckIntercept( THaTrack *track )
{
  TVector3 icept;
  Double_t t;
  return CalcTrackIntercept(track, icept, t);
}

//_____________________________________________________________________________
Bool_t THaSpectrometerDetector::CalcInterceptCoords( THaTrack* track,
						     Double_t& x, Double_t& y)
{
  Double_t t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
Bool_t THaSpectrometerDetector::CalcPathLen( THaTrack* track, Double_t& t )
{
  TVector3 icept;
  return CalcTrackIntercept(track, icept, t);
}
