///////////////////////////////////////////////////////////////////
//
//   FastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "FastbusModule.h"
#include "THaSlotData.h"
#include <iostream>
#include <stdexcept>

using namespace std;

namespace Decoder {

FastbusModule::FastbusModule( UInt_t crate, UInt_t slot )
  : Module(crate, slot), fHasHeader(false),
    fSlotMask(0), fSlotShift(0), fChanMask(0), fChanShift(0),
    fDataMask(0), fOptMask(0), fOptShift(0),
    fChan(0), fData(0), fRawData(0)
{
  FastbusModule::SetSlot(crate, slot);  //FIXME redundant/useless?
}

void FastbusModule::Init() {
  Module::Init();
  fSlotMask = 0xf8000000;
  fSlotShift = 27;
}

Int_t FastbusModule::Decode(const UInt_t *evbuffer) {
  fChan = Chan(*evbuffer);
  fData = Data(*evbuffer);
  fRawData = *evbuffer;
  return 1;
}

void FastbusModule::SetSlot( UInt_t crate, UInt_t slot, UInt_t header,
                             UInt_t mask, Int_t modelnum )
{
  // SetSlot function with parameter checks appropriate for Fastbus
  if( fCrate >= MAXROC )
    throw invalid_argument("FastBusModule::ERROR: crate out of bounds");
  if( fSlot >= MAXSLOT_FB )
    throw invalid_argument("FastBusModule::ERROR: slot out of bounds");

  Module::SetSlot(crate, slot, header, mask, modelnum);
}


UInt_t FastbusModule::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                const UInt_t* pstop )
{
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
    if (p > pstop) break;
  }
  if (fHeader) {
    UInt_t fWordsExpect = (fHeader&fWdcntMask);
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
