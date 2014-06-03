////////////////////////////////////////////////////////////////////
//
//   ToyModuleX
//   This toy doesn't do anything; look at ToyFastbusModule instead
//
/////////////////////////////////////////////////////////////////////

#include "ToyModuleX.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


ToyModuleX::ToyModuleX(Int_t crate, Int_t slot) { 
  fCrate = crate;
  fSlot = slot;
}

ToyModuleX::~ToyModuleX() { 
}


ClassImp(ToyModuleX)
