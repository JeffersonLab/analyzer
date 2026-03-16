////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   LeCroy model 1151 scaler.  It has 16 channels
//
/////////////////////////////////////////////////////////////////////

#include "Scaler1151.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Scaler1151::fgThisType =
  DoRegister( { "Decoder::Scaler1151" , 1151,
    ECrateCode::kVME, EModuleType::kScaler, 16 });

Scaler1151::Scaler1151( UInt_t crate, UInt_t slot )
  : GenScaler(crate, slot)
{
  Scaler1151::Init();
}

void Scaler1151::Init()
{
  fNumChan = 16;
  fWordsExpect = 16;
  GenScaler::GenInit();
}

}

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Decoder::Scaler1151)
#endif
