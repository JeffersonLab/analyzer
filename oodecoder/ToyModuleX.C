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


ToyModuleX::ToyModuleX(Int_t crate, Int_t slot) : ToyModule(crate,slot) { 
}

ToyModuleX::~ToyModuleX() { 
}

Int_t ToyModuleX::Decode(const Int_t *evbuffer) {
  fChan = 0;
  fData = Data(*evbuffer);
  fRawData = fData;
}

Bool_t ToyModuleX::IsSlot(UInt_t rdata) {
  return ((rdata & fHeaderMask)==fHeader);
}


ClassImp(ToyModuleX)
