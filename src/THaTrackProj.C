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
#include <iostream>

using namespace std;

const Double_t THaTrackProj::kBig = 1.e38;

//_____________________________________________________________________________
void THaTrackProj::Clear( Option_t* )
{
  // Reset per-event data.

  fX = fY = fPathl = fdX = kBig;
  fChannel = -1;
  fIsOK = false;
}

//_____________________________________________________________________________
void THaTrackProj::Print( Option_t* opt ) const
{
  // Print track projection info

  TObject::Print(opt);
  cout << "X/Y/dX = " << fX    << "/" << fY << "/" << fdX << " m"
       << endl
       << "pathl/chan/good = " << fPathl << "/" << fChannel << "/" << fIsOK
       << endl;
}

//_____________________________________________________________________________
ClassImp(THaTrackProj)

