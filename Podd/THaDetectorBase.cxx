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
#include "TMath.h"
#include "VarType.h"
#include "TRotation.h"

using std::vector;

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase( const char* name,
				  const char* description ) :
  THaAnalysisObject(name,description), fNelem(0),
  fXax(1.0,0.0,0.0), fYax(0.0,1.0,0.0), fZax(0.0,0.0,1.0)
{
  // Normal constructor. Creates an empty detector map.

  fSize[0] = fSize[1] = fSize[2] = kBig;
  fDetMap = new THaDetMap;
}

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase() : fDetMap(0), fNelem(0) {
  // for ROOT I/O only
}

//_____________________________________________________________________________
THaDetectorBase::~THaDetectorBase()
{
  // Destructor
  delete fDetMap;
}

//_____________________________________________________________________________
void THaDetectorBase::DefineAxes( Double_t rotation_angle )
{
  // Define detector orientation, assuming a tilt by rotation_angle (in rad)
  // around the y-axis

  fXax.SetXYZ( 1.0, 0.0, 0.0 );
  fXax.RotateY( rotation_angle );
  fYax.SetXYZ( 0.0, 1.0, 0.0 );
  fZax = fXax.Cross(fYax);

}

//_____________________________________________________________________________
Int_t THaDetectorBase::FillDetMap( const vector<Int_t>& values, UInt_t flags,
				   const char* here )
{
  // Utility function to fill this detector's detector map.
  // See THaDetMap::Fill for documentation.

  Int_t ret = fDetMap->Fill( values, flags );
  if( ret == 0 ) {
    Error( Here(here), "No detector map entries found. Check database." );
  } else if( ret == -1 ) {
    Error( Here(here), "Too many detector map entries (maximum %d)",
	   THaDetMap::kDetMapSize );
  } else if( ret < -1 ) {
    Error( Here(here), "Invalid detector map data format "
	   "(wrong number of values). Check database." );
  }
  return ret;
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::DetToTrackCoord( const TVector3& p ) const
{
  // Convert coordinates of point 'p' from the detector to the track
  // reference frame.

  return TVector3( p.X()*fXax + p.Y()*fYax + p.Z()*fZax + fOrigin );
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::DetToTrackCoord( Double_t x, Double_t y ) const
{
  // Convert x/y coordinates in detector plane to the track reference frame.

  return TVector3( x*fXax + y*fYax + fOrigin );
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::TrackToDetCoord( const TVector3& point ) const
{
  // Convert/project coordinates of the given 'point' to coordinates in this
  // detector's reference frame (defined by fOrigin, fXax, and fYax).

  TVector3 v = point - fOrigin;
  // This works out to exactly the same as a multiplication with TRotation
  return TVector3( v.Dot(fXax), v.Dot(fYax), v.Dot(fZax) );
}

//_____________________________________________________________________________
Bool_t THaDetectorBase::IsInActiveArea( Double_t x, Double_t y ) const
{
  // Check if given (x,y) coordinates are inside this detector's active area
  // (defined by fSize). x and y MUST be given in coordinates of this detector
  // plane, i.e. relative to fOrigin and measured along fXax and fYax.

  return ( TMath::Abs(x) <= fSize[0] &&
	   TMath::Abs(y) <= fSize[1] );
}

//_____________________________________________________________________________
Bool_t THaDetectorBase::IsInActiveArea( const TVector3& point ) const
{
  // Check if given point 'point' is inside this detector's active area
  // (defined by fSize). 'point' is first projected onto the detector
  // plane (defined by fOrigin, fXax and fYax), and then the projected
  // (x,y) coordinates are checked against fSize.
  // Usually 'point' lies within the detector plane. If it does not,
  // be sure you know what you are doing.

  TVector3 v = TrackToDetCoord( point );
  return IsInActiveArea( v.X(), v.Y() );
}

//_____________________________________________________________________________
void THaDetectorBase::PrintDetMap( Option_t* opt ) const
{
  // Print out detector map.

  fDetMap->Print( opt );
}

//_____________________________________________________________________________
Int_t THaDetectorBase::ReadGeometry( FILE* file, const TDatime& date,
				     Bool_t required )
{
  // Read this detector's basic geometry information from the database.
  // Derived classes may override to read more advanced data.

  const char* const here = "ReadGeometry";

  vector<double> position, size, angles;
  Bool_t optional = !required;
  DBRequest request[] = {
    { "position", &position, kDoubleV, 0, optional, 0,
      "\"position\" (detector position [m])" },
    { "size",     &size,     kDoubleV, 0, optional, 0,
      "\"size\" (detector size [m])" },
    { "angle",    &angles,   kDoubleV, 0, 1, 0,
      "\"angle\" (detector angles(s) [deg]" },
    { 0 }
  };
  Int_t err = LoadDB( file, date, request );
  if( err )
    return kInitError;

  if( !position.empty() ) {
    if( position.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %u for "
	     "detector position. Must be exactly 3. Fix database.",
	     static_cast<unsigned int>(position.size()) );
      return 1;
    }
    fOrigin.SetXYZ( position[0], position[1], position[2] );
  }
  else
    fOrigin.SetXYZ(0,0,0);

  if( !size.empty() ) {
    if( size.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %u for "
	     "detector size. Must be exactly 3. Fix database.",
	     static_cast<unsigned int>(size.size()) );
      return 2;
    }
    if( size[0] == 0 || size[1] == 0 || size[2] == 0 ) {
      Error( Here(here), "Illegal zero detector dimension. Fix database." );
      return 3;
    }
    if( size[0] < 0 || size[1] < 0 || size[2] < 0 ) {
      Warning( Here(here), "Illegal negative value for detector dimension. "
	       "Taking absolute. Check database." );
    }
    fSize[0] = 0.5 * TMath::Abs(size[0]);
    fSize[1] = 0.5 * TMath::Abs(size[1]);
    fSize[2] = TMath::Abs(size[2]);
  }
  else
    fSize[0] = fSize[1] = fSize[2] = kBig;

  if( !angles.empty() ) {
    if( angles.size() != 1 && angles.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %u for "
	     "detector angle(s). Must be either 1 or 3. Fix database.",
	     static_cast<unsigned int>(angles.size()) );
      return 4;
    }
    // If one angle is given, it indicates a rotation about y, as before.
    // If three angles are given, they are interpreted as Euler angles
    // following the y-convention, i.e. rotation about z, rotation about y',
    // rotation about z''. The sign of a rotation is defined by the right-hand
    // rule (mathematics definition): if the thumb points along the positive
    // direction of the axis, the fingers indicate a positive rotation.
    if( angles.size() == 1 ) {
      DefineAxes( angles[0] * TMath::DegToRad() );
    }
    else {
      TRotation m;
      // For TRotation (and only TRotation, it seems), ROOT's definition of the
      // sense of rotation seems to be the left-hand rule: "A rotation around a
      // specified axis means counterclockwise rotation around the positive
      // direction of the axis." But we define rotations by the right-hand rule,
      // so we need to invert all angles.
      m.SetYEulerAngles( -angles[0] * TMath::DegToRad(),
			 -angles[1] * TMath::DegToRad(),
			 -angles[2] * TMath::DegToRad() );
      fXax.SetXYZ( m.XX(), m.XY(), m.XZ() );
      fYax.SetXYZ( m.YX(), m.YY(), m.YZ() );
      fZax.SetXYZ( m.ZX(), m.ZY(), m.ZZ() );
    }
  } else
    DefineAxes(0);

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaDetectorBase)
