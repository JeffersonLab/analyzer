//*-- Author :    Chris Behre   29 June 2000

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
// The Hall A BPM and Raster data class.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"
#include "THaBpm.h"
#include "THaRaster.h"
#include "THaEpicsData.h"

#include <iostream.h>

const char* const myName = "B";

ClassImp(THaBeam)

//_____________________________________________________________________________
THaBeam::THaBeam( const char* descript ) : THaApparatus( myName, descript )
{
  // Constructor. Defines detectors used by this apparatus.

  fNmydets    = 4;
  fMydets     = new THaDetector*[fNmydets];
  fMydets[0]  = new THaBpm("bpm4a","BPM 4a");
  fMydets[1]  = new THaBpm("bpm4b","BPM 4b");
  fMydets[2]  = new THaRaster("rast","Raster");
  fMydets[3]  = new THaEpicsData("epics","EPICS data");

  for( int i=0; i<fNmydets; i++ ) {
    fMydets[i]->SetApparatus(this);
    fDetectors->AddLast( fMydets[i] );
  }
}

//_____________________________________________________________________________
Int_t THaBeam::Reconstruct()
{
  // Currently does nothing. Returns zero.

  return 0;
}
