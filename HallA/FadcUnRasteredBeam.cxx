//*-- Author :  Ole Hansen 30 July 2021

//////////////////////////////////////////////////////////////////////////
//
// HallA::FadcUnRasteredBeam
//
// Apparatus describing an unrastered beam with FADC BPM readouts
//
//////////////////////////////////////////////////////////////////////////

#include "FadcUnRasteredBeam.h"
#include "FadcBPM.h"

namespace HallA {

//_____________________________________________________________________________
FadcUnRasteredBeam::FadcUnRasteredBeam( const char* name, const char* description,
                                        Int_t runningsum_depth )
  : THaUnRasteredBeam(name, description, runningsum_depth, false)
{
  AddDetector( new FadcBPM("BPMA","1st BPM", this) );
  AddDetector( new FadcBPM("BPMB","2nd BPM", this) );
}

//_____________________________________________________________________________

} // namespace HallA

////////////////////////////////////////////////////////////////////////////////
ClassImp(HallA::FadcUnRasteredBeam)
