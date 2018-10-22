//*-- Author :    Ole Hansen 25-Mar-2003

//////////////////////////////////////////////////////////////////////////
//
// THaIdealBeam
//
// Apparatus describing an ideal beam - constant position and direction.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaIdealBeam.h"
#include "TMath.h"
#include "TDatime.h"
#include "VarDef.h"

ClassImp(THaIdealBeam)

//_____________________________________________________________________________
THaIdealBeam::THaIdealBeam( const char* name, const char* description )
  : THaBeam( name, description )
{
  // Constructor. Sets default beam position to (0,0,0) and direction to
  // (0,0,1) (along z-axis)

  fDirection.SetXYZ(0.0,0.0,1.0);
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaIdealBeam::Init( const TDatime& run_time )
{
  // Module initialization: get run parameters, then read databases. Calls
  // our ReadRunDatabase below and sets our fixed position & direction.

  if( THaBeam::Init(run_time) )
    return fStatus;

  // Copy our beam data to our beam info variables. This can be done here
  // instead of in Reconstruct() because the info is constant.
  Update();
  return kOK;
}

//_____________________________________________________________________________
Int_t THaIdealBeam::ReadRunDatabase( const TDatime& date )
{
  // Query the run database for the position and direction information.
  // Tags:  <prefix>.x     (m)
  //        <prefix>.y     (m)
  //        <prefix>.theta (deg)
  //        <prefix>.phi   (deg)
  //
  // If no tags exist, position and direction will be set to (0,0,0) 
  // and (0,0,1), respectively.

  Int_t err = THaBeam::ReadRunDatabase( date );
  if( err ) return err;

  FILE* file = OpenRunDBFile( date );
  if( !file ) return kFileError;

  static const Double_t degrad = TMath::Pi()/180.0;

  Double_t x = 0.0, y = 0.0, th = 0.0, ph = 0.0;

  const DBRequest req[] = {
    { "x",     &x,  kDouble, 0, 1 },
    { "y",     &y,  kDouble, 0, 1 },
    { "theta", &th, kDouble, 0, 1 },
    { "phi",   &ph, kDouble, 0, 1 },
    { 0 }
  };
  err = LoadDB( file, date, req );
  if( err )
    return kInitError;

  fPosition.SetXYZ( x, y, 0.0 );

  Double_t tt = TMath::Tan(degrad*th);
  Double_t tp = TMath::Tan(degrad*ph);
  fDirection.SetXYZ( tt, tp, 1.0 );
  fDirection *= 1.0/TMath::Sqrt( 1.0+tt*tt+tp*tp );
  
  fclose(file);
  return kOK;
}
