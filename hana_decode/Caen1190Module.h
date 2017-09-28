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

    Caen1190Module() : VmeModule() {}
    Caen1190Module(Int_t crate, Int_t slot);
    virtual ~Caen1190Module();

    using Module::GetData;

    virtual void Init();
    virtual void Clear(const Option_t *opt);
    virtual Int_t Decode(const UInt_t *p);
    virtual Int_t GetData(Int_t chan, Int_t hit) const;

    // Loads slot data.  if you don't define this, the base class's method is used
    virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );
    // Loads slot data for bank structures
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len);

  private:

    Int_t *fNumHits;
    Int_t *fTdcData;  // Raw data

    THaSlotData *slot_data;  // Need to fix if multi-threading becomes available
   
    struct tdc_data_struct {
      UInt_t evno, slot, chan, raw, chip_nr_hd, flags;
      Int_t status;
    } tdc_data;

    static TypeIter_t fgThisType;
    ClassDef(Caen1190Module,0)  //  Caen1190 of a module; make your replacements

      };

}

#endif
