#ifndef Podd_Caen1190Module_h_
#define Podd_Caen1190Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Caen1190Module
//
/////////////////////////////////////////////////////////////////////


#include "VmeModule.h"
#include <cstring>  // for memset
#include <vector>

namespace Decoder {

  class Caen1190Module : public VmeModule {

  public:

    Caen1190Module() : slot_data(nullptr) {}
    Caen1190Module(Int_t crate, Int_t slot);
    virtual ~Caen1190Module() = default;

    using Module::GetData;

    virtual void  Init();
    virtual void  Clear(Option_t *opt="");
    virtual Int_t Decode(const UInt_t *p);
    virtual Int_t GetData(Int_t chan, Int_t hit) const;
    virtual Int_t GetOpt(Int_t chan, Int_t hit) const;

    // Loads slot data.  if you don't define this, the base class's method is used
    virtual Int_t LoadSlot(THaSlotData *sldat,  const UInt_t *evbuffer, const UInt_t *pstop );
    // Loads slot data for bank structures
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len);

  private:

    std::vector<Int_t> fNumHits;
    std::vector<Int_t> fTdcData;  // Raw data
    std::vector<Int_t> fTdcOpt;  // Edge flag =0 Leading edge, = 1 Trailing edge

    THaSlotData *slot_data;  // Need to fix if multi-threading becomes available
   
    class tdcData {
    public:
      tdcData() :
        glb_hdr_evno(0), glb_hdr_slno(0), glb_trl_wrd_cnt(0), glb_trl_slno(0),
        glb_trl_status(0), chan(0), raw(0), opt(0), flags(0), trig_time(0),
        chip_nr_hd(0), hdr_chip_id(0), hdr_event_id(0), hdr_bunch_id(0),
        trl_chip_id(0), trl_event_id(0), trl_word_cnt(0), status(0) {}
      void clear() { memset(this, 0, sizeof(tdcData)); }
      UInt_t glb_hdr_evno, glb_hdr_slno, glb_trl_wrd_cnt, glb_trl_slno, glb_trl_status;
      UInt_t chan, raw , opt , flags, trig_time, chip_nr_hd;
      UInt_t hdr_chip_id, hdr_event_id, hdr_bunch_id;
      UInt_t trl_chip_id, trl_event_id, trl_word_cnt;
      Int_t status;
    } tdc_data;

    static TypeIter_t fgThisType;
    ClassDef(Caen1190Module,0)  //  Caen1190 of a module; make your replacements

      };

}

#endif
