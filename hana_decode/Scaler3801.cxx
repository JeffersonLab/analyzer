////////////////////////////////////////////////////////////////////
//
//   Scaler3801
//   SIS Struck 3801 scaler.  It has 32 channels and runs in
//      a FIFO mode
//
/////////////////////////////////////////////////////////////////////

#include "Scaler3801.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Scaler3801::fgThisType =
  DoRegister( ModuleType( "Decoder::Scaler3801" , 3801 ));

Scaler3801::Scaler3801( UInt_t crate, UInt_t slot )
  : GenScaler(crate, slot)
{
  Scaler3801::Init();
}

void Scaler3801::Init()
{
  fNumChan = 32;
  fWordsExpect = 32;
  GenScaler::GenInit();
}

}

ClassImp(Decoder::Scaler3801)
