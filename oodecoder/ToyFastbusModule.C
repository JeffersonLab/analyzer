///////////////////////////////////////////////////////////////////
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
  fSlotMask=0xf8000000;
  fSlotShift=27;
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

void ToyFastbusModule::Init(Int_t crate, Int_t slot, Int_t i1, Int_t i2) {
  cout << "Initializeing FB module in crate "<<crate<<"     slot "<<slot<<endl;
  fCrate = crate;
  fSlot = slot;
  fSlotMask = 0xf8000000;
  fSlotShift = 27;
}

Int_t ToyFastbusModule::Decode(const Int_t *evbuffer) {
  fChan = Chan(*evbuffer);
  fData = Data(*evbuffer);
  fRawData = fData;
}

Int_t ToyFastbusModule::LoadSlot(THaSlotData *sldat, const Int_t* evbuffer) {
// this increments evbuffer
  cout << "ToyFastbusModule:: loadslot "<<endl; 
  fWordsSeen = 0;
  while (IsSlot( *evbuffer )) {
    if (fHasHeader && fWordsSeen==0) {
      fWordsSeen++;
    } else {
      Decode(evbuffer);
      sldat->loadData(fChan, fData, fRawData);
      fWordsSeen++;
      evbuffer++;
      cout << "hi, evbuff "<<evbuffer<<"   "<<hex<<*evbuffer<<dec<<endl;
      // Need to prevent runaway
    }
  }
  return 1;
}

void ToyFastbusModule::DoPrint() {

  cout << "ToyFastbusModule   name = "<<fName<<endl;
  cout << "Crate  "<<fCrate<<"     slot "<<fSlot<<endl;

}


ClassImp(ToyFastbusModule)
