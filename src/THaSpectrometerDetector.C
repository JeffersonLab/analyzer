//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"
#include "THaTrack.h"

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
THaSpectrometerDetector::~THaSpectrometerDetector()
{
  // Destructor

}

//_____________________________________________________________________________
void THaSpectrometerDetector::DefineAxes(Double_t rotation_angle)
{
  // define variables used for calculating intercepts of tracks
  // with the detector
  // right now, we assume that all detectors except VDCs are 
  // perpendicular to the Transport frame

  fXax.SetXYZ( TMath::Sin(rotation_angle), 0.0, TMath::Cos(rotation_angle) );
  fYax.SetXYZ( 0.0, 1.0, 0.0 );
  fZax = fXax.Cross(fYax);

  //cout<<"Z: "<<fZax.X()<<" "<<fZax.Y()<<" "<<fZax.Z()<<endl;

  fDenom.ResizeTo( 3, 3 );
  fNom.ResizeTo( 3, 3 );
  fDenom.SetColumn( fXax, 0 );
  fDenom.SetColumn( fYax, 1 );
  fNom.SetColumn( fXax, 0 );
  fNom.SetColumn( fYax, 1 );
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcTrackIntercept(THaTrack* theTrack, 
					 Double_t& t, Double_t& xcross, 
					 Double_t& ycross)
{
  // projects a given track on to the plane of the detector
  // xcross and ycross are the x and y coords of this intersection
  // t is the distance from the origin of the track to the given plane

  TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
  Double_t norm = TMath::Sqrt(1.0 + theTrack->GetTheta()*theTrack->GetTheta() +
			      theTrack->GetPhi()*theTrack->GetPhi());
  TVector3 t_hat( theTrack->GetTheta()/norm, theTrack->GetPhi()/norm, 1.0/norm );
  fDenom.SetColumn( -t_hat, 2 );
  fNom.SetColumn( t0-fOrigin, 2 );

  // first get the distance...
  Double_t det = fDenom.Determinant();
  if( fabs(det) < 1e-5 ) 
    return false;  // No useful solution for this track
  t = fNom.Determinant() / det;

  // ...then the intersection point
  TVector3 v = t0 + t*t_hat - fOrigin;
  xcross = v.Dot(fXax);
  ycross = v.Dot(fYax);

  return true;
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CheckIntercept(THaTrack *track)
{
  Double_t x, y, t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcInterceptCoords(THaTrack* track, Double_t& x, Double_t& y)
{
  Double_t t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcPathLen(THaTrack* track, Double_t& t)
{
  Double_t x, y;
  return CalcTrackIntercept(track, t, x, y);
}
