//*-- Author :    Ole Hansen   11-May-04

//////////////////////////////////////////////////////////////////////////
//
// THaBeamEloss
//
// Correct energy loss of the beam by a constant. This is basically trivial,
// but useful to have to provide input for kinematics modules. 
// More advanced corrections can be implemented by inheritance from
// this module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeamEloss.h"
#include "THaBeam.h"
#include "THaVertexModule.h"
#include "TMath.h"
#include "TVector3.h"

using namespace std;

//_____________________________________________________________________________
THaBeamEloss::THaBeamEloss( const char* name, const char* description,
			    const char* input_beam ) :
  THaElossCorrection(name,description,input_beam), fBeamModule(NULL)
{
  // Normal constructor.

  Clear();
}

//_____________________________________________________________________________
THaBeamEloss::~THaBeamEloss()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaBeamEloss::CalcEloss( THaBeamInfo* beamifo )
{
  // Compute the mean energy loss for the beam given in beamifo.
  //
  // Currently, we use a very simple algorithm that computes
  // the energy loss based on a fixed material thickness.
  // Only the beta dependence of the energy loss is used,
  // but there are no corrections for angle, pathlength etc.
  //
  // May be overridden by derived classes as necessary.

  Double_t p0 = beamifo->GetP();
  Double_t beta = p0 / TMath::Sqrt(p0*p0 + fM*fM);
  if( fElectronMode ) {
    fEloss = ElossElectron( beta, fZmed, fAmed, 
			    fDensity, fPathlength );
  } else {
    fEloss = ElossHadron( fZ, beta, fZmed, fAmed, 
			  fDensity, fPathlength );
  }
}

//_____________________________________________________________________________
void THaBeamEloss::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaElossCorrection::Clear(opt);
  BeamIfoClear();
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaBeamEloss::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the input beam module named in fInputName and save pointer to it. 
  // Extract mass and charge of the beam particles from the input.

  static const char* const here = "Init()";

  // Find the input beam module
  fBeamModule = dynamic_cast<THaBeamModule*>
    ( FindModule( fInputName.Data(), "THaBeamModule"));
  if( !fBeamModule )
    return fStatus;

  // Get the parent beam apparatus from the input module
  // NB: FindModule() above already checked initialization
  THaBeamInfo* beamifo = fBeamModule->GetBeamInfo();
  THaBeam* beam = beamifo->GetBeam();
  if( !beam ) {
    Error( Here(here), "Oops. Input beam module has no pointer to "
	   "a beam apparatus?!?" );
    return fStatus = kInitError;  
  }
  // Needed for initialization of dependent modules in a chain
  fBeamIfo.SetBeam(beam);

  // Get beam particle properties from the input module. 
  // Overrides anything set by SetMass()
  SetMass( beamifo->GetM() );
  fZ = TMath::Abs(beamifo->GetQ());

  // Standard initialization. Calls this object's DefineVariables().
  // Reads material parameters from the run database
  THaElossCorrection::Init( run_time );

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaBeamEloss::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  THaElossCorrection::DefineVariables( mode );

  // Define the variables for the beam info subobject
  return DefineVarsFromList( THaBeamModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
Int_t THaBeamEloss::Process( const THaEvData& )
{
  // Calculate corrections and adjust the track parameters.

  if( !IsOK() ) return -1;

  THaBeamInfo* beamifo = fBeamModule->GetBeamInfo();
  if( !beamifo->IsOK() ) return 2;

  // Copy the input track info
  fBeamIfo = *beamifo;
  
  // Compute the correction. Remember that we want to compute the
  // beam energy at the reaction vertex, so we need to subtract the
  // energy loss from the input energy.
  Double_t p_in = fBeamIfo.GetP(); 
  if( p_in <= 0.0 ) return 4; //oops
  Double_t E_in = TMath::Sqrt(p_in*p_in + fM*fM);
  if( !fTestMode ) {
    // calculate pathlength for this event if we have a vertex module
    if( fExtPathMode ) {
      if( !fVertexModule->HasVertex() )
	return 1;
      fPathlength = 
	TMath::Abs(fVertexModule->GetVertex().Z() - fZref) * fScale;
    }
    //update fEloss
    CalcEloss(beamifo);
  }
  Double_t p_out = TMath::Sqrt(p_in*p_in + fEloss*fEloss - 2.0*E_in*fEloss);
  
  // Apply the corrected momentum to our beam info
  fBeamIfo.SetP(p_out);

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaBeamEloss)

