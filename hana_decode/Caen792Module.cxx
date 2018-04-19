/////////////////////////////////////////////////////////////////////
//
//   Caen792Module
//   author Stephen Wood
//   author Vincent Sulkosky
//   
//   Currently assume decoding is identical to the 775
//
/////////////////////////////////////////////////////////////////////

#include "Caen792Module.h"
#include <iostream>

using namespace std;

namespace Decoder {

  Module::TypeIter_t Caen792Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Caen792Module" , 792 ));

  Caen792Module::Caen792Module(Int_t crate, Int_t slot) : Caen775Module(crate, slot) {
    fDebugFile=0;
  }

}
    
ClassImp(Decoder::Caen792Module)
