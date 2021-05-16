////////////////////////////////////////////////////////////////////
//
//   Lecroy1881Module
//
/////////////////////////////////////////////////////////////////////

#include "Lecroy1881Module.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t Lecroy1881Module::fgThisType =
  DoRegister( ModuleType( "Decoder::Lecroy1881Module" , 1881));

Lecroy1881Module::Lecroy1881Module( UInt_t crate, UInt_t slot )
  : FastbusModule(crate, slot)
{
  Lecroy1881Module::Init();
}

void Lecroy1881Module::Init()
{
  FastbusModule::Init();
  fChanMask = 0x7e0000;
  fDataMask = 0x3fff;
  fWdcntMask = 0x7f;
  fOptMask = 0x3000000;
  fChanShift = 17;
  fOptShift = 24;
  fHasHeader = true;
  fHeader = 0;
  fModelNum = 1881;
}

}

ClassImp(Decoder::Lecroy1881Module)
