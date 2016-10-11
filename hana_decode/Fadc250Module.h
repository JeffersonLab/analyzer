#ifndef Fadc250Module_
#define Fadc250Module_

/////////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
/////////////////////////////////////////////////////////////////////

#define NADCCHAN 16

#include "VmeModule.h"
#include "stdint.h"
#include <vector>

namespace Decoder {

  class Fadc250Module : public VmeModule {   // Inheritance

  public:
    
    Fadc250Module();                         // Default constructor
    Fadc250Module(Int_t crate, Int_t slot);  // Constructor
    virtual ~Fadc250Module();                // Virtual constructor

    using Module::GetData;
    using Module::LoadSlot;

    virtual void Clear(const Option_t *opt="");
    virtual void Init();
    virtual void CheckDecoderStatus() const;
    virtual void CheckDecoderStatus(Int_t crate, Int_t slot) const;
    virtual Int_t GetPulseIntegralData(Int_t chan, Int_t ievent) const;
    virtual Int_t GetEmulatedPulseIntegralData(Int_t chan) const;
    virtual Int_t GetPulseTimeData(Int_t chan, Int_t ievent) const;
    virtual Int_t GetPulsePeakData(Int_t chan, Int_t ievent) const;
    virtual Int_t GetPulsePedestalData(Int_t chan, Int_t ievent) const;
    virtual Int_t GetPulseSamplesData(Int_t chan, Int_t ievent) const;
    virtual std::vector<uint32_t> GetPulseSamplesVector(Int_t chan) const;
    virtual Int_t GetNumFadcEvents(Int_t chan) const;
    virtual Int_t GetNumFadcSamples(Int_t chan, Int_t ievent) const;
    virtual Int_t GetFadcMode(Int_t chan) const;
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop);
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, Int_t pos, Int_t len);
    virtual Int_t Decode(const UInt_t *pdat);
    virtual Bool_t IsMultiFunction();
    virtual Bool_t HasCapability(Decoder::EModuleType type);
            
  private:

    struct fadc_data_struct {
      // Header data objects
      uint32_t slot_blk_hdr, mod_id, iblock_num, nblock_events;  // Block header objects
      uint32_t PL, NSA, NSB;                                     // Block header objects cont.
      uint32_t slot_blk_trl, nwords_inblock;                     // Block trailer objects
      uint32_t slot_evt_hdr, evt_num;                            // Event header objects
      uint32_t trig_time;                                        // Trigger time objects
      // Window raw data objects
      uint32_t chan, win_width;                                  // FADC channel, window width
      uint32_t samples;                                          // FADC raw data samples
      bool overflow, invalid_samples;                            // True if any sample's "overflow" or "not valid" bit set, respectively
      // Pulse raw data objects
      uint32_t pulse_num, sample_num_tc;                         // FADC pulse number, sample number of threshold crossing
      // Pulse integral data objects
      uint32_t qual_factor, pulse_integral;                      // FADC quality factor, pulse integral
      // Pulse time data objects
      uint32_t coarse_pulse_time, fine_pulse_time, time;         // FADC pulse coarse time, pulse fine time, pulse time
      // Pulse pedestal data objects
      uint32_t pedestal, pulse_peak;                             // FADC pedestal, pulse peak value
      // Scaler data objects
      uint32_t scaler_words;                                     // FADC scaler words
    } fadc_data;  //  fadc_data_struct

    // FIXME: perhaps better as a vector of a structure?
    std::vector<uint32_t> fPulseIntegral[NADCCHAN], fPulseTime[NADCCHAN];
    std::vector<uint32_t> fPulsePeak[NADCCHAN], fPulsePedestal[NADCCHAN];
    std::vector<uint32_t> fPulseSamples[NADCCHAN];

    Bool_t data_type_4, data_type_6, data_type_7, data_type_8, data_type_10;
    Bool_t block_header_found, block_trailer_found, event_header_found;

    void ClearDataVectors();
    void PopulateDataVector(std::vector<uint32_t> data_vector[NADCCHAN], uint32_t chan, uint32_t data);
    Int_t SumVectorElements(std::vector<uint32_t> data_vector) const;

    Bool_t slots_match;
   
    static TypeIter_t fgThisType;
    ClassDef(Fadc250Module,0)  //  JLab FADC 250 Module

      } ;  // Fadc250Module class

}  // Decoder namespace

#endif
