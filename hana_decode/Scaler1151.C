////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   LeCroy model 1151 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "Scaler1151.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace Decoder;


Scaler1151::Scaler1151(Int_t crate, Int_t slot) : GenScaler(crate, slot) { 
  Init();
}

Scaler1151::~Scaler1151() { 
}

void Scaler1151::Init() {
  fNumChan = 16;
  fWordsExpect = 16;
  GenScaler::GenInit();
}


ClassImp(Decoder::Scaler1151)
