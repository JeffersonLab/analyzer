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


Scaler3801::Scaler3801(Int_t crate, Int_t slot) : THaGenScaler(crate, slot, 32) { 
}

Scaler3801::~Scaler3801() { 
}


ClassImp(Scaler3801)
