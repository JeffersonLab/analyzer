////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   Abstract interface to VME modules
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "VmeModule.h"
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
  // Simplest version of IsSlot relies on a unique header.
  if ((rdata & fHeaderMask)==fHeader) {
    fWordsExpect = (rdata & fWdcntMask)>>fWdcntShift;
    return kTRUE;
  }
  return kFALSE;
}

ClassImp(Decoder::VmeModule)
