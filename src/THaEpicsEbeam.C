//*-- Author :    Ole Hansen 07-May-2004

//////////////////////////////////////////////////////////////////////////
//
// THaEpicsEbeam
//
// A physics module that provides beam energy/momentum based on
// EPCIS information
// 
//////////////////////////////////////////////////////////////////////////

#include "THaEpicsEbeam.h"
#include "VarDef.h"
#include "THaEvData.h"
#include "TMath.h"

//_____________________________________________________________________________
THaEpicsEbeam::THaEpicsEbeam( const char* name, const char* description,
			      const char* beam, const char* epics_var,
			      Double_t scale_factor ) : 
  THaPhysicsModule( name,description ), fEcorr(0.0), fEpicsIsMomentum(kFALSE),
  fScaleFactor(scale_factor), fBeamName(beam), fEpicsVar(epics_var), 
  fBeamModule(NULL)
{
  // Constructor.
}


//_____________________________________________________________________________
THaEpicsEbeam::~THaEpicsEbeam()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void THaEpicsEbeam::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaPhysicsModule::Clear(opt);
  BeamIfoClear();
  fEcorr = 0.0;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaEpicsEbeam::Init( const TDatime& run_time )
{
  // Initialize the module.

  fBeamModule = dynamic_cast<THaBeamModule*>
    ( FindModule( fBeamName.Data(), "THaBeamModule"));
  if( !fBeamModule )
    return fStatus;

  //this is done by THaBeamInfo::operator= in Process
  //  fBeamIfo.SetBeam( fBeamModule->GetBeamInfo()->GetBeam() );

  return THaPhysicsModule::Init(run_time);
}
  
//_____________________________________________________________________________
Int_t THaEpicsEbeam::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "ecorr", "Beam energy correction (output-input) (GeV)", "fEcorr" },
    { 0 }
  };
  DefineVarsFromList( vars, mode );

  // Define the variables for the beam info subobject
  return DefineVarsFromList( GetRVarDef(), mode );
}

//_____________________________________________________________________________
Int_t THaEpicsEbeam::Process( const THaEvData& evdata )
{
  // Obtain actual beam energy from EPICS (if available) and 
  // calculate new beam parameters

  if( !IsOK() ) return -1;

  THaBeamInfo* input = fBeamModule->GetBeamInfo();
  if( !input->IsOK() ) return 2;

  // Copy the input beam info
  fBeamIfo = *input;

  // Obtain current beam energy (or momentum) from EPICS
  // If requested EPICS variable not loaded, do nothing
  if( evdata.IsLoadedEpics(fEpicsVar) ) {
    Double_t e = evdata.GetEpicsData(fEpicsVar);
    // the scale factor must convert the EPICS value to GeV
    e *= fScaleFactor;
    Double_t m = fBeamIfo.GetM();
    Double_t p;
    if( fEpicsIsMomentum ) {
      p = e;
      e = TMath::Sqrt(p*p+m*m);
    } else {
      p = TMath::Sqrt( TMath::Max(e*e-m*m,0.0) );
    }
    fBeamIfo.SetP(p);
    fEcorr = e - input->GetE();
  }
  
  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
void THaEpicsEbeam::SetBeam( const char* name ) 
{
  // Set the name of the module providing the beam input data
  if( !IsInit())
    fBeamName = name; 
  else
    PrintInitError("SetBeam");
}

//_____________________________________________________________________________
void THaEpicsEbeam::SetEpicsVar( const char* name ) 
{
  // Set the name of the EPICS variable to use for corrections
  if( !IsInit())
    fEpicsVar = name; 
  else
    PrintInitError("SetEpicsVar");
}

//_____________________________________________________________________________
void THaEpicsEbeam::SetEpicsIsMomentum( Bool_t mode ) 
{
  // If mode=kTRUE, interpret EPICS data as momentum rather than energy
  if( !IsInit())
    fEpicsIsMomentum = mode; 
  else
    PrintInitError("SetEpicsIsMomentum");
}

//_____________________________________________________________________________
void THaEpicsEbeam::SetScaleFactor( Double_t factor ) 
{
  // If mode=kTRUE, interpret EPICS data as momentum rather than energy
  if( !IsInit())
    fScaleFactor = factor;
  else
    PrintInitError("SetEpicsIsMomentum");
}

//_____________________________________________________________________________
ClassImp(THaEpicsEbeam)


