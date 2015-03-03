////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   LeCroy model 1151 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler1151.h"

using namespace std;

namespace Decoder {

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

}

ClassImp(Decoder::Scaler1151)
