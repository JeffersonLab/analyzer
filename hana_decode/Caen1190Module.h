#ifndef Podd_Caen1190Module_h_
#define Podd_Caen1190Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Caen1190Module
//
/////////////////////////////////////////////////////////////////////


#include "VmeModule.h"
#include <cstring>  // for memset

namespace Decoder {

  class Caen1190Module : public VmeModule {

  public:

    Caen1190Module() : fNumHits(0), fTdcData(0), slot_data(0)
    { memset(&tdc_data, 0, sizeof(tdc_data)); }
    Caen1190Module(Int_t crate, Int_t slot);
    virtual ~Caen1190Module();

    using Module::GetData;

    virtual void Init();
    virtual void Clear(Option_t *opt);
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
      UInt_t glb_hdr_evno, glb_hdr_slno, glb_trl_wrd_cnt, glb_trl_slno, glb_trl_status;
      UInt_t chan, raw, flags, trig_time, chip_nr_hd;
      UInt_t hdr_chip_id, hdr_event_id, hdr_bunch_id;
      UInt_t trl_chip_id, trl_event_id, trl_word_cnt;
      Int_t status;
    } tdc_data;

    static TypeIter_t fgThisType;
    ClassDef(Caen1190Module,0)  //  Caen1190 of a module; make your replacements

      };

}

#endif
