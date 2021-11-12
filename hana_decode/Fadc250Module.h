#ifndef Podd_Fadc250Module_h_
#define Podd_Fadc250Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
/////////////////////////////////////////////////////////////////////

#include "PipeliningModule.h"
#include <vector>
#include <cstdint>
#include <cstring>  // for memset

namespace Decoder {

  class Fadc250Module : public PipeliningModule {

  public:
    
    Fadc250Module() : Fadc250Module(0,0) {}
    Fadc250Module( UInt_t crate, UInt_t slot );
    virtual ~Fadc250Module();

    using PipeliningModule::GetData;
    using PipeliningModule::Init;

    virtual void Clear( Option_t *opt="" );
    virtual void Init();
    virtual void CheckDecoderStatus() const;
    virtual UInt_t GetPulseIntegralData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetEmulatedPulseIntegralData( UInt_t chan ) const;
    virtual UInt_t GetPulseTimeData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPulseCoarseTimeData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPulseFineTimeData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPulsePeakData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPulsePedestalData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPulseSamplesData( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetPedestalQuality( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetOverflowBit( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetUnderflowBit( UInt_t chan, UInt_t ievent ) const;
    virtual std::vector<uint32_t> GetPulseSamplesVector( UInt_t chan ) const;
    virtual Int_t  GetFadcMode() const;
    virtual Int_t  GetMode() const { return GetFadcMode(); };
    virtual UInt_t GetNumFadcEvents( UInt_t chan ) const;
    virtual UInt_t GetNumFadcSamples( UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, const UInt_t* pstop );
    virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, UInt_t pos, UInt_t len );
    virtual Int_t  Decode( const UInt_t* data );
    virtual Bool_t IsMultiFunction() { return true; }
    virtual Bool_t HasCapability( Decoder::EModuleType type );
    virtual UInt_t GetData( Decoder::EModuleType mtype, UInt_t chan, UInt_t ievent ) const;
    virtual UInt_t GetNumEvents( Decoder::EModuleType mtype, UInt_t ichan ) const;
    virtual UInt_t GetNumEvents() const { return GetNumEvents(0); } ;
    virtual UInt_t GetNumEvents( UInt_t ichan) const { return GetNumFadcEvents(ichan); } ;
    virtual UInt_t GetNumSamples( UInt_t ichan) const { return GetNumFadcSamples(ichan, 0);};
    virtual UInt_t GetTriggerTime() const;

  private:

    static const size_t NADCCHAN = 16;

    struct fadc_data_struct {
      // Header data objects
      uint32_t slot_blk_hdr, mod_id, iblock_num, nblock_events;     // Block header objects
      uint32_t PL, NSA, NSB;                                        // Block header objects cont.
      uint32_t slot_blk_trl, nwords_inblock;                        // Block trailer objects
      uint32_t slot_evt_hdr, evt_num;                               // Event header objects (pre  0x0C00) 
      uint32_t eh_trig_time, trig_num;                              // Event header objects (post 0x0C00)
      uint32_t trig_time_w1, trig_time_w2;                          // Trigger time objects
      uint64_t trig_time;                                           // Trigger time objects cont.
      // Window raw data objects
      uint32_t chan, win_width;                                     // FADC channel, window width
      uint32_t samples;                                             // FADC raw data samples
      bool overflow, invalid_samples;                               // True if any sample's "overflow" or "not valid" bit set, respectively
      // Pulse raw data objects
      uint32_t pulse_num, sample_num_tc;                            // FADC pulse number, sample number of threshold crossing
      // Pulse integral data objects
      uint32_t qual_factor, pulse_integral;                         // FADC quality factor, pulse integral
      // Pulse time data objects
      uint32_t coarse_pulse_time, fine_pulse_time, time;            // FADC pulse coarse time, pulse fine time, pulse time
      // Pulse pedestal data objects
      uint32_t pedestal, pulse_peak, pedestal_sum;                  // FADC pedestal, pulse peak value
      // Scaler data objects
      uint32_t scaler_words;                                        // FADC scaler words
      // Event block objects
      uint32_t evnt_of_blk;                                         // FADC event block information
      // Integral word objects
      uint32_t nsa_ext, sample_sum;                                 // FADC pulse parameters
      uint32_t samp_overflow, samp_underflow, samp_over_thresh;     // FADC pulse parameters
      // Time word object
      uint32_t peak_beyond_nsa, peak_not_found, peak_above_maxped;  // FADC pulse parameters
      void clear() { memset(this, 0, sizeof(fadc_data_struct)); }
    } __attribute__((aligned(128))) fadc_data;  // fadc_data_struct

    struct fadc_pulse_data {
      std::vector<uint32_t> integral, time, peak, pedestal;
      std::vector<uint32_t> samples, coarse_time, fine_time;
      std::vector<uint32_t> pedestal_quality, overflow, underflow;
      void clear() {
	integral.clear(); time.clear(); peak.clear(); pedestal.clear();
	samples.clear(); coarse_time.clear(); fine_time.clear();
	pedestal_quality.clear(); overflow.clear(); underflow.clear();
      }
    } __attribute__((aligned(128)));
    std::vector<fadc_pulse_data> fPulseData; // Pulse data for each channel

    Bool_t data_type_4, data_type_6, data_type_7, data_type_8, data_type_9, data_type_10;
    Bool_t block_header_found, block_trailer_found, event_header_found, slots_match;

    void ClearDataVectors();
    void PopulateDataVector( std::vector<uint32_t>& data_vector, uint32_t data ) const;
    static uint32_t SumVectorElements( const std::vector<uint32_t>& data_vector );
    void LoadTHaSlotDataObj( THaSlotData* sldat );
    void PrintDataType() const;

    void DecodeBlockHeader( UInt_t pdat, uint32_t data_type_id );
    void DecodeBlockTrailer( UInt_t pdat );
    void DecodeEventHeader( UInt_t pdat );
    void DecodeTriggerTime( UInt_t pdat, uint32_t data_type_id );
    void DecodeWindowRawData( UInt_t pdat, uint32_t data_type_id );
    void DecodePulseRawData( UInt_t pdat, uint32_t data_type_id );
    void DecodePulseIntegral( UInt_t pdat );
    void DecodePulseTime( UInt_t pdat );
    void DecodePulseParameters( UInt_t pdat, uint32_t data_type_id );
    void DecodePulsePedestal( UInt_t pdat );
    void DecodeScalerHeader( UInt_t pdat );
    void UnsupportedType( UInt_t pdat, uint32_t data_type_id );

    static TypeIter_t fgThisType;

    ClassDef(Fadc250Module,0)  //  JLab FADC 250 Module
  };  // Fadc250Module class

}  // Decoder namespace

#endif
