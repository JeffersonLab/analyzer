////////////////////////////////////////////////////////////////////
//
//   ToyScalerEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyScalerEvtHandler.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyScalerEvtHandler::ToyScalerEvtHandler() { 
}

ToyScalerEvtHandler::~ToyScalerEvtHandler() { 
}

Int_t ToyScalerEvtHandler::Decode(THaEvData *evdata) {

  std::cout << "howdy !  I am a scaler event !\n"<<std::endl;

  return 0;
}

void ToyScalerEvtHandler::Init(THaCrateMap *map) {

}

ClassImp(ToyScalerEvtHandler)
