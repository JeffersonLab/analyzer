//*-- Author :   Ole Hansen 24-Feb-03

//////////////////////////////////////////////////////////////////////////
//
//   THaScalerGroup
//
//   A group of scaler banks. Typically, there is one scaler 
//   group for each spectrometer.
//
//   This class is a wrapper for the standalone THaScaler class.
//   The wrapper is required to let scalers be a THaAnalysisObject 
//   without breaking the THaScaler standalone package.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaScalerGroup.h"
#include "TString.h"

ClassImp(THaScalerGroup)

//_____________________________________________________________________________
THaScalerGroup::THaScalerGroup( const char* bank ) : THaAnalysisObject("","")
{
  // Constructor.

  fScaler = new THaScaler( bank );
  if( !fScaler || fScaler->IsZombie() ) {
    MakeZombie();
    return;
  }

  TString description(bank);
  description += " scaler group";

  TString name(bank);
  name.ToLower();
  if( name == "left" || name == "l" )
    name = "LS";
  else if( name == "right" || name == "r" )
    name = "RS";
  else if( name == "evleft" )
    name = "EL";
  else if( name == "evright" )
    name = "ER";
  else if( name == "rcs" )
    name = "RCSS";
    
  SetNameTitle( name, description );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaScalerGroup::Init( const TDatime& date )
{
  // Initialize the scaler group.

  if( IsZombie() || !fScaler ) return (fStatus = kInitError);

  Int_t status = fScaler->Init( date );
  if( status == SCAL_ERROR )
    fStatus = kInitError;
  else
    fStatus = kOK;

  return fStatus;
}

//_____________________________________________________________________________
THaScalerGroup::~THaScalerGroup()
{
  // Destructor

  if( fIsSetup )
    RemoveVariables();

  delete fScaler;
}

