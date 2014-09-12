////////////////////////////////////////////////////////////////////
//
//   Scaler3801
//   SIS Struck 3801 scaler.  It has 32 channels and runs in 
//      a FIFO mode
//
/////////////////////////////////////////////////////////////////////

#include "Scaler3801.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


Scaler3801::Scaler3801(Int_t crate, Int_t slot) : GenScaler(crate, slot) { 
  Init();
}

Scaler3801::~Scaler3801() { 
}

void Scaler3801::Init() {
  fNumChan = 32;
  fWordsExpect = 32;
  GenScaler::GenInit();
}


ClassImp(Decoder::Scaler3801)
