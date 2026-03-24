//*-- Author :    Ole Hansen   22-Mar-2026
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::DetMapFromDB                                                 //
//                                                                           //
// Test object for DetMap_t tests. Reads detector maps from the database.    //
// We do this here so we can test THaDetectorBase::ReadDetMap method.        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DetMapFromDB.h"
#include <cstdio>

namespace Podd::Tests {

//_____________________________________________________________________________
// DetMapFromDB constructor
DetMapFromDB::DetMapFromDB( const char* name, const char* description )
  : THaDetector(name, description)
{}

//_____________________________________________________________________________
// Destructor. Free memory and remove variables from global list.
DetMapFromDB::~DetMapFromDB()
{
  RemoveVariables(); // should do nothing for this class
}

//_____________________________________________________________________________
// Read the database, and in this case, only the detector map. This is the
// functionality to be tested.
Int_t DetMapFromDB::ReadDatabase( const TDatime& date )
{
  FILE* file = OpenFile(date);
  if( !file )
    return kFileError;

  // The flags here are used only for legacy detector maps defined
  // with "detmap = <ints>". v2 maps "detector_map = <string>" ignore them.
  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel |
    THaDetMap::kFillRefIndex | THaDetMap::kFillRefChan;
  Int_t err = ReadDetMap( file, date, flags );
  if( err ) {
    (void)fclose(file);
    return kInitError;
  }

  // Sanity check whether the above call had unexpected side effects
  fNumber = -1;
  std::vector<DBRequest> req = {
    { .name = "number", .var = &fNumber, .type = kInt }
  } ;
  err = LoadDB( file, date, req );
  (void)fclose(file);
  if( err )
    return kInitError;

  return kOK;
}

//_____________________________________________________________________________

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Podd::Tests::DetMapFromDB)
#endif
