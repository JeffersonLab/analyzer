////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   Abstract interface to VME modules
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


VmeModule::VmeModule(Int_t crate, Int_t slot) : ToyModule(crate,slot) { 
}

VmeModule::~VmeModule() { 
}

Bool_t VmeModule::IsSlot(UInt_t rdata) {
  return ((rdata & fHeaderMask)==fHeader);
}


ClassImp(VmeModule)
