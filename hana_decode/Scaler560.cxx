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

Scaler560::Scaler560( UInt_t crate, UInt_t slot )
  : GenScaler(crate, slot)
{
  Scaler560::Init();
}

void Scaler560::Init()
{
  fNumChan = 16;
  fWordsExpect = 16;
  GenScaler::GenInit();
}

}

ClassImp(Decoder::Scaler560)
