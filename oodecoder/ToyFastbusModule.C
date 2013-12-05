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

// crate, etc (here, toy data) would ultimately come from crate map
ToyFastbusModule::ToyFastbusModule(Int_t crate, Int_t slot) : fChanMask(0xfe0000), fDataMask(0xffff), fWdcntMask(0x7ff), fChanShift(17) {  
  fCrate = crate;
  fSlot = slot;
}

ToyFastbusModule::ToyFastbusModule() : fChanMask(0xfe0000), fDataMask(0xffff), fWdcntMask(0x7ff), fChanShift(17) {  
  fCrate = 0;
  fSlot = 0;
}

Int_t ToyFastbusModule::Decode(THaEvData *evdata, Int_t jstart) {
  
  cout<<"ToyFastbusModule decode from "<<dec<<jstart<<endl;
  cout << "Crate "<<fCrate<<"   slot "<<fSlot<<endl;
// Yes, I know this is inefficient ... it's a TOY code.
  Int_t index=jstart;
  while (index++ < evdata->GetEvLength()) {
    Int_t data = evdata->GetRawData(index);
    if ( IsSlot(data) ) {
      Int_t chan = (data & fChanMask) >> fChanShift;
      Int_t wdcnt = (data & fWdcntMask);
      if (wdcnt > 10) wdcnt=10;  // hummm... TOY decoding is wrong; fix later
      Int_t rdata = (data & fDataMask);
      cout << "slot "<<fSlot<<" found.  chan = "<<chan<<"    wdcnt "<<wdcnt<<endl;
      for (Int_t i = 0; i<wdcnt; i++) {
	 evdata->LoadData(fCrate, fSlot, chan, rdata);
         index++;
      }
    }

  }

  return 0;
}

ToyFastbusModule::~ToyFastbusModule() { 
}

Bool_t ToyFastbusModule::IsSlot(Int_t rdata) {
  return (fSlot == ((rdata>>27)&0xf));  // TOY, but sorta works
}


ClassImp(ToyFastbusModule)
