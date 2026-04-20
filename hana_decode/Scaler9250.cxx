/**
   \class Scaler9250
   \ingroup Decoders

   \brief Decoder module to read the FADC250 scalers.

   These scalers are identified by a bank with the tag 9250.
*/

#include "Scaler9250.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Scaler9250::fgThisType = // NOLINT(*-throwing-static-initialization)
  DoRegister( ModuleType( "Decoder::Scaler9250" , 9250 ));

Scaler9250::Scaler9250( UInt_t crate, UInt_t slot )
  : GenScaler(crate, slot)
{
  Scaler9250::Init();
}

void Scaler9250::Init()
{
  GenScaler::Init();
  fNumChan = 16;
  fWordsExpect = fNumChan;
  GenInit();
}

}

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Decoder::Scaler9250)
#endif
