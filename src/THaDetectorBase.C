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
THaDetectorBase::THaDetectorBase() : fDetMap(0) {
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
  // Define detector orientation, assuming a tilt by rotation_angle around
  // the y-axis

  fXax.SetXYZ( TMath::Cos(rotation_angle), 0.0, TMath::Sin(rotation_angle) );
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
Bool_t THaDetectorBase::IsInActiveArea( Double_t x, Double_t y ) const
{
  // Check if given (x,y) coordinates are inside this detector's active area
  // (defined by fOrigin/fSize)

  return ( x >= fOrigin.X()-fSize[0] &&
	   x <= fOrigin.X()+fSize[0] &&
	   y >= fOrigin.Y()-fSize[1] &&
	   y <= fOrigin.Y()+fSize[1] );
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

  vector<double> position, size;
  Bool_t optional = !required;
  DBRequest request[] = {
    { "position", &position, kDoubleV, 0, optional, 0,
      "\"position\" (detector position [m])" },
    { "size",     &size,     kDoubleV, 0, optional, 0,
      "\"size\" (detector size [m])"},
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

  return 0;
}


//_____________________________________________________________________________
ClassImp(THaDetectorBase)
