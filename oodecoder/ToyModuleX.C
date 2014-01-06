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

ToyModuleX::ToyModuleX() { 
}

ToyModuleX::ToyModuleX(Int_t crate, Int_t slot) { 
  fCrate = crate;
  fSlot = slot;
}

ToyModuleX::~ToyModuleX() { 
}

Int_t ToyModuleX::Decode(THaEvData *evdata, Int_t jstart) {
  std::cout<<"ToyModuleX decode from "<<jstart<<std::endl;
  
  return 0;
}

Bool_t ToyModuleX::IsSlot(Int_t rdata) {
  return kFALSE;
}

ClassImp(ToyModuleX)
