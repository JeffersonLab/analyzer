////////////////////////////////////////////////////////////////////
//
//   ToyScalerEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyScalerEvtHandler.h"
#include "THaEvData.h"
#include "TNamed.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyScalerEvtHandler::ToyScalerEvtHandler(const char *name, const char* description) : ToyEvtTypeHandler(name,description) {
}

ToyScalerEvtHandler::~ToyScalerEvtHandler() { 
}

Int_t ToyScalerEvtHandler::Analyze(THaEvData *evdata) {

  std::cout << "Howdy !!  I am a SCALER event !\n"<<std::endl;

  Int_t crate  = 30;    // would be from fMap
  Int_t header = 0xabc00000;  //  ditto
  Int_t slotmask = 0xf0000;   //  ditto

  Int_t ipt = -1;
  Int_t data;

// Parse the data, load into evdata structures.  Conceivably produce
// global variables (not done in this toy example)

  while (++ipt < evdata->GetEvLength()) { 
    data = evdata->GetRawData(ipt);
    if ((data & header)==header) {
      Int_t slot = (data&slotmask)>>16; 
      Int_t numchan = data&0xff;
      cout<<"Found scaler at  slot "<<dec<<slot<<"   numchan "<<numchan<<endl;
      for (Int_t chan=0; chan<numchan; chan++) {
        Int_t scadat = evdata->GetRawData(++ipt);
        cout << "               chan "<<chan;
        cout <<"   data 0x"<<hex<<scadat<<dec<<endl;
	//        evdata->LoadData(crate, slot, chan, scadat);
      }
    }
  }

  return 1;
}

THaAnalysisObject::EStatus ToyScalerEvtHandler::Init(const TDatime& dt) {
  eventtypes.push_back(140);
  return kOK;
}

ClassImp(ToyScalerEvtHandler)
