////////////////////////////////////////////////////////////////////
//
//   Scaler3800
//   SIS Struck model 3800 scaler.  It has 32 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler3800.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


Scaler3800::Scaler3800(Int_t crate, Int_t slot) : THaGenScaler(crate, slot) { 
  Init();
}

Scaler3800::~Scaler3800() { 
}

void Scaler3800::Init() {
  fNumChan = 32;
  fWordsExpect = 32;
  THaGenScaler::GenInit();
}


ClassImp(Scaler3800)
