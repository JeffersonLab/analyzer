#ifndef Podd_F1TDCModule_h_
#define Podd_F1TDCModule_h_

/////////////////////////////////////////////////////////////////////
//
//   F1TDCModule
//   JLab F1 TDC Module
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"

namespace Decoder {

class F1TDCModule : public VmeModule {

public:

   F1TDCModule() : VmeModule() {}
   F1TDCModule(Int_t crate, Int_t slot);
   virtual ~F1TDCModule();

   using Module::GetData;
   using Module::LoadSlot;

   enum EResolution { ILO = 0, IHI = 1 };

   virtual void Init();
   virtual void Clear(const Option_t *opt="");
   virtual Bool_t IsSlot(UInt_t rdata);
   virtual Int_t GetData(Int_t chan, Int_t hit) const;

   void SetResolution(Int_t which=0) {
     fResol = IHI;
     if (which==0) fResol=ILO;
     return;
   }
   EResolution GetResolution() const { return fResol; };
   Bool_t IsHiResolution() const { return (fResol==IHI); };

   Int_t GetNumHits() const { return fNumHits; };
   Int_t Decode(const UInt_t*) { return 0; };

private:

// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const UInt_t* evbuffer, const UInt_t *pstop );

   Int_t fNumHits;
   EResolution fResol;
   Int_t *fTdcData;  // Raw data (either samples or pulse integrals)
   Bool_t IsInit;
   Int_t slotmask, chanmask, datamask;

   static TypeIter_t fgThisType;
   ClassDef(F1TDCModule,0)  //  JLab F1 TDC Module

};

}

#endif
