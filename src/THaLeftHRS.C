//*-- Author :    Ole Hansen   17-May-00

//////////////////////////////////////////////////////////////////////////
//
// THaLeftHRS
//
// The standard Hall A Left arm HRS. Contains the following detectors:
//
//  - VDC  "L.vdc"
//  - S1   "L.s1"
//  - S2   "L.s2" 
//
//////////////////////////////////////////////////////////////////////////

#include "THaLeftHRS.h"
#include "THaVDC.h"
#include "THaScintillator.h"

static const char* const MYNAME = "L";

ClassImp(THaLeftHRS)

//_____________________________________________________________________________
THaLeftHRS::THaLeftHRS( const char* description ) 
  : THaHRS( MYNAME, description )
{
  // Constructor. Defines detectors used by this apparatus.

  fNmydets    = 3;
  fMydets     = new THaDetector*[fNmydets];
  fMydets[0]  = new THaScintillator("s1","L-arm S1");
  fMydets[1]  = new THaScintillator("s2","L-arm S2");
  fMydets[2]  = new THaVDC("vdc","L-arm VDC");

  for( int i=0; i<fNmydets; i++ ) {
    fMydets[i]->SetApparatus(this);
    fDetectors->AddLast( fMydets[i] );
  }
}

