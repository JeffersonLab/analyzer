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

   Caen775Module() : VmeModule() {}
   Caen775Module(Int_t crate, Int_t slot);
   virtual ~Caen775Module();

   using Module::GetData;
   using Module::LoadSlot;

   virtual UInt_t GetData(Int_t chan) const;
   virtual void Init();
   virtual void Clear(const Option_t *opt="");
   virtual Int_t Decode(const UInt_t*) { return 0; }
   virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );
   virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, Int_t pos, Int_t len);
   virtual const char* MyModType() {return "tdc";}
   virtual const char* MyModName() {return "775";}
 
private:

   static const size_t NTDCCHAN = 32;

   Int_t *fNumHits;

   static TypeIter_t fgThisType;
   ClassDef(Caen775Module,0)  //  Caen775 of a module; make your replacements

};

}

#endif
