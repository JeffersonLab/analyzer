#ifndef VETROCtdcModule_
#define VETROCtdcModule_

/////////////////////////////////////////////////////////////////////
//
//   VETROCtdcModule
//   JLab F1 TDC Module
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"

//const Int_t NTDCCHAN = 192;
//const Int_t MAXHIT   = 100;

namespace Decoder {

class VETROCtdcModule : public VmeModule {

public:

const Int_t NTDCCHAN = 192;
const Int_t MAXHIT   = 100;

   VETROCtdcModule() {};
   VETROCtdcModule(Int_t crate, Int_t slot);
   virtual ~VETROCtdcModule();

   using Module::GetData;
   using Module::LoadSlot;

   enum EResolution { ILO = 0, IHI = 1 };

   virtual void Init();
   virtual void Clear(const Option_t *opt="");
   virtual Bool_t IsSlot(UInt_t rdata);

   void SetResolution(Int_t which=0) {
     fResol = IHI;
     if (which==0) fResol=ILO;
     return;
   }
   EResolution GetResolution() const { return fResol; };
   Bool_t IsHiResolution() const { return (fResol==IHI); };

   Int_t GetNumHits(Int_t chan, Bool_t edge=0);// const { return fNumHits[chan]; };
   Int_t GetHit(Int_t chan, Int_t nhit=0, Bool_t edge=0); // if only the channel is specified, return first hit for that channel with edge equal 0
   Int_t GetFine(Int_t chan, Int_t nhit=0, Bool_t edge=0); // if only the channel is specified, return first hit for that channel with edge equal 0
   Double_t GetTevent() const { return tEVT; };
   Int_t Decode(const UInt_t *p) { return 0; };

   // Loads slot data for bank structures
   virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len);
// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const UInt_t* evbuffer, const UInt_t *pstop );

private:


   Int_t *fNumHitsP;
   Int_t *fNumHitsN;
   EResolution fResol;
   Int_t *fTdcDataP;  // Raw data of Leading(?) edges 
   Int_t *fTdcDataN;  // Raw data of Falling(?) edges 
   Int_t *fTdcFineP;  // Fine (raw) data of Leading(?) edges - for VETROC calibration only
   Int_t *fTdcFineN;  // Fine (raw) data of Leading(?) edges - for VETROC calibration only
   Double_t tEVT;

   Bool_t IsInit;
   Int_t slotmask, chanmask, datamask;

   static TypeIter_t fgThisType;
   ClassDef(VETROCtdcModule,0)  //  JLab F1 TDC Module, test version

};

}

#endif
