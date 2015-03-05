////////////////////////////////////////////////////////////////////
//
//   Scaler560
//   CAEN model 560 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler560.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Scaler560::fgThisType =
  DoRegister( ModuleType( "Decoder::Scaler560" , 560 ));

Scaler560::Scaler560(Int_t crate, Int_t slot) : GenScaler(crate, slot) {
  Init();
}

Scaler560::~Scaler560() {
}

void Scaler560::Init() {
  fNumChan = 16;
  fWordsExpect = 16;
  GenScaler::GenInit();
}

}

ClassImp(Decoder::Scaler560)
