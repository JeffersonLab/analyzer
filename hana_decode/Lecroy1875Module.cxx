////////////////////////////////////////////////////////////////////
//
//   Lecroy1875Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1875Module.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Lecroy1875Module::fgThisType =
  DoRegister( ModuleType( "Decoder::Lecroy1875Module" , 1875));

Lecroy1875Module::Lecroy1875Module( UInt_t crate, UInt_t slot )
  : FastbusModule(crate, slot)
{
  Lecroy1875Module::Init();
}

void Lecroy1875Module::Init()
{
  FastbusModule::Init();
  fChanMask = 0x7f0000;
  fDataMask = 0xfff;
  fWdcntMask = 0;
  fOptMask = 0x800000;
  fChanShift = 16;
  fOptShift = 23;
  fHasHeader = false;
  fHeader = 0;
  fModelNum = 1875;
}


}

ClassImp(Decoder::Lecroy1875Module)
