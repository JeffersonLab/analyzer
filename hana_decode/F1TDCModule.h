#ifndef Podd_F1TDCModule_h_
#define Podd_F1TDCModule_h_

/////////////////////////////////////////////////////////////////////
//
//   F1TDCModule
//   JLab F1 TDC Module
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"
#include <vector>

namespace Decoder {

class F1TDCModule : public VmeModule {

public:

   F1TDCModule() : fNumHits(0), fResol(ILO), IsInit(false),
                   slotmask(0), chanmask(0), datamask(0) {}
   F1TDCModule( UInt_t crate, UInt_t slot );
   virtual ~F1TDCModule() = default;

   using VmeModule::GetData;
   using VmeModule::LoadSlot;
   using VmeModule::Init;

   enum EResolution { ILO = 0, IHI = 1 };

   virtual void Init();
   virtual void Clear(Option_t *opt="");
   virtual Bool_t IsSlot(UInt_t rdata);
   virtual UInt_t GetData( UInt_t chan, UInt_t hit) const;

   void SetResolution(Int_t which=0) {
     fResol = (which==0) ? ILO : IHI;
   }
   EResolution GetResolution() const { return fResol; };
   Bool_t IsHiResolution() const { return (fResol==IHI); };

   UInt_t GetNumHits() const { return fNumHits; };
   Int_t Decode(const UInt_t*) { return 0; };

private:

// Loads sldat and increments ptr to evbuffer
  UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, const UInt_t* pstop );

   UInt_t fNumHits;
   EResolution fResol;
   std::vector<UInt_t> fTdcData;  // Raw data (either samples or pulse integrals)
   Bool_t IsInit;
   UInt_t slotmask, chanmask, datamask;

   static TypeIter_t fgThisType;
   ClassDef(F1TDCModule,0)  //  JLab F1 TDC Module

};

}

#endif
