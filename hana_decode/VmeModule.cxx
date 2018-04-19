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

Int_t VmeModule::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer,
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
    if (p >= pstop) break;
    if (fDebugFile) *fDebugFile << "IsSlot ... data = "<<*p<<endl;
    p++;
    Decode(p);
    for (size_t ichan = 0, nchan = GetNumChan(); ichan < nchan; ichan++) {
      Int_t mdata,rdata;
      fWordsSeen++;
      p++;
      if (p >= pstop) break;
      rdata = *p;
      mdata = rdata;
      sldat->loadData(ichan, mdata, rdata);
      if (ichan < fData.size()) fData[ichan]=rdata;
    }
  }
  return fWordsSeen;
}

}

ClassImp(Decoder::VmeModule)
