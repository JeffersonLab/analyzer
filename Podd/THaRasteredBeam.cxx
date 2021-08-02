//*-- Author :    Bodo Reitz April 2003

///////////////////////////////////////////////////////////////////////////////
//
// THaRasteredBeam
//
// Apparatus describing a rastered beam that is analyzed using event-by-event
// raster currents.
//
///////////////////////////////////////////////////////////////////////////////

#include "THaRasteredBeam.h"
#include "THaRaster.h"
#include "TList.h"

ClassImp(THaRasteredBeam)

//_____________________________________________________________________________
THaRasteredBeam::THaRasteredBeam( const char* name, const char* description,
                                  bool do_setup )
  : THaBeam( name, description )
{
  if( do_setup )
    AddDetector(new THaRaster("Raster", "raster", this));
}

//_____________________________________________________________________________
Int_t THaRasteredBeam::Reconstruct()
{

  TIter nextDet( fDetectors ); 
  nextDet.Reset();

  // This apparatus assumes that there is only one detector 
  // in the list. If someone adds detectors by hand, the first 
  // detector in the list will be used to get the beam position
  // the others will be processed

  if( auto* theBeamDet = dynamic_cast<THaBeamDet*>( nextDet() ) ) {
    theBeamDet->Process();
    fPosition = theBeamDet->GetPosition();
    fDirection = theBeamDet->GetDirection();
  }
  else {
    Error( Here("Reconstruct"), 
	   "Beamline Detectors Missing in Detector List" );
  }

  // Process any other detectors that may have been added (by default none)
  while( TObject* obj = nextDet() ) {
    if( auto* theBeamDet = dynamic_cast<THaBeamDet*>(obj) )
      theBeamDet->Process();
  }

  Update();

  return 0;
}
