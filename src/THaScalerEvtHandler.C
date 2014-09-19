////////////////////////////////////////////////////////////////////
//
//   THaScalerEvtHandler
//
//   Prototype event handler for scalers.
//   R. Michaels,  Sept, 2014
//
//   This class does the following
//      For a particular set of event types (here, just 140)
//      decode the scalers and put some variables into global variables.
//      The global variables can then appear in the Podd output tree T.
//      In addition, a tree "TS" is created by this class; it contains 
//      just the scaler data by itself.  
//      The list of global variables and how they are tied to the 
//      scaler module and channels is defined here; eventually this
//      will be modified to use a scaler.map file
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaScalerEvtHandler.h"
#include "GenScaler.h"
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

THaScalerEvtHandler::THaScalerEvtHandler(const char *name, const char* description) : THaEvtTypeHandler(name,description) {
  rdata = new Int_t[MAXEVLEN];
  fDebugFile = 0;
  fScalerTree = 0;
  evcount = 0;
}

THaScalerEvtHandler::~THaScalerEvtHandler() { 
  delete [] rdata;
  if (fDebugFile) {
    fDebugFile->close();
    delete fDebugFile;
  }
  if (fScalerTree) {
    delete fScalerTree;
  }
}

Int_t THaScalerEvtHandler::End( THaRunBase* r) {
  if (fScalerTree) fScalerTree->Write();   
}

Int_t THaScalerEvtHandler::Analyze(THaEvData *evdata) {

  Int_t ldebug=1;
  Int_t lfirst=1;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  if (lfirst && !fScalerTree) {

    lfirst = 0; // Can't do this in Init for some reason

    fScalerTree = new TTree("TS","THa Scaler Data");
    fScalerTree->SetAutoSave(200000000);

    string name, tinfo;

    name = "evcount";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &evcount, tinfo.c_str(), 4000);

    // yeah, this following should be a loop, once we have the loop for AddVars
    name = "bcmu1";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &dvars[0], tinfo.c_str(), 4000);
    name = "bcmu1r";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &dvars[1], tinfo.c_str(), 4000);
    name = "bcmu3";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &dvars[2], tinfo.c_str(), 4000);
    name = "bcmu3r";
    tinfo = name + "/D";
    fScalerTree->Branch(name.c_str(), &dvars[3], tinfo.c_str(), 4000);

  }


// Parse the data, load local data arrays.

  Int_t ndata = evdata->GetEvLength();
  if (ndata >= MAXEVLEN) {
    cout << "THaScalerEvtHandler:: ERROR: Event length crazy "<<endl;
    ndata = MAXEVLEN;
  }

  if (fDebugFile) *fDebugFile<<"\n\nTHaScalerEvtHandler :: Debugging event type "<<evdata->GetEvType()<<endl;

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

  // The correspondance between dvars and the scaler and the channel
  // will be driven by a scaler.map file  -- later
  dvars[0] = scalers[1]->GetData(4);
  dvars[1] = scalers[1]->GetRate(4);
  dvars[2] = scalers[1]->GetData(5);
  dvars[3] = scalers[1]->GetRate(5);

  evcount = evcount + 1.0;

  for (Int_t j=0; j<scalers.size(); j++) scalers[j]->Clear("");

  if (fDebugFile) *fDebugFile << "scaler tree ptr  "<<fScalerTree<<endl;

  if (fScalerTree) fScalerTree->Fill();

  return 1;
}

THaAnalysisObject::EStatus THaScalerEvtHandler::Init(const TDatime& dt, Int_t idebug) {

  eventtypes.push_back(140);  // what events to look for

  if (idebug) {
    fDebugFile = new ofstream();
    fDebugFile->open("scaler.txt");
  }

  // This will be driven by a scaler.map and will be a kind of loop.
  AddVars("TSbcmu1", "BCM x1 counts");
  AddVars("TSbcmu1r","BCM x1 rate");
  AddVars("TSbcmu3", "BCM u3 counts");
  AddVars("TSbcmu3r", "BCM u3 rate");

  DefVars();

// This should be driven by a scaler.map.  Hard-coded for now.

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
  if(fDebugFile) *fDebugFile << "THaScalerEvtHandler:: Name of scaler bank "<<fName<<endl;
  for (Int_t i=0; i<scalers.size(); i++) {
    if(fDebugFile) *fDebugFile << "Scaler  #  "<<i<<endl;
    //    scalers[i]->DoPrint();
  }
  return kOK;
}

void THaScalerEvtHandler::AddVars(char *name, char *desc) {
  RVarDef newvar;
  newvar.name = name;
  newvar.desc = desc;
  if (fDebugFile) *fDebugFile << "THaScalerEvtHandler:: adding a new variable "<<name<<"   "<<desc<<endl;
  rvars.push_back(newvar);
}

void THaScalerEvtHandler::DefVars() {
  // called after AddVars called several times
  if (rvars.size() == 0) return;
  Nvars = rvars.size();
  dvars = new Double_t[Nvars];  // dvars is a class member
  if (gHaVars) { 
    if(fDebugFile) *fDebugFile << "THaScalerEVtHandler:: Have gHaVars "<<gHaVars<<endl;
  } else {
    cout << "No gHaVars ?!  Well, that's a problem !!"<<endl;
    return;
  }
  if(fDebugFile) *fDebugFile << "THaScalerEvtHandler:: rvars size "<<rvars.size()<<endl;
  const Int_t* count = 0;
  for (UInt_t i = 0; i < rvars.size(); i++) {
     gHaVars->DefineByType(rvars[i].name, rvars[i].desc,  &dvars[i], kDouble, count);    
  }
}



ClassImp(THaScalerEvtHandler)
