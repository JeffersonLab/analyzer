///////////////////////////////////////////////////////////////////
//
//   FastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "FastbusModule.h"
#include "THaSlotData.h"
#include <iostream>

using namespace std;

namespace Decoder {

FastbusModule::FastbusModule(Int_t crate, Int_t slot) : Module(crate, slot) {
  SetSlot(crate, slot);
}

FastbusModule::~FastbusModule() {
}

void FastbusModule::Init() {
  fSlotMask = 0xf8000000;
  fSlotShift = 27;
  fDebugFile=0;
}

Int_t FastbusModule::Decode(const UInt_t *evbuffer) {
  fChan = Chan(*evbuffer);
  fData = Data(*evbuffer);
  fRawData = fData;
  return 1;
}

Int_t FastbusModule::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop) {
  static int first_load=kTRUE;
  if (first_load) {
    if (fCrate < 0 || fCrate > MAXROC) {
       cerr << "FastBusModule::ERROR: crate out of bounds"<<endl;
       fCrate = 0;
    }
    if (fSlot < 0 || fSlot > MAXSLOT_FB) {
       cerr << "FastBusModule::ERROR: slot out of bounds"<<endl;
       fSlot = 0;
    }
    first_load=kFALSE;
  }
  fWordsSeen = 0;
  fHeader=0;
  const UInt_t *p = evbuffer;
  if (fDebugFile) {
     *fDebugFile << "FastbusModule:: loadslot "<<endl;
     if (fHasHeader) {
         *fDebugFile << "TFB:: Has header "<<endl;
     } else {
         *fDebugFile << "TFB:: Has NO header "<<endl;
     }
     *fDebugFile << "FBModule::  Model number  "<<dec<<fModelNum<<endl;
  }
  while (IsSlot( *p )) {
    if (p >= pstop) break;
    if (fHasHeader && fWordsSeen==0) {
      fHeader = *p;
      if (fDebugFile) *fDebugFile << "FastbusModule:: header "<<hex<<fHeader<<dec<<endl;
    } else {
      Decode(p);
      if (fDebugFile) *fDebugFile << "FastbusModule:: chan "<<dec<<fChan<<"  data "<<fData<<"   raw "<<hex<<*p<<dec<<endl;
      sldat->loadData(fChan, fData, fRawData);
    }
    fWordsSeen++;
    p++;
  }
  if (fHeader) {
    Int_t fWordsExpect = (fHeader&fWdcntMask);
    if (fDebugFile) *fDebugFile << "FastbusModule:: words expected  "<<dec<<fWordsExpect<<endl;
    if (fWordsExpect != fWordsSeen) {
      if (fDebugFile) *fDebugFile << "ERROR:  FastbusModule:  crate "<<fCrate<<"   slot "<<fSlot<<" number of words expected "<<fWordsExpect<<"  not equal num words seen "<<fWordsSeen<<endl;
// This happens a lot for some modules, and appears to be harmless, so I suppress it.
//      cerr << "ERROR:  FastbusModule:   number of words expected "<<fWordsExpect<<"  not equal num words seen "<<fWordsSeen<<endl;
    }
  }
  return fWordsSeen;
}

void FastbusModule::DoPrint() const {

  if (fDebugFile) {
       *fDebugFile << "FastbusModule   DoPrint.   name = "<<fName<<
           "  Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
       *fDebugFile << "FastbusModule   model num  "<<fModelNum;
       *fDebugFile << "   masks   "<<hex<<fChanMask<<"  "<<fDataMask<<"  "<<fWdcntMask<<endl;
       if (fHasHeader) {
            *fDebugFile << "FastbusModule :: has a header "<<endl;
       } else {
            *fDebugFile << "FastbusModule :: has NO header "<<endl;

       }
  }

}

}

ClassImp(Decoder::FastbusModule)
