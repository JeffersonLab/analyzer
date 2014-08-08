////////////////////////////////////////////////////////////////////
//
//   Lecroy1881Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1881Module.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

Lecroy1881Module::Lecroy1881Module(Int_t crate, Int_t slot) : ToyFastbusModule(crate, slot) {
  Init();
}

void Lecroy1881Module::Init() {
  fChanMask = 0x7e0000;
  fDataMask = 0x3fff;
  fWdcntMask = 0x7f;
  fOptMask = 0x3000000;
  fChanShift = 17;
  fOptShift = 24;
  fHasHeader = kTRUE;
  fHeader = 0;
  fModelNum = 1881;
}

Lecroy1881Module::~Lecroy1881Module() { 
}


ClassImp(Lecroy1881Module)
