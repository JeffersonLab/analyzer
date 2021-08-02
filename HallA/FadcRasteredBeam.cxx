//*-- Author :  Ole Hansen 16 July 2021

//////////////////////////////////////////////////////////////////////////
//
// HallA::FadcRasteredBeam
//
// Apparatus describing a rastered beam with FADC raster current readout
//
//////////////////////////////////////////////////////////////////////////

#include "FadcRasteredBeam.h"
#include "FadcBPM.h"
#include "FadcRaster.h"

namespace HallA {

//_____________________________________________________________________________
FadcRasteredBeam::FadcRasteredBeam( const char* name, const char* description )
  : THaRasteredBeam(name, description, false)
{
  AddDetector( new FadcRaster("Raster2","downstream raster", this) );
  AddDetector( new FadcRaster("Raster","upstream raster", this) );
  AddDetector( new FadcBPM("BPMA","1st BPM", this) );
  AddDetector( new FadcBPM("BPMB","2nd BPM", this) );
}

//_____________________________________________________________________________

} // namespace HallA

////////////////////////////////////////////////////////////////////////////////
ClassImp(HallA::FadcRasteredBeam)
