////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "ToyFastbusModule.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

ToyFastbusModule::ToyFastbusModule(Int_t crate, Int_t slot) fCrate(crate), fSlot(slot) {
  if (fCrate < 0 || fCrate > MAXROC) {
       cerr << "ERROR: crate out of bounds"<<endl;
       fCrate = 0;
  }
  if (fSlot < 0 || fSlot > 26) {
       cerr << "ERROR: slot out of bounds"<<endl;
       fSlot = 0;
  }
}

ToyFastbusModule::ToyFastbusModule() : fChanMask(0xfe0000), fDataMask(0xffff), fWdcntMask(0x7ff), fChanShift(17) {  
  fCrate = 0;
  fSlot = 0;
}



Int_t ToyFastbusModule::Clear() {
  ndata = 0;
}

Int_t ToyFastbusModule::Decode(Int_t rdata) {
  
  cout<<"ToyFastbusModule decode  "<<endl;
  cout << "Crate "<<fCrate<<"   slot "<<fSlot<<endl;
  if (IsSlot(rdata) {
     Int_t chan = (rdata & fChanMask) >> fChanShift;
     Int_t dat  = (rdata & fDataMask) >> fDataShift;
     if (chan < 0 || chan > MAXCHAN) {
	// error
     }
     // increment hits
     // print stuff to debug

    }
    
  return 0;
}

ToyFastbusModule::~ToyFastbusModule() { 
}

Bool_t ToyFastbusModule::IsSlot(Int_t rdata) {
  return (fSlot == ((rdata&fSlotMask)>>fSlotShift));  
}


ClassImp(ToyFastbusModule)
