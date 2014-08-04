////////////////////////////////////////////////////////////////////
//
//   Lecroy1875Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1875Module.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

Lecroy1875Module::Lecroy1875Module(Int_t crate, Int_t slot) : ToyFastbusModule(crate, slot) {
  fChanMask = 0x7f0000;
  fDataMask = 0xfff;
  fWdcntMask = 0;
  fOptMask = 0x800000;
  fChanShift = 16;
  fOptShift = 23;
  fHasHeader = kFALSE;
  fHeader = 0;
}

Lecroy1875Module::~Lecroy1875Module() { 
}


ClassImp(Lecroy1875Module)
