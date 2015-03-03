#ifndef F1TDCModule_
#define F1TDCModule_

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

   F1TDCModule() {};
   F1TDCModule(Int_t crate, Int_t slot);
   virtual ~F1TDCModule();

   using Module::GetData;

   enum EResolution { ILO = 0, IHI = 1 };

   virtual void Init();
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

private:

// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const UInt_t* evbuffer, const UInt_t *pstop );

   Int_t fNumHits;
   EResolution fResol;
   Int_t *fTdcData;  // Raw data (either samples or pulse integrals)
   Bool_t IsInit;
   void Clear(const Option_t *opt);
   static TypeIter_t fgThisType;
   Int_t slotmask, chanmask, datamask;
   ClassDef(F1TDCModule,0)  //  JLab F1 TDC Module

};

}

#endif
