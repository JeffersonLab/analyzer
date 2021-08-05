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
//     (e.g. using an ideal beam for the hrs reconstruction and
//      unrastered beam to fill global variables of the beamline)
// 
//////////////////////////////////////////////////////////////////////////

#include "THaUnRasteredBeam.h"
#include "THaBPM.h"
#include "TList.h"

using namespace std;

ClassImp(THaUnRasteredBeam)

//_____________________________________________________________________________

THaUnRasteredBeam::THaUnRasteredBeam( const char* name, const char* description,
                                      Int_t runningsum_depth, bool do_setup )
  : THaBeam( name, description ), fRunningSumDepth(runningsum_depth),
    fRunningSumWrap(false), fRunningSumNext(0)
{


  if( do_setup ) {
    AddDetector( new THaBPM("BPMA","1st bpm",this) );
    AddDetector( new THaBPM("BPMB","2nd bpm",this) );
  }

  if (fRunningSumDepth>1) {
    fRSPosition.clear();
    fRSDirection.clear();
    fRSPosition.resize(fRunningSumDepth);
    fRSDirection.resize(fRunningSumDepth);
  } else {
    fRunningSumDepth=0;
  }
}


//_____________________________________________________________________________
Int_t THaUnRasteredBeam::Reconstruct()
{

  // only the first two detectors in the list are used to get
  // the beam position in two points, and to extrapolate that
  // to the nominal target point
  // the following detectors are processed, but not used

  TVector3 pos[2];

  TIter nextDet( fDetectors );
  nextDet.Reset();
  for( auto& thePos : pos ) {
    auto* theBeamDet = dynamic_cast<THaBeamDet*>( nextDet() );
    if( theBeamDet ) {
      theBeamDet->Process();
      thePos = theBeamDet->GetPosition();
    } else {
      Error( Here("Reconstruct"), 
	     "Beamline Detectors Missing in Detector List" );
    }
  }

  // Process any other detectors we may have
  while( auto* theBeamDet = dynamic_cast<THaBeamDet*>(nextDet()) ) {
    theBeamDet->Process();
  }

  if( fRunningSumDepth != 0 ) {


    if (fRunningSumWrap) {
      fRSAvPos = fRSAvPos - ( ( 1./fRunningSumDepth) * fRSPosition[fRunningSumNext] );
      fRSAvDir = fRSAvDir - ( ( 1./fRunningSumDepth) * fRSDirection[fRunningSumNext]);

    }      

    fRSDirection[fRunningSumNext] = pos[1] - pos[0] ;
    fRSPosition[fRunningSumNext]  = pos[1] +
                                    (pos[1](2) / (pos[0](2) - pos[1](2))) * fRSDirection[fRunningSumNext] ;

    if (fRunningSumWrap) {
      fRSAvPos = fRSAvPos + ( ( 1./fRunningSumDepth) * fRSPosition[fRunningSumNext] );
      fRSAvDir = fRSAvDir +  (( 1./fRunningSumDepth) * fRSDirection[fRunningSumNext]);
    } else {
      if (fRunningSumNext==0) { 
	fRSAvPos = fRSPosition[fRunningSumNext];
	fRSAvDir = fRSDirection[fRunningSumNext];
      } else {
	fRSAvPos = ((double)fRunningSumNext/(fRunningSumNext+1.))*fRSAvPos 
	  + ( (1./(fRunningSumNext+1)) * fRSPosition[fRunningSumNext] );
	fRSAvDir = ((double)fRunningSumNext/(fRunningSumNext+1.))*fRSAvDir 
	  + ( (1./(fRunningSumNext+1)) * fRSDirection[fRunningSumNext] );
      }
    }
    fRunningSumNext++;
    if (fRunningSumNext==fRunningSumDepth) {
      fRunningSumNext = 0;
      fRunningSumWrap = true ;
    }
    fDirection = fRSAvDir;
    fPosition =  fRSAvPos;

  } else {

    fDirection = pos[1] - pos[0];
    fPosition = pos[1] + (pos[1](2) / (pos[0](2) - pos[1](2))) * fDirection ;

  }
  Update();

  return 0;

}

//_____________________________________________________________________________
void  THaUnRasteredBeam::ClearRunningSum()
{
  fRunningSumNext = 0 ;
  fRunningSumWrap = false ;
}
