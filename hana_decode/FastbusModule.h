#ifndef Podd_FastbusModule_h_
#define Podd_FastbusModule_h_

/////////////////////////////////////////////////////////////////////
//
//   FastbusModule
//   fastbus module class.  
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Module.h"

namespace Decoder {

class FastbusModule : public Module {

public:

   FastbusModule()
    : fHasHeader(false),
      fSlotMask(0), fSlotShift(0), fChanMask(0), fChanShift(0),
      fDataMask(0), fOptMask(0), fOptShift(0),
      fChan(0), fData(0), fRawData(0) {}
   FastbusModule( UInt_t crate, UInt_t slot );
   virtual ~FastbusModule() = default;

   using Module::LoadSlot;
   using Module::Init;

   virtual Int_t  Decode(const UInt_t *evbuffer);
   virtual Bool_t IsSlot(UInt_t rdata) { return (Slot(rdata)==fSlot); };
   virtual UInt_t LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop);
   void DoPrint() const;

   UInt_t GetOpt( UInt_t rdata) const { return Opt(rdata); };

   UInt_t Slot(UInt_t rdata) const { return (rdata>>fSlotShift); };
   UInt_t Chan(UInt_t rdata) const { return (rdata&fChanMask)>>fChanShift; };
   UInt_t Data(UInt_t rdata) const { return (rdata&fDataMask); };
   UInt_t Opt(UInt_t rdata) const { return (rdata&fOptMask)>>fOptShift; };

  virtual void SetSlot( UInt_t crate, UInt_t slot, UInt_t header = 0,
                        UInt_t mask = 0, Int_t modelnum = 0 );

protected:

   Bool_t fHasHeader;
   UInt_t fSlotMask, fSlotShift;
   UInt_t fChanMask, fChanShift;
   UInt_t fDataMask;
   UInt_t fOptMask, fOptShift;
   UInt_t fChan, fData, fRawData;

   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(FastbusModule,0)  // Fastbus module

};

}

#endif
