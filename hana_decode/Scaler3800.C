////////////////////////////////////////////////////////////////////
//
//   Scaler3800
//   SIS Struck model 3800 scaler.  It has 32 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler3800.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Scaler3800::fgThisType =
  DoRegister( ModuleType( "Decoder::Scaler3800" , 3800 ));

Scaler3800::Scaler3800(Int_t crate, Int_t slot) : GenScaler(crate, slot) {
  Init();
}

Scaler3800::~Scaler3800() {
}

void Scaler3800::Init() {
  fNumChan = 32;
  fWordsExpect = 32;
  GenScaler::GenInit();
}

}

ClassImp(Decoder::Scaler3800)
