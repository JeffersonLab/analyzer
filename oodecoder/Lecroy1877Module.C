////////////////////////////////////////////////////////////////////
//
//   LeCroy1877Module
//
/////////////////////////////////////////////////////////////////////

#include "LeCroy1877Module.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

// crate, etc (here, toy data) would ultimately come from crate map
LeCroy1877Module::LeCroy1877Module(Int_t crate, Int_t slot) : ToyFastBusModule(crate, slot) {
  fChanMask = 0xfe0000;
  fDataMask = 0xffff;
  fWdcntMask = 0x7ff;
  fChanShift = 17;
  fNoWdCnt = 0;
}

LeCroy1877Module::~LeCroy1877Module() { 
}


ClassImp(LeCroy1877Module)
