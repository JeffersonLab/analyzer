////////////////////////////////////////////////////////////////////
//
//   ToyEvtTypeHandler
//   handles events of a particular type
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyEvtTypeHandler::ToyEvtTypeHandler(const char* name, const char* description)  : THaAnalysisObject(name, description) { 
}

ToyEvtTypeHandler::~ToyEvtTypeHandler() { 
}

Int_t ToyEvtTypeHandler::Analyze(THaEvData *evdata) {

  return 1;

}

void ToyEvtTypeHandler::Print() {
  cout << "Hello !  ToyEvtTypeHandler name =  "<<GetName()<<endl;
  cout << "    description "<<GetTitle()<<endl;
  cout << "    event types handled are "<<endl;
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    cout << "    event type "<<eventtypes[i]<<endl;
  }
  cout << "----------------- good bye ----------------- "<<endl;
}



THaAnalysisObject::EStatus ToyEvtTypeHandler::Init(const TDatime& dt) {
  return kOK;
}

Bool_t ToyEvtTypeHandler::IsMyEvent(Int_t evnum) {
 
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    if (evnum == eventtypes[i]) return kTRUE;
  }

  return kFALSE;

}

//_____________________________________________________________________________
void ToyEvtTypeHandler::MakePrefix()
{
  THaAnalysisObject::MakePrefix( NULL );
}



ClassImp(ToyEvtTypeHandler)
