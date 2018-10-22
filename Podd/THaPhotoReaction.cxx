//*-- Author:   LEDEX collaboration   June 2006

// THaPhotoReaction
//
// This module calculates the energy of an incident REAL photon
// in a deuteron photodisintegration reaction g(d,p)n from the detected
// proton energy. It also calculates the CM scattering angle of the 
// detected proton (used for Wigner rotation calculations) 
//

#include "THaPhotoReaction.h"
#include "THaTrackingModule.h"
#include "THaBeam.h"
#include "VarDef.h"
#include "TMath.h"

using namespace std;

//FIXME: Assume deuterium target for now
static const Double_t target_mass = 1.87561; 

//_____________________________________________________________________________
THaPhotoReaction::THaPhotoReaction( const char* name, const char* description,
				    const char* spectro )
//				    Double target_mass )
  : THaPhysicsModule(name,description),
    fEGamma(kBig), fScatAngle(kBig), fScatAngleCM(kBig), fMA(target_mass),
    fSpectroName(spectro), fSpectro(0), fBeam(0)
{
  // Standard constructor. Assume an ideal beam along z_lab.
}

//_____________________________________________________________________________
THaPhotoReaction::THaPhotoReaction( const char* name, const char* description,
				    const char* spectro, const char* beam )
//				    Double_t target_mass ) 
  : THaPhysicsModule(name,description),
    fEGamma(kBig), fScatAngle(kBig), fScatAngleCM(kBig), fMA(target_mass),
    fSpectroName(spectro), fBeamName(beam), fSpectro(0), fBeam(0)
{
  // Constructor with specification of optional beam module.
}


//_____________________________________________________________________________
THaPhotoReaction::~THaPhotoReaction()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void THaPhotoReaction::Clear( Option_t* opt )
{
  // Clear physics variables in preparation for next event

  THaPhysicsModule::Clear(opt);
  fScatAngle = fScatAngleCM = fEGamma = kBig;
}

//_____________________________________________________________________________
Int_t THaPhotoReaction::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "EGamma",   "Real Brem. Photon Energy (GeV)", "fEGamma"},
    { "angle",    "Scattering Angle (rad)",         "fScatAngle"},
    { "angle_cm", "Scattering Angle(rad) in CM",    "fScatAngleCM"},
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaPhotoReaction::Init( const TDatime& run_time )
{
  // Initialize the module. Do standard initialization, then
  // locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  fSpectro = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSpectroName.Data(), "THaTrackingModule"));
  if( !fSpectro )
    return fStatus;

  // Optional beam apparatus
  if( fBeamName.Length() > 0 ) {
    fBeam = dynamic_cast<THaBeamModule*>
      ( FindModule( fBeamName.Data(), "THaBeamModule") );
    if( !fBeam )
      return fStatus;
  }

  // Standard initialization. Calls this object's DefineVariables().
  THaPhysicsModule::Init( run_time );

 return fStatus;
}

//_____________________________________________________________________________
Int_t THaPhotoReaction::Process( const THaEvData& )
{
  const Double_t Mn = 0.939565;
  const Double_t Mp = 0.938272;
 
  // This Procedure calculates the energy of the (REAL)
  // photon from the detected proton momentum 

  if( !IsOK() || !gHaRun ) return -1;

  // Get tracking info of detected proton
  THaTrackInfo* trkifo = fSpectro->GetTrackInfo();
  if( !trkifo || !trkifo->IsOK() ) return 1;

  // Two relevant 4-vectors here:
  // fP0 = incident photon
  // fP1 = detected proton

  // Direction of incident gamma assumed along the beam
  TVector3 p0(0.,0.,1.);
  if( fBeam )
    p0 = (fBeam->GetBeamInfo()->GetPvect()).Unit();

  fP1.SetVectM( trkifo->GetPvect(), Mp );

  fScatAngle = p0.Angle( fP1.Vect() );

  // Calculate E_gamma from energy conservation
  Double_t E1    = fP1.E();
  Double_t Ein2  = fMA*fMA+Mp*Mp-Mn*Mn-2*fMA*E1;
  fEGamma        = 0.5*Ein2/(E1-fMA-fP1.Z());

  // FIXME: this is not correct if the beam is not along z_lab
  // FIXME: use Boost() & Angle()
  double betacm  = fEGamma/(fEGamma+fMA);
  double gammacm = 1./(sqrt(1-betacm*betacm));
  double pzcm    = fP1.Z()*gammacm-E1*betacm*gammacm;
  fScatAngleCM   = acos( pzcm/sqrt( pow(pzcm,2)+
				    pow(fP1.X(),2)+
				    pow(fP1.Y(),2)
				    )
			 );
  
  fDataValid = true;
 
  return 0;
}

//_____________________________________________________________________________
// Int_t THaPhotoReaction::ReadRunDatabase( const TDatime& date )
// {

//   Int_t err = THaPhysicsModule::ReadRunDatabase( date );
//   if( err ) return err;

//   if ( fMA > 0.0 ) 
//     return kOK;

//   FILE* f = OpenRunDBFile( date );
//   if( !f ) return kFileError;

//   if( fMA <= 0.0 ) {
//     TString name(fPrefix), tag("MA"); name += tag;
//     Int_t st = LoadDBvalue( f, date, name.Data(), fMA );
//     if( st )
//       LoadDBvalue( f, date, tag.Data(), fMA );
//     if( fMA <= 0.0 ) fMA = 0.938;
//   }

//   fclose(f);
//   return kOK;
// }
  
//_____________________________________________________________________________
// void THaPhotoReaction::SetTargetMass( Double_t m ) 
// {
//   if( !IsInit())
//     fMA = m; 
//   else
//     PrintInitError("SetTargetMass()");
// }

//_____________________________________________________________________________
void THaPhotoReaction::SetSpectrometer( const char* name ) 
{
  if( !IsInit())
    fSpectroName = name; 
  else
    PrintInitError("SetSpectrometer()");
}

//_____________________________________________________________________________
void THaPhotoReaction::SetBeam( const char* name ) 
{
  if( !IsInit())
    fBeamName = name; 
  else
    PrintInitError("SetBeam()");
}

ClassImp(THaPhotoReaction)

