////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   Abstract interface to VME modules
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace Decoder;


VmeModule::VmeModule(Int_t crate, Int_t slot) : Module(crate,slot) { 
}

VmeModule::~VmeModule() { 
}

Bool_t VmeModule::IsSlot(UInt_t rdata) {
  return ((rdata & fHeaderMask)==fHeader);
}


ClassImp(Decoder::VmeModule)
