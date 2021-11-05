////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   Abstract interface to VME modules
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"
#include "THaSlotData.h"
#include <iostream>

using namespace std;

namespace Decoder {

VmeModule::VmeModule(UInt_t crate, UInt_t slot) : Module(crate,slot) {
}

Bool_t VmeModule::IsSlot(UInt_t rdata) {
  // Simplest version of IsSlot relies on a unique header.
  if ((rdata & fHeaderMask)==fHeader) {
    fWordsExpect = (rdata & fWdcntMask)>>fWdcntShift;
    return true;
  }
  return false;
}

UInt_t VmeModule::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer,
                            const UInt_t *pstop)
{
  // This is a simple, default method for loading a slot
  const UInt_t *p = evbuffer;
  if (fDebugFile) {
       *fDebugFile << "Module:: Loadslot "<<endl;
       *fDebugFile << "header  0x"<<hex<<fHeader<<dec<<endl;
       *fDebugFile << "masks  "<<hex<<fHeaderMask<<endl;
  }
  if (!fHeader) cerr << "Module::LoadSlot::ERROR : no header ?"<<endl;
  fWordsSeen=0;
  while (IsSlot( *p )) {
    if (fDebugFile) *fDebugFile << "IsSlot ... data = "<<*p<<endl;
    p++;
    if (p > pstop) break;
    Decode(p);
    for( UInt_t ichan = 0, nchan = GetNumChan(); ichan < nchan; ichan++ ) {
      fWordsSeen++;
      p++;
      if (p > pstop) break;
      UInt_t rdata = *p;
      UInt_t mdata = rdata;
      sldat->loadData(ichan, mdata, rdata);
      if (ichan < fData.size()) fData[ichan]=rdata;
    }
  }
  return fWordsSeen;
}

}

ClassImp(Decoder::VmeModule)
