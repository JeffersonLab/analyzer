////////////////////////////////////////////////////////////////////
//
//   LeCroy1877Module
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

// crate, etc (here, toy data) would ultimately come from crate map
Lecroy1877Module::Lecroy1877Module(Int_t crate, Int_t slot) : ToyFastbusModule(crate, slot) {
  fChanMask = 0xfe0000;
  fDataMask = 0xffff;
  fWdcntMask = 0x7ff;
  fChanShift = 17;
  fNoWdCnt = 0;
}

Lecroy1877Module::~Lecroy1877Module() { 
}


ClassImp(Lecroy1877Module)
