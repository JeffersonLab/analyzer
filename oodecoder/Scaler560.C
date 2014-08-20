////////////////////////////////////////////////////////////////////
//
//   Scaler560
//   CAEN model 560 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler560.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


Scaler560::Scaler560(Int_t crate, Int_t slot) : THaGenScaler(crate, slot) { 
  Init();
}

Scaler560::~Scaler560() { 
}

void Scaler560::Init() {
  fNumChan = 16;
  fWordsExpect = 16;
  THaGenScaler::GenInit();
}

ClassImp(Scaler560)
