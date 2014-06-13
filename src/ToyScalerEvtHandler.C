////////////////////////////////////////////////////////////////////
//
//   ToyScalerEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyScalerEvtHandler.h"
#include "THaGenScaler.h"
#include "Scaler3800.h"
#include "Scaler3801.h"
#include "Scaler1151.h"
#include "Scaler560.h"
#include "THaCodaData.h"
#include "THaEvData.h"
#include "TNamed.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyScalerEvtHandler::ToyScalerEvtHandler(const char *name, const char* description) : ToyEvtTypeHandler(name,description) {
  rdata = new Int_t[MAXEVLEN];
  fDebugFile = 0;
  fScalerTree = 0;
}

ToyScalerEvtHandler::~ToyScalerEvtHandler() { 
  delete [] rdata;
  if (fDebugFile) {
    fDebugFile->close();
    delete fDebugFile;
  }
  if (fScalerTree) {
    delete fScalerTree;
  }
}

Int_t ToyScalerEvtHandler::End( THaRunBase* r) {

  if (fScalerTree) fScalerTree->Write();   

}



Int_t ToyScalerEvtHandler::Analyze(THaEvData *evdata) {


  Int_t ldebug=1;
  Int_t lfirst=1;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  if (lfirst && !fScalerTree) {

    lfirst = 0; // Can't do this in Init for some reason

    fScalerTree = new TTree("TS","Toy Scaler Data");
    fScalerTree->SetAutoSave(200000000);

    string name, tinfo;

    name = "bcmu1";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &TSbcmu1, tinfo.c_str(), 4000);
    name = "bcmu1r";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &TSbcmu1r, tinfo.c_str(), 4000);
    name = "bcmu3";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &TSbcmu3, tinfo.c_str(), 4000);
    name = "bcmu3r";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &TSbcmu3r, tinfo.c_str(), 4000);

  }



// Parse the data, load local data arrays.

  Int_t ndata = evdata->GetEvLength();
  if (ndata >= MAXEVLEN) {
    cout << "ToyScalerEvtHandler:: ERROR: Event length crazy "<<endl;
    ndata = MAXEVLEN;
  }

  if (fDebugFile) *fDebugFile<<"\n\nDebugging event type "<<evdata->GetEvType()<<endl;

  // local copy of data
  for (Int_t i=0; i<ndata; i++) rdata[i] = evdata->GetRawData(i);

  Int_t nskip;
  Int_t *p = rdata;
  Int_t *pstop = rdata+ndata;

  while (p < pstop) {
    if (fDebugFile) *fDebugFile << "p  and  pstop  "<<p<<"   "<<pstop<<"   "<<hex<<*p<<"   "<<dec<<endl;   nskip = 1;
    for (Int_t j=0; j<scalers.size(); j++) {
       nskip = scalers[j]->Decode(p);
       if (nskip > 1) break;
       if (fDebugFile) scalers[j]->DebugPrint(fDebugFile);
    }
    p = p + nskip;
  }

  TSbcmu1  = scalers[1]->GetData(4);
  TSbcmu1r = scalers[1]->GetRate(4);
  TSbcmu3  = scalers[1]->GetData(5);
  TSbcmu3r = scalers[1]->GetRate(5);

  if (fDebugFile) *fDebugFile << "my data "<<TSbcmu1<<"  "<<TSbcmu1r<<"  "<<TSbcmu3<<"  "<<TSbcmu3r<<endl;
  
  for (Int_t j=0; j<scalers.size(); j++) scalers[j]->Clear("");

  if (fDebugFile) *fDebugFile << "scaler tree ptr  "<<fScalerTree<<endl;

  if (fScalerTree) fScalerTree->Fill();

  return 1;
}

THaAnalysisObject::EStatus ToyScalerEvtHandler::Init(const TDatime& dt, Int_t idebug) {
  eventtypes.push_back(140);

  if (idebug) {
    fDebugFile = new ofstream();
    fDebugFile->open("scaler.txt");
  }

  Int_t retval = 0;

  TSbcmu1  = 0;
  TSbcmu1r = 0;
  TSbcmu3  = 0;
  TSbcmu3r = 0;

  RVarDef vars[] = {
    { "TSbcmu1",    "BCM x1 counts",     "TSbcmu1"   },  
    { "TSbcmu1r",   "BCM x1 rate",       "TSbcmu1r"  },  
    { "TSbcmu3",    "BCM u3 counts",     "TSbmcu3"   },  
    { "TSbcmu3r",   "BCM u3 rate",       "TSbcmu3r"  },  
    { 0 }
  };

  Int_t nvar = sizeof(vars)/sizeof(RVarDef);

  retval = DefineVarsFromList( vars, kDefine );

// This should be driven by cratemap or maybe scaler.map.  Hard-coded for now.
  if (fName == "Left") {
    scalers.push_back(new Scaler1151(1,0));
    scalers.push_back(new Scaler3800(1,1));
    scalers.push_back(new Scaler3800(1,2));
    scalers.push_back(new Scaler3800(1,3));
    scalers[0]->SetHeader(0xabc00000, 0xffff0000);
    scalers[1]->SetHeader(0xabc10000, 0xffff0000);
    scalers[2]->SetHeader(0xabc20000, 0xffff0000);
    scalers[3]->SetHeader(0xabc30000, 0xffff0000);
    scalers[0]->LoadNormScaler(scalers[1]);
    scalers[1]->SetClock(4, 7, 1024);
    scalers[2]->LoadNormScaler(scalers[1]);
    scalers[3]->LoadNormScaler(scalers[1]);
  } else {
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler1151(2,1));
    scalers.push_back(new Scaler1151(2,2));
    scalers[0]->SetHeader(0xceb00000, 0xffff0000);
    scalers[1]->SetHeader(0xceb10000, 0xffff0000);
    scalers[2]->SetHeader(0xceb20000, 0xffff0000);
    scalers[3]->SetHeader(0xceb30000, 0xffff0000);
    scalers[0]->SetClock(4, 7, 1024);
    scalers[1]->LoadNormScaler(scalers[0]);
    scalers[2]->LoadNormScaler(scalers[0]);
    scalers[3]->LoadNormScaler(scalers[0]);
  }
  cout << "Name of scaler bank "<<fName<<endl;
  for (Int_t i=0; i<scalers.size(); i++) {
    cout << "Scaler   "<<i<<endl;
    scalers[i]->DoPrint();
  }
  return kOK;
}

ClassImp(ToyScalerEvtHandler)
