////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "ToyFastbusModule.h"
#include "ToyModule.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyFastbusModule::ToyFastbusModule() { 
}

// open question: why must I declare this in header if I declared
// it in base class header ???
Int_t ToyFastbusModule::Decode(THaEvData *evdata, Int_t jstart) {
  std::cout<<"ToyFastbusModule decode from "<<jstart<<std::endl;
  
  return 0;
}

ToyFastbusModule::~ToyFastbusModule() { 
}

Bool_t ToyFastbusModule::IsSlot(Int_t slot, Int_t rdata) {
  return kTRUE;
}


ClassImp(ToyFastbusModule)
