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
Int_t THaIdealBeam::ReadRunDatabase( const TDatime& date )
{
  // Query the run database for the position and direction information.
  // Tags:  <prefix>.x     (m)
  //        <prefix>.y     (m)
  //        <prefix>.theta (deg)
  //        <prefix>.phi   (deg)

  Int_t err = THaBeam::ReadRunDatabase( date );
  if( err ) return err;

  FILE* file = OpenRunDBFile( date );
  if( !file ) return kFileError;

  static const Double_t degrad = TMath::Pi()/180.0;

  Double_t x = 0.0, y = 0.0, th = 0.0, ph = 0.0;

  const TagDef tags[] = {
    { "x",     &x },
    { "y",     &y },
    { "theta", &th },
    { "phi",   &ph },
    { 0 }
  };
  LoadDB( file, date, tags, fPrefix );

  fPosition.SetXYZ( x, y, 0.0 );

  Double_t tt = TMath::Tan(degrad*th);
  Double_t tp = TMath::Tan(degrad*ph);
  fDirection.SetXYZ( tt, tp, 1.0 );
  fDirection *= 1.0/TMath::Sqrt( 1.0+tt*tt+tp*tp );
  
  fclose(file);
  return kOK;
}
