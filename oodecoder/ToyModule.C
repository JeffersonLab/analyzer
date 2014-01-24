////////////////////////////////////////////////////////////////////
//
//   ToyModule
//
//   Abstract class
//
/////////////////////////////////////////////////////////////////////

#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyModule::ToyModule() { 
}

ToyModule::ToyModule(Int_t crate, Int_t slot) { 
  fCrate = crate;
  fSlot = slot;
}

ToyModule::~ToyModule() { 
}


ToyModule::ToyModule(const ToyModule& rhs) {
}; 



ClassImp(ToyModule)
