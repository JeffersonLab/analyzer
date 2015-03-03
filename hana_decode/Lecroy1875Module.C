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

Lecroy1875Module::Lecroy1875Module(Int_t crate, Int_t slot) : FastbusModule(crate, slot) {
  Init();
}

void Lecroy1875Module::Init() {
  fChanMask = 0x7f0000;
  fDataMask = 0xfff;
  fWdcntMask = 0;
  fOptMask = 0x800000;
  fChanShift = 16;
  fOptShift = 23;
  fHasHeader = kFALSE;
  fHeader = 0;
  fModelNum = 1875;
  FastbusModule::Init();
}


Lecroy1875Module::~Lecroy1875Module() {
}

}

ClassImp(Decoder::Lecroy1875Module)
