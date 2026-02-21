//*-- Author :    Vincent Sulkosky and R. Feuerbach,  Jan 2006
// Changed to an abstract base class, Ole Hansen, Aug 2006
////////////////////////////////////////////////////////////////////////
//
// THaHelicityDet
//
// Base class for a beam helicity detector.
//
////////////////////////////////////////////////////////////////////////


#include "THaHelicityDet.h"
#include <vector>

using namespace std;

//_____________________________________________________________________________
THaHelicityDet::THaHelicityDet( const char* name, const char* description ,
				THaApparatus* apparatus )
  : THaDetector( name, description, apparatus )
  , fHelicity(kUnknown)
  , fSign(1)
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::~THaHelicityDet()
{
  // Destructor
  RemoveVariables();
}

//_____________________________________________________________________________
Int_t THaHelicityDet::DefineVariables( EMode mode )
{
  // Initialize global variables

  const vector<RVarDef> var = {
    { .name = "helicity", .desc = "Beam helicity",  .def = "fHelicity" },
  };
  return DefineVarsFromList( var, mode );
}

//_____________________________________________________________________________
const char* THaHelicityDet::GetDBFileName() const
{
  // All helicity detectors read from db_hel.dat by default. Database
  // keys should (but do not have to) be prefixed by each detector's prefix,
  // e.g. L.adchel.detmap

  return "hel.";
}

//_____________________________________________________________________________
void THaHelicityDet::MakePrefix()
{
  // All helicity detectors use only their name, not apparatus.name, as
  // prefix.

  THaDetectorBase::MakePrefix( nullptr );

}

//_____________________________________________________________________________
Int_t THaHelicityDet::ReadDatabase( const TDatime& date )
{
  // Read fSign

  fSign = 1;  // Default sign is +1

  FILE* file = OpenFile( date );
  if( !file )
    // No database file is fine since we only read an optional parameter here
    return kOK;

  const vector<DBRequest> request = {
    { .name = "helicity_sign", .var = &fSign, .type = kInt, .optional = true, .search = -2 },
  };
  Int_t err = LoadDB( file, date, request );
  (void)fclose(file);
  if( err )
    return kInitError;

  return kOK;
}

ClassImp(THaHelicityDet)
