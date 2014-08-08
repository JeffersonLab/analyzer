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
  if (fDebugFile) *fDebugFile << "Initializeing FB module in crate "<<crate<<"     slot "<<slot<<endl;
  fCrate = crate;
  fSlot = slot;
  fSlotMask = 0xf8000000;
  fSlotShift = 27;
}

Int_t ToyFastbusModule::Decode(const Int_t *evbuffer) {
  fChan = Chan(*evbuffer);
  fData = Data(*evbuffer);
  fRawData = fData;
  return 1;
}

Int_t ToyFastbusModule::LoadSlot(THaSlotData *sldat, const Int_t* evbuffer, const Int_t *pstop) {
  fWordsSeen = 0;
  fHeader=0;
  const Int_t *p = evbuffer;
  if (fDebugFile) {
     *fDebugFile << "ToyFastbusModule:: loadslot "<<endl; 
     if (fHasHeader) {
         *fDebugFile << "Has header "<<endl;
     } else {
         *fDebugFile << "Has NO header "<<endl;
     }
     *fDebugFile << "Model number  "<<dec<<fModelNum<<endl;
  }
  while (IsSlot( *p )) {
    if (fDebugFile) *fDebugFile << "chk1 "<<p<<"  "<<fWordsSeen<<"  "<<pstop<<endl;
    if (p >= pstop) break;
    if (fHasHeader && fWordsSeen==0) {
      fHeader = *p;
      if (fDebugFile) *fDebugFile << "ToyFastbusModule:: header "<<hex<<fHeader<<dec<<endl; 
  } else {
      Decode(p);
      if (fDebugFile) *fDebugFile << "ToyFastbusModule:: chan "<<dec<<fChan<<"  data "<<fData<<endl;
      sldat->loadData(fChan, fData, fRawData);
    }
    fWordsSeen++;
    p++;
  }
  if (fHeader) {
    Int_t fWordsExpect = (fHeader&fWdcntMask)>>fWdcntShift;
    if (fDebugFile) *fDebugFile << "ToyFastbusModule:: words exected  "<<dec<<fWordsExpect<<endl;
    if (fWordsExpect != fWordsSeen) {
      cerr << "ERROR:  FastbusModule:   number of words expected "<<fWordsExpect<<"  not equal num words seen "<<fWordsSeen<<endl;
    }
  }
  return fWordsSeen;
}

void ToyFastbusModule::DoPrint() {

  if (fDebugFile) {
       *fDebugFile << "ToyFastbusModule   DoPrint.   name = "<<fName<<
           "  Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
       *fDebugFile << "ToyFastbusModule   model num  "<<fModelNum<< endl;
       *fDebugFile << "stuff   "<<hex<<fChanMask<<"  "<<fDataMask<<"  "<<fWdcntMask<<endl;
       if (fHasHeader) {
            *fDebugFile << "ToyFastbusModule :: has a header "<<endl;
       } else {
            *fDebugFile << "ToyFastbusModule :: has NO header "<<endl;

       }
  }

}

ClassImp(ToyFastbusModule)
