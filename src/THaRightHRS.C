//*-- Author :    Ole Hansen   2-Oct-01

//////////////////////////////////////////////////////////////////////////
//
// THaRightHRS
//
// The standard Hall A right arm HRS. Contains the following detectors:
//
//  - VDC                     "R.vdc"
//  - S1                      "R.s1"
//  - S2                      "R.s2"
//  - A1 aerogel Cherenkov    "R.aero1"
//  - Gas Cherenkov           "R.cer"
//  - Total shower counter    "R.ts"
//    + Preshower             "R.ps"
//    + Shower                "R.sh"
//
//////////////////////////////////////////////////////////////////////////

#include "THaRightHRS.h"
#include "THaVDC.h"
#include "THaScintillator.h"
#include "THaCherenkov.h"
#include "THaTotalShower.h"

static const char* const MYNAME = "R";

ClassImp(THaRightHRS)

//_____________________________________________________________________________
THaRightHRS::THaRightHRS( const char* description ) 
  : THaHRS( MYNAME, description )
{
  // Constructor. Defines detectors used by this apparatus.

  fNmydets    = 3;
  fMydets     = new THaDetector*[fNmydets];
  fMydets[0]  = new THaScintillator("s1","R-arm S1");
  fMydets[1]  = new THaScintillator("s2","R-arm S2");
  fMydets[2]  = new THaVDC("vdc","R-arm VDC");

  for( int i=0; i<fNmydets; i++ ) {
    fMydets[i]->SetApparatus(this);
    fDetectors->AddLast( fMydets[i] );
  }
}
