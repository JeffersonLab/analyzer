////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   LeCroy model 1151 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler1151.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


Scaler1151::Scaler1151(Int_t crate, Int_t slot) : THaGenScaler(crate, slot, 16) { 
}

Scaler1151::~Scaler1151() { 
}


ClassImp(Scaler1151)
