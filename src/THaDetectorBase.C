//*-- Author :    Ole Hansen   15-Jun-01

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
// Abstract base class for a detector or subdetector.
//
// Derived classes must define all internal variables, a constructor 
// that registers any internal variables of interest in the global 
// physics variable list, and a Decode() method that fills the variables
// based on the information in the THaEvData structure.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"
#include "THaDetMap.h"
#include "THaTrack.h"
#include "TROOT.h"


ClassImp(THaDetectorBase)

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase( const char* name, 
				  const char* description ) :
  THaAnalysisObject(name,description), fNelem(0)
{
  // Normal constructor. Creates an empty detector map.

  fSize[0] = fSize[1] = fSize[2] = 0.0;
  fDetMap = new THaDetMap;
}

//_____________________________________________________________________________
THaDetectorBase::~THaDetectorBase()
{
  // Destructor

  delete fDetMap;
}

//_____________________________________________________________________________
void THaDetectorBase::PrintDetMap( Option_t* opt ) const
{
  // Print out detector map.

  fDetMap->Print( opt );
}

//_____________________________________________________________________________
void THaDetectorBase::DefineAxes(Double_t rotation_angle)
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
bool THaDetectorBase::CalcTrackIntercept(THaTrack* theTrack, 
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
bool THaDetectorBase::CheckIntercept(THaTrack *track)
{
  Double_t x, y, t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaDetectorBase::CalcInterceptCoords(THaTrack* track, Double_t& x, Double_t& y)
{
  Double_t t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaDetectorBase::CalcPathLen(THaTrack* track, Double_t& t)
{
  Double_t x, y;
  return CalcTrackIntercept(track, t, x, y);
}
