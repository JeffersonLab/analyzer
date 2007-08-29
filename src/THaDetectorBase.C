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
#include "VarType.h"

using std::vector;

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
Int_t THaDetectorBase::FillDetMap( const vector<Int_t>& values, UInt_t flags,
				   const char* here )
{
  // Utility function to fill this detector's detector map. 
  // See THaDetMap::Fill for documentation.

  Int_t ret = fDetMap->Fill( values, flags );
  if( ret == 0 ) {
    Warning( Here(here), "No detector map entries found. Check database." );
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

  vector<double> position, size;
  Int_t err;
  if( required ) {
    DBRequest request[] = {
      { "position", &position, kDoubleV, 0, 0, 0,
	"\"position\" (detector position [m])" },
      { "size", &size, kDoubleV, 0, 0, 0, "\"size\" (detector size [m])"},
      { 0 }
    };
    err = LoadDB( file, date, request, fPrefix );
  } else {
    DBRequest request[] = {
      { "position", &position, kDoubleV, 0, 1 },
      { "size", &size, kDoubleV, 0, 1 },
      { 0 }
    };
    err = LoadDB( file, date, request, fPrefix );
  }
  if( err )
    return err;

  if( !position.empty() ) {
    if( position.size() != 3 ) {
      Error( Here("ReadGeometry"), "Incorrect number of values for "
	     "detector position" );
      return 1;
    }
    fOrigin.SetXYZ( position[0], position[1], position[2] );
  }
  if( !size.empty() ) {
    if( size.size() != 3 ) {
      Error( Here("ReadGeometry"), "Incorrect number of values for "
	     "detector size" );
      return 2;
    }
    fSize[0] = size[0]/2.0;
    fSize[1] = size[1]/2.0;
    fSize[2] = size[2];
  }

  return 0;
}


//_____________________________________________________________________________
ClassImp(THaDetectorBase)
