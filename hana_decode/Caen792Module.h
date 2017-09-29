#ifndef Caen792Module_
#define Caen792Module_

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

    Caen792Module() : Caen775Module() {}
    Caen792Module(Int_t crate, Int_t slot);
    virtual const char* MyModType() {return "adc";}
    virtual const char* MyModName() {return "792";}

  private:

    static TypeIter_t fgThisType;

    ClassDef(Caen792Module,0)  //  Caen792 module

  };

}

#endif
