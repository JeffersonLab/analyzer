#ifndef Podd_Caen792Module_h_
#define Podd_Caen792Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Caen792Module
//   Single Hit ADC
//
/////////////////////////////////////////////////////////////////////

#include "Caen775Module.h"

namespace Decoder {

  class Caen792Module : public Caen775Module {

  public:

    Caen792Module() = default;
    Caen792Module(Int_t crate, Int_t slot);
    const char* MyModType() override {return "adc";}
    const char* MyModName() override {return "792";}

  private:

    static TypeIter_t fgThisType;

    ClassDefOverride(Caen792Module,0)  //  Caen792 module

  };

}

#endif
