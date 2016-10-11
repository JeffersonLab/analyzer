#ifndef Caen1190Module_
#define Caen1190Module_

/////////////////////////////////////////////////////////////////////
//
//   Caen1190Module
//
/////////////////////////////////////////////////////////////////////


#include "VmeModule.h"

namespace Decoder {

class Caen1190Module : public VmeModule {

public:

   Caen1190Module() {};
   Caen1190Module(Int_t crate, Int_t slot);
   virtual ~Caen1190Module();

   using Module::GetData;
   using Module::LoadSlot;

   virtual Int_t GetData(Int_t chan, Int_t hit) const;
   virtual void Init();
   virtual void Clear(const Option_t *opt="");
   virtual Int_t Decode(const UInt_t *p) { return 0; };

// Loads slot data.  if you don't define this, the base class's method is used
  virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );

private:

   Int_t *fNumHits;
   Int_t *fTdcData;  // Raw data	

   static TypeIter_t fgThisType;
   ClassDef(Caen1190Module,0)  //  Caen1190 of a module; make your replacements

};

}

#endif
