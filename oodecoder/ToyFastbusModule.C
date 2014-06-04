////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "DecoderGlobals.h"
#include "ToyFastbusModule.h"
#include "ToyModule.h"
//#include "THaEvData.h"
#include "TMath.h"
#include "Rtypes.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

ToyFastbusModule::ToyFastbusModule(Int_t crate, Int_t slot) : ToyModule(crate, slot) {
  if (fCrate < 0 || fCrate > MAXROC) {
       cerr << "ERROR: crate out of bounds"<<endl;
       fCrate = 0;
  }
  if (fSlot < 0 || fSlot > MAXSLOT_FB) {
       cerr << "ERROR: slot out of bounds"<<endl;
       fSlot = 0;
  }
}

ToyFastbusModule::~ToyFastbusModule() {
}


Int_t ToyFastbusModule::Decode(const Int_t *evbuffer) {
  fChan = Chan(*evbuffer);
  fData = Data(*evbuffer);
  fRawData = fData;
}




ClassImp(ToyFastbusModule)
