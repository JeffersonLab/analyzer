////////////////////////////////////////////////////////////////////
//
//   ToyScalerEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyScalerEvtHandler.h"
#include "/home/rom/poddfork1/analyzer/oodecoder/THaGenScaler.h"
#include "/home/rom/poddfork1/analyzer/oodecoder/Scaler3800.h"
#include "/home/rom/poddfork1/analyzer/oodecoder/Scaler1151.h"
#include "/home/rom/poddfork1/analyzer/oodecoder/THaCodaData.h"
#include "THaEvData.h"
#include "TNamed.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyScalerEvtHandler::ToyScalerEvtHandler(const char *name, const char* description) : ToyEvtTypeHandler(name,description) {
  rdata = new Int_t[MAXEVLEN];
}

ToyScalerEvtHandler::~ToyScalerEvtHandler() { 
  delete [] rdata;
}

Int_t ToyScalerEvtHandler::Analyze(THaEvData *evdata) {

  Int_t ldebug=1;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  std::cout << "Howdy !!  I am a SCALER event !\n"<<std::endl;

  Int_t header = 0xabc00000;  //  get from a database
  Int_t slotmask = 0xf0000;   //  ditto
  Int_t ipt = -1;

// Parse the data, load into evdata structures.  Conceivably produce
// global variables (not done in this toy example)

  Int_t ndata = evdata->GetEvLength();
  if (ndata >= MAXEVLEN) {
    cout << "ToyScalerEvtHandler:: ERROR: Event length crazy "<<endl;
    ndata = MAXEVLEN;
  }

  // local copy of data
  while (++ipt < ndata) { 
    rdata[ipt] = evdata->GetRawData(ipt);
  }


  Int_t nskip;
  Int_t *p = rdata;
  Int_t *pstop = rdata+ndata;

  while (p < pstop) {
    cout << "p  and  pstop  "<<p<<"   "<<pstop<<"   "<<hex<<*p<<"   "<<dec<<endl;   nskip = 1;
    for (Int_t j=0; j<scalers.size(); j++) {
       nskip = scalers[j]->Decode(p);
       if (nskip > 1) break;
    }
    p = p + nskip;
    cout << "duh  nskip = "<<nskip<< "   "<<p<<endl;

  }
  
 for (Int_t j=0; j<scalers.size(); j++) {
       
      scalers[j]->Clear("");
 }


#ifdef THING1
    if ((data & header)==header) {
      Int_t slot = (data&slotmask)>>16; 
      Int_t numchan = data&0xff;
      cout<<"Found scaler at  slot "<<dec<<slot<<"   numchan "<<numchan<<endl;
      for (Int_t chan=0; chan<numchan; chan++) {
        Int_t scadat = evdata->GetRawData(++ipt);
        cout << "               chan "<<chan;
        cout <<"   data 0x"<<hex<<scadat<<dec<<endl;

      }
    }
#endif

  return 1;
}

THaAnalysisObject::EStatus ToyScalerEvtHandler::Init(const TDatime& dt) {
  eventtypes.push_back(140);

  if (fName == "Left") {
    scalers.push_back(new Scaler1151(1,0));
    scalers.push_back(new Scaler3800(1,1));
    scalers.push_back(new Scaler3800(1,2));
    scalers.push_back(new Scaler3800(1,3));
    scalers[0]->SetHeader(0xabc00000, 0xffff0000);
    scalers[1]->SetHeader(0xabc10000, 0xffff0000);
    scalers[2]->SetHeader(0xabc20000, 0xffff0000);
    scalers[3]->SetHeader(0xabc30000, 0xffff0000);
  } else {
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler1151(2,1));
    scalers.push_back(new Scaler1151(2,2));
    scalers[0]->SetHeader(0xceb00000, 0xffff0000);
    scalers[1]->SetHeader(0xceb10000, 0xffff0000);
    scalers[2]->SetHeader(0xceb20000, 0xffff0000);
    scalers[3]->SetHeader(0xceb30000, 0xffff0000);
  }
  cout << "Name of scaler bank "<<fName<<endl;
  for (Int_t i=0; i<scalers.size(); i++) {
    cout << "Scaler   "<<i<<endl;
    scalers[i]->DoPrint();
  }
  return kOK;
}

ClassImp(ToyScalerEvtHandler)
