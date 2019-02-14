//*-- Author :    Ole Hansen   04-Dec-03

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// UserModule                                                           //
//                                                                      //
// Example of a user physics module that illustrates some typical       //
// operations.                                                          //
//                                                                      //
// The module takes tracks from one spectrometer and calculates two     //
// derived quantities, fResultA and fResultB. The two results are       //
// exported as global variables and can also be accessed through Get    //
// functions.                                                           //
//                                                                      //
// The calculation uses one parameter, fParameter. This could be a      //
// particle mass, a magnetic field strength, etc. The parameter value   //
// usually comes from the run database, but can also be set explicitly  //
// in the constructor or via  SetParameter(),                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "UserModule.h"
#include "THaTrackingModule.h"
#include "VarDef.h"

using namespace std;

//_____________________________________________________________________________
UserModule::UserModule( const char* name, const char* description,
			const char* spectro, Double_t parameter ) :
  THaPhysicsModule(name,description), 
  fResultA(kBig), fResultB(kBig), fParameter(parameter),
  fSpectroName(spectro), fSpectro(NULL)
{
  // Normal constructor. 

}

//_____________________________________________________________________________
UserModule::~UserModule()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void UserModule::Clear( Option_t* opt )
{
  // Clear all internal variables.

  THaPhysicsModule::Clear(opt);

  fResultA = fResultB = kBig;
  fDataValid = false;
}

//_____________________________________________________________________________
Int_t UserModule::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "A",  "Result A", "fResultA" },
    { "B",  "Result B", "fResultB" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus UserModule::Init( const TDatime& run_time )
{
  // Initialize the module. Do standard initialization, then
  // locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's ReadDatabase(),
  // ReadRunDatabase(), and DefineVariables()
  // (see THaAnalysisObject::Init)
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  // Find the spectrometer (or tracking module) by name and make sure
  // it is of the correct type.
  fSpectro = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSpectroName.Data(), "THaTrackingModule"));
  return fStatus;
}

//_____________________________________________________________________________
Int_t UserModule::Process( const THaEvData& /* evdata */ )
{
  // Calculate results.
  // This function is called for every physics event.

  // Quit if not successfully initialized
  if( !IsOK() ) return -1;

  // Get the track data from the spectrometer (or tracking module)
  THaTrackInfo* trkifo = fSpectro->GetTrackInfo();
  if( !trkifo ) return 1;

  // Calculate results.
  // As an example, we calculate the track projections onto a plane
  // which is located a distance fParameter away from the target 
  // (e.g. a sieve slit). The plane is assumed to be perpendicular to
  // the central ray, and tracks are assumed to be straight.
  // (Such simple results can also be obtained, more easily, using 
  // formulas in output.def.)
  // One would normally do susbtantially more complex work here.

  // "x_sieve"
  fResultA = trkifo->GetX() + fParameter * trkifo->GetTheta();
  // "y_sieve"
  fResultB = trkifo->GetY() + fParameter * trkifo->GetPhi();

  // That's it
  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t UserModule::ReadRunDatabase( const TDatime& date )
{
  // Read the parameters of this module from the run database
  // if the parameters were not given to the constructor or explicitly set.
  //
  // We search for a single parameter named "param".
  // First search the database for and entry "<prefix>.param", then, 
  // if not found, for "param". If still not found, use zero.

  // Call the run database reader of the base class. Currently, this
  // does nothing, but it might do something (e.g. initialization of
  // common parameters) in the future. 
  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  // Parameter already set? If so, nothing to do here
  if ( fParameter != 0.0 ) 
    return kOK;

  // Now for the real work...
  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

  // Search database for "<prefix>.param"
  TString name(fPrefix), tag("param"); name += tag;
  Int_t st = LoadDBvalue( f, date, name.Data(), fParameter );
  // Not found? Search for "param".
  if( st )
    st = LoadDBvalue( f, date, tag.Data(), fParameter );
  // Still not found? Use some default value.
  // (Alternatively, print message and return error code)
  if( st ) 
    fParameter = 0.0; //not really necessary since already zero
  
  fclose(f);
  return kOK;
}
  
//_____________________________________________________________________________
void UserModule::PrintInitError( const char* here )
{
  Error( Here(here), "Cannot set. Module already initialized." );
}

//_____________________________________________________________________________
void UserModule::SetParameter( Double_t value ) 
{
  if( !IsInit())
    fParameter = value; 
  else
    PrintInitError("SetParameter");
}

//_____________________________________________________________________________
void UserModule::SetSpectrometer( const char* name ) 
{
  if( !IsInit())
    fSpectroName = name; 
  else
    PrintInitError("SetSpectrometer");
}

//_____________________________________________________________________________
ClassImp(UserModule)

