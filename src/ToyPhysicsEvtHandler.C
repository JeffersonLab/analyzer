////////////////////////////////////////////////////////////////////
//
//   ToyPhysicsEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyPhysicsEvtHandler.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include "TNamed.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyPhysicsEvtHandler::ToyPhysicsEvtHandler(const char *name, const char* description) : ToyEvtTypeHandler(name,description) {
				      
  rocnum = new Int_t[MAXROC];
  rocpos = new Int_t[MAXROC];
  roclen = new Int_t[MAXROC];
}

ToyPhysicsEvtHandler::~ToyPhysicsEvtHandler() { 
  delete [] rocnum;
  delete [] rocpos;
  delete [] roclen;
}

Int_t ToyPhysicsEvtHandler::Analyze(THaEvData *evdata) {


  return 1;

}



THaAnalysisObject::EStatus ToyPhysicsEvtHandler::Init(const TDatime& dt) {
  for (Int_t i = 0; i < 14; i++) {
    eventtypes.push_back(i+1);
  }
  return kOK;

}


ClassImp(ToyPhysicsEvtHandler)
