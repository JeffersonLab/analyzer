////////////////////////////////////////////////////////////////////
//
//   Lecroy1877Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1877Module.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

Lecroy1877Module::Lecroy1877Module(Int_t crate, Int_t slot) : ToyFastbusModule(crate, slot) {
  Init(); 
}

void Lecroy1877Module::Init() {
  fChanMask = 0xfe0000;
  fDataMask = 0xffff;
  fWdcntMask = 0x7ff;
  fOptMask = 0x10000;
  fChanShift = 17;
  fOptShift = 16;
  fHasHeader = kTRUE;
  fHeader = 0;
  fModelNum = 1877;
}

Lecroy1877Module::~Lecroy1877Module() { 
}


ClassImp(Lecroy1877Module)
