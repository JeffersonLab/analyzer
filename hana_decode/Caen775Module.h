#ifndef Podd_Caen775Module_h_
#define Podd_Caen775Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Caen775Module
//   Single Hit TDC with adjustable FSR and resolution
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"

namespace Decoder {

class Caen775Module : public VmeModule {

public:

   Caen775Module() = default;
   Caen775Module( UInt_t crate, UInt_t slot );

   using VmeModule::GetData;
   using VmeModule::Init;

   UInt_t GetData( UInt_t chan) const override;
   void   Init() override;
   void   Clear(Option_t *opt="") override;
   Int_t  Decode(const UInt_t*) override { return 0; }
   UInt_t LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop ) override;
   UInt_t LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer, UInt_t pos, UInt_t len ) override;

   virtual const char* MyModType() {return "tdc";}
   virtual const char* MyModName() {return "775";}
 
private:

   static const size_t NTDCCHAN = 32;

   static TypeIter_t fgThisType;

  ClassDefOverride(Caen775Module,0)  //  Caen775 of a module; make your replacements
};

}

#endif
