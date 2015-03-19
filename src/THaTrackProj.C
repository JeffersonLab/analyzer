//*-- Author :    Robert Feuerbach   17-Oct-2003

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// THaTrackProj                                                             //
//                                                                          //
// Track coordinates projected to a (non-tracking) detector plane           //
// Optionally holds detector block/paddle/PMT number and position residual  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include "THaTrackProj.h"

const Double_t THaTrackProj::kBig = 1.e38;

//_____________________________________________________________________________
ClassImp(THaTrackProj)

