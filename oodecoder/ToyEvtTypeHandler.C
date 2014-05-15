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

ToyEvtTypeHandler::ToyEvtTypeHandler() { 
  fMap=0;
}

ToyEvtTypeHandler::~ToyEvtTypeHandler() { 
}

Int_t ToyEvtTypeHandler::Decode(THaEvData *evdata) {
  return 1;
}

Int_t ToyEvtTypeHandler::Init(THaCrateMap *map) {
  *fMap = *map;   // need to define copy C'tor
  return 1;
}


ClassImp(ToyEvtTypeHandler)
