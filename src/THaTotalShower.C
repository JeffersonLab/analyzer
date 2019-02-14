///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTotalShower                                                            //
//                                                                           //
// A total shower counter, consisting of a shower and a preshower.           //
// Calculates the total energy deposited in Shower+Preshower.                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTotalShower.h"
#include "THaShower.h"
#include "VarType.h"
#include "VarDef.h"
#include "TMath.h"
#include <string>

class THaEvData;
class TClonesArray;
class TDatime;

using namespace std;

ClassImp(THaTotalShower)

//_____________________________________________________________________________
THaTotalShower::THaTotalShower( const char* name, const char* description,
				THaApparatus* apparatus ) :
  THaPidDetector(name,description,apparatus), 
  fShower(0), fPreShower(0), fMaxDx(0.0), fMaxDy(0.0)
{
  // Constructor. With this method, the subdetectors are created using
  // this detector's prefix followed by "sh" and "ps", respectively,
  // and variable names like "L.ts.sh.nhits".

  Setup( name, "sh", "ps", description, apparatus, true );
}

//_____________________________________________________________________________
THaTotalShower::THaTotalShower( const char* name, 
				const char* shower_name,
				const char* preshower_name,
				const char* description,
				THaApparatus* apparatus ) :
  THaPidDetector(name,description,apparatus),
  fShower(0), fPreShower(0), fMaxDx(0.0), fMaxDy(0.0)
{
  // Constructor. With this method, the subdetectors are created using
  // the given names 'shower_name' and 'preshower_name', and variable 
  // names like "L.sh.nhits".

  Setup( name, shower_name, preshower_name, description, apparatus, false );
}

//_____________________________________________________________________________
void THaTotalShower::Setup( const char* name,
			    const char* shower_name,
			    const char* preshower_name,
			    const char* description,
			    THaApparatus* apparatus,
			    bool subnames )
{
  // Set up the total shower counter. Called by constructor.

  static const char* const here = "Setup()";
  static const char* const message = 
    "Must construct %s detector with valid name! Object construction failed.";

  // Base class constructor failed?
  if( IsZombie()) return;

  if( !shower_name || !*shower_name ) {
    Error( Here(here), message, "shower" );
    MakeZombie();
    return;
  }
  if( !preshower_name || !*preshower_name ) {
    Error( Here(here), message, "preshower" );
    MakeZombie();
    return;
  }

  string sname, psname;
  if( subnames ) {
    sname  = string(name) + "." + shower_name;
    psname = string(name) + "." + preshower_name;
  } else {
    sname  = shower_name;
    psname = preshower_name;
  }

  if( !description || !*description ) {
    description = "Total shower counter";
    SetTitle( description );
  }
  string desc(description), sdesc(desc), psdesc(desc);
  sdesc.append(" shower subdetector" );
  psdesc.append(" preshower subdetector");

  fShower = new THaShower( sname.c_str(), sdesc.c_str(), apparatus );
  if( !fShower || fShower->IsZombie() ) {
    MakeZombie();
    return;
  }
  fPreShower = new THaShower( psname.c_str(), psdesc.c_str(), apparatus );
  if( !fPreShower || fPreShower->IsZombie() ) {
    MakeZombie();
    return;
  }
}

//_____________________________________________________________________________
THaTotalShower::~THaTotalShower()
{
  // Destructor. Remove variables from global list.

  delete fPreShower;
  delete fShower;
  if( fIsSetup )
    RemoveVariables();
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaTotalShower::Init( const TDatime& run_time )
{
  // Set up total shower counter. This function 
  // reads the total shower parameters from a local database file 
  // (e.g. "db_ts.dat"), then calls Init() for
  // the shower and preshower subdetectors, respectively.

  if( IsZombie() || !fShower || !fPreShower )
    return fStatus = kInitError;

  EStatus status;
  if( (status = THaPidDetector::Init( run_time )) ||
      (status = fShower->Init( run_time )) ||
      (status = fPreShower->Init( run_time )) )
    return fStatus = status;

  return fStatus;
}

//_____________________________________________________________________________
void THaTotalShower::Clear( Option_t* opt )
{
  // Clear event data

  THaPidDetector::Clear(opt);
  fE = kBig;
  fID = -1;
}

//_____________________________________________________________________________
Int_t THaTotalShower::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // Read configuration parameters
  Double_t dxdy[2];
  DBRequest request[] = {
    { "max_dxdy",  dxdy, kDouble, 2 },
    { 0 }
  };
  Int_t err = LoadDB( fi, date, request, fPrefix );
  fclose(fi);
  if( err )
    return err;

  fMaxDx = dxdy[0];
  fMaxDy = dxdy[1];

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaTotalShower::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register global variables

  RVarDef vars[] = {
    { "e",  "Energy (MeV) of largest cluster",    "fE" },
    { "id", "ID of Psh&Sh coincidence (1==good)", "fID" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t THaTotalShower::Decode( const THaEvData& evdata )
{
  // Decode total shower detector. Calls Decode() of fPreShower and fShower.
  // Return the return value of fShower->Decode().

  if( !IsOK() ) 
    return -1;

  fPreShower->Decode( evdata );
  return fShower->Decode( evdata );
}

//_____________________________________________________________________________
Int_t THaTotalShower::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct Clusters in shower and preshower detectors.
  // Then compute total shower energy and cluster ID.
  //
  // Valid fIDs:
  //       1   Matching clusters found.
  //       0   Clusters found, but separated too far
  //      -1   No cluster found in either shower or preshower or both
  //

  if( !IsOK() ) 
    return -1;
  
  fPreShower->CoarseProcess( tracks );
  fShower->CoarseProcess( tracks );

  if( fShower->GetNhits() == 0 || fPreShower->GetNhits() == 0 )
    fID = -1;
  else {
    fE = fShower->GetE() + fPreShower->GetE();
    Float_t dx = fPreShower->GetX() - fShower->GetX();
    Float_t dy = fPreShower->GetY() - fShower->GetY();
    if( TMath::Abs(dx) < fMaxDx && TMath::Abs(dy) < fMaxDy )
      fID = 1;
  }
  return 0;
}
//_____________________________________________________________________________
Int_t THaTotalShower::FineProcess( TClonesArray& tracks )
{
  // Fine processing. 
  // Call fPreShower->FineProcess() and fShower->FineProcess() in turn.
  // Return return value of fShower->FineProcess().

  if( !IsOK() )
    return -1;

  fPreShower->FineProcess( tracks );
  return fShower->FineProcess( tracks );
}

//_____________________________________________________________________________
void THaTotalShower::SetApparatus( THaApparatus* app )
{
  // Set the apparatus of this detector as well as the subdetectors

  THaPidDetector::SetApparatus( app );
  fShower->SetApparatus( app );
  fPreShower->SetApparatus( app );
  return;
}

///////////////////////////////////////////////////////////////////////////////
