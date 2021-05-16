////////////////////////////////////////////////////////////////////
//
//   Lecroy1877Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1877Module.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Lecroy1877Module::fgThisType =
  DoRegister( ModuleType( "Decoder::Lecroy1877Module" , 1877));

Lecroy1877Module::Lecroy1877Module( UInt_t crate, UInt_t slot )
  : FastbusModule(crate, slot)
{
  Lecroy1877Module::Init();
}

void Lecroy1877Module::Init()
{
  FastbusModule::Init();
  fChanMask = 0xfe0000;
  fDataMask = 0xffff;
  fWdcntMask = 0x7ff;
  fOptMask = 0x10000;
  fChanShift = 17;
  fOptShift = 16;
  fHasHeader = true;
  fHeader = 0;
  fModelNum = 1877;
}

}

ClassImp(Decoder::Lecroy1877Module)
