//*-- Author :    Bodo Reitz April 2003

//////////////////////////////////////////////////////////////////////////
//
// THaUnRasteredBeam
//
// Apparatus describing an unrastered beam.
//      can also be used if one only is interested 
//      in average beam position for a rastered beam
//      but in that latter case the event by event ext-target
//      corrections need to be skipped
//     (e.g. using an idealbeam for the hrs reconstruction and 
//      unrastered beam to fill global variables of the beamline)
// 
//////////////////////////////////////////////////////////////////////////

#include "THaUnRasteredBeam.h"
#include "THaBPM.h"
#include "TMath.h"
#include "TDatime.h"
#include "VarDef.h"
#include "TVector.h"


ClassImp(THaUnRasteredBeam)

//_____________________________________________________________________________

THaUnRasteredBeam::THaUnRasteredBeam( const char* name, const char* description ) :
    THaBeam( name, description ) 
{


  AddDetector( new THaBPM("BPMA","1st bpm",this) );
  AddDetector( new THaBPM("BPMB","2nd bpm",this) );
}


Int_t THaUnRasteredBeam::Reconstruct()
{

  // only the first two detectors in the list are used to get
  // get the beam position in two points, and to extrapolate that 
  // to the nominal target point
  // the following detectors are processed, but not used

  TVector3 p[2];

  TIter nextDet( fDetectors );
  nextDet.Reset();
  for( Int_t i = 0; i<2; i++ ) {
    THaBeamDet* theBeamDet = static_cast<THaBeamDet*>( nextDet() );
    if( theBeamDet ) {
      theBeamDet->Process();
      p[i] = theBeamDet->GetPosition();
    } else {
      Error( Here("Reconstruct()"), "Beamline Detectors Missing in Detector List" );
    }
  }

  // Process any other detectors we may have
  while (THaBeamDet * theBeamDet=
	 static_cast<THaBeamDet*>( nextDet() )) {
    theBeamDet->Process();
  }

  fDirection = p[1]-p[0];

  fPosition = p[1] + (p[1](2)/(p[0](2)-p[1](2))) * fDirection ;

  return 0;

}

