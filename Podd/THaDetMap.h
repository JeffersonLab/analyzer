#ifndef Podd_THaDetMap_h_
#define Podd_THaDetMap_h_

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// Standard detector map.
// The detector map defines the hardware channels that correspond to a
// single detector. Typically, "channels" are Fastbus or VME addresses
// characterized by
//
//   Crate, Slot, range of channels
//
//////////////////////////////////////////////////////////////////////////

//FIXME: Several things here are duplicates of information that already is
// (or should be) available from the cratemap:  specifically "model" and
// "resolution", as well as the "model map" in AddModule. We should get it from
// there -  to ensure consistency and have one less maintenance headache.

#include "THaEvData.h"
#include "Decoder.h"
#include <vector>
#include <string>
#include <memory>

class THaDetMap {

public:
  class Iterator;
  class MultiHitIterator;

  // Configuration data for a frontend module
  class Module {
  public:
    Module() = default;
#if __clang__ || __GNUC__ > 4
    Module( const Module& rhs ) = default;
    Module& operator=( const Module& rhs ) = default;
#else
    // work around apparent g++ 4 bug
    Module( Module& rhs ) = default;
    Module& operator=( Module& rhs ) = default;
#endif
    UInt_t   crate;
    UInt_t   slot;
    UInt_t   lo;
    UInt_t   hi;
    UInt_t   first;      // logical number of first channel
    Int_t    model;      // model number of module (for ADC/TDC identification).
    Decoder::ChannelType type;
    UInt_t   plane;      // Detector plane
    UInt_t   signal;     // (eg. PosADC, NegADC, PosTDC, NegTDC)
    Int_t    refchan;    // for pipeline TDCs: reference channel number
    Int_t    refindex;   // for pipeline TDCs: index into reference channel map
    Double_t resolution; // Resolution (s/chan) for TDCs
    Bool_t   reverse;    // Indicates that "first" corresponds to hi, not lo
    Bool_t   cmnstart;   // TDC in common start mode (default false)

    bool   operator==( const Module& rhs ) const
    { return crate == rhs.crate && slot == rhs.slot; }
    bool   operator!=( const Module& rhs ) const
    { return !(*this==rhs); }
    UInt_t GetNchan() const { return hi-lo+1; }
    Int_t  GetModel() const { return model; }
    Bool_t IsTDC()    const { return
        type == Decoder::ChannelType::kCommonStopTDC ||
        type == Decoder::ChannelType::kCommonStartTDC ||
        type == Decoder::ChannelType::kMultiFunctionTDC; }
    Bool_t IsADC()    const { return
        type == Decoder::ChannelType::kADC ||
        type == Decoder::ChannelType::kMultiFunctionADC; }
    Bool_t IsCommonStart() const { return cmnstart; }
    void   SetModel( Int_t model );
    void   SetResolution( Double_t resolution );
    // For legacy modules
    void   SetTDCMode( Bool_t cstart );
    void   MakeTDC();
    void   MakeADC();
    // Get logical detector channel from physical one, starting at 0.
    // For historical reasons, 'first' starts at 1, so subtract 1 here.
    Int_t  ConvertToLogicalChannel( UInt_t chan ) const
    { return static_cast<Int_t>(first + (reverse ? hi-chan : chan-lo) - 1); }
  };

  // Flags for GetMinMaxChan()
  enum ECountMode {
    kLogicalChan         = 0,
    kRefIndex            = 1
  };
  // Flags for Fill()
  enum EFillFlags {
    kDoNotClear          = BIT(0),    // Don't clear the map first
    kSkipLogicalChannel  = BIT(9),    // Parse but ignore the logical channel number
    kFillLogicalChannel  = BIT(10),   // Parse the logical channel number
    kFillModel           = BIT(11),   // Parse the model number
    kFillRefIndex        = BIT(12),   // Parse the reference index
    kFillRefChan         = BIT(13),   // Parse the reference channel
    kFillPlane           = BIT(14),   // Parse the detector plane (for Hall C)
    kFillSignal          = BIT(15)    // Parse the signal type (for Hall C)
  };

  THaDetMap() : fStartAtZero(false) {}
  THaDetMap( const THaDetMap& );
  THaDetMap& operator=( const THaDetMap& );
  virtual ~THaDetMap() = default;

  THaDetMap::Iterator MakeIterator( const THaEvData& evdata );
  THaDetMap::MultiHitIterator MakeMultiHitIterator( const THaEvData& evdata );

  virtual Int_t     AddModule( UInt_t crate, UInt_t slot,
                               UInt_t chan_lo, UInt_t chan_hi,
                               UInt_t first = 0, Int_t model = 0,
                               Int_t refindex = -1, Int_t refchan = -1,
                               UInt_t plane = 0, UInt_t signal = 0 );
          void      Clear()  { fMap.clear(); }
  virtual Module*   Find( UInt_t crate, UInt_t slot, UInt_t chan );
  virtual Int_t     Fill( const std::vector<Int_t>& values, UInt_t flags = 0 );
          void      GetMinMaxChan( UInt_t& min, UInt_t& max,
                                   ECountMode mode = kLogicalChan ) const;
          Module*   GetModule( UInt_t i ) const;
          UInt_t    GetNchan( UInt_t i ) const;
          UInt_t    GetTotNumChan() const;
          UInt_t    GetSize() const { return fMap.size(); }

          Int_t     GetModel( UInt_t i ) const;
          Bool_t    IsADC( UInt_t i ) const;
          Bool_t    IsTDC( UInt_t i ) const;
  static  Int_t     GetModel( Module* d );
  static  Bool_t    IsADC( Module* d );
  static  Bool_t    IsTDC( Module* d );

  virtual void      Print( Option_t* opt="" ) const;
  virtual void      Reset();
  virtual void      Sort();

  void SetStartAtZero( Bool_t value ) { fStartAtZero = value; }

protected:
  using ModuleVec_t = std::vector<std::unique_ptr<Module>>;
  ModuleVec_t fMap;     // Modules of this detector map

  Module* uGetModule( UInt_t i ) const { return fMap[i].get(); }
  void CopyMap( const ModuleVec_t& map );

  // Channels in this map start counting at 0. Used by hit iterators.
  Bool_t fStartAtZero;

  //___________________________________________________________________________
  // Utility classes for iterating over active channels in current event data
public:
  class Iterator {
  public:
    Iterator( THaDetMap& detmap, const THaEvData& evdata, bool do_init = true );
    Iterator() = delete;
    virtual ~Iterator() = default;

    // Positioning
    virtual Iterator& operator++();
    const Iterator operator++(int) {
      Iterator clone(*this); ++(*this); return clone;
    }
    virtual void reset();
    UInt_t size() const { return fNTotChan; }

    // Status
    explicit virtual operator bool() const {
      return (fIMod >= 0 and fIMod < static_cast<Int_t>(fNMod) and
              fIChan >= 0 and fIChan < static_cast<Int_t>(fNChan));
    }
    bool operator!() const { return !static_cast<bool>(*this); }

    // Current value
    class HitInfo_t {
    public:
      HitInfo_t() :
        module{nullptr}, type{Decoder::ChannelType::kUndefined},
        modtype{Decoder::ChannelType::kUndefined},
        ev{kMaxUInt}, crate{kMaxUInt}, slot{kMaxUInt}, chan{kMaxUInt}, nhit{0},
        hit{kMaxUInt}, lchan{-1} {}
      void set_crate_slot( const THaDetMap::Module* mod ) {
        if( mod->IsADC() )
          type = Decoder::ChannelType::kADC;
        else if( mod->IsTDC() )
          type = mod->IsCommonStart() ? Decoder::ChannelType::kCommonStartTDC
                                      : Decoder::ChannelType::kCommonStopTDC;
        modtype = mod->type;
        crate = mod->crate;
        slot  = mod->slot;
      }
      void reset() {
        module = nullptr; type = modtype = Decoder::ChannelType::kUndefined;
        crate = slot = chan = hit = kMaxUInt; nhit = 0; lchan = -1;
      }
      Decoder::Module*     module; // Current frontend module being decoded
      Decoder::ChannelType type;   // Measurement type for current channel (ADC/TDC)
      Decoder::ChannelType modtype; // Module type (ADC/TDC/MultiFunctionADC etc.)
      UInt_t  ev;      // Event number (for error messages)
      UInt_t  crate;   // Hardware crate
      UInt_t  slot;    // Hardware slot
      UInt_t  chan;    // Physical channel in current module
      UInt_t  nhit;    // Number of hits in current channel
      UInt_t  hit;     // Hit number in current channel
      Int_t   lchan;   // Logical channel according to detector map
    };

    const HitInfo_t* operator->() const { return &fHitInfo; }
    const HitInfo_t& operator* () const { return fHitInfo; }

  protected:
    THaDetMap& fDetMap;
    const THaEvData& fEvData;
    const THaDetMap::Module* fMod;
    UInt_t fNMod;         // Number of modules in detector map (cached)
    UInt_t fNTotChan;     // Total number of detector map channels (cached)
    UInt_t fNChan;        // Number of channels active in current module
    Int_t  fIMod;         // Current module index in detector map
    Int_t  fIChan;        // Current channel
    HitInfo_t fHitInfo;   // Current state of this iterator

    // Formatter for exception messages
    std::string msg( const char* txt ) const;
  };

  class MultiHitIterator : public Iterator {
  public:
    MultiHitIterator( THaDetMap& detmap, const THaEvData& evdata,
                      bool do_init = true );
    MultiHitIterator() = delete;
    virtual ~MultiHitIterator() = default;
    explicit virtual operator bool() const {
      return (Iterator::operator bool() and fIHit >= 0 and
              fIHit < static_cast<Int_t>(fHitInfo.nhit));
    }
    virtual Iterator& operator++();
    virtual void reset();
  protected:
    Int_t fIHit;         // Current raw hit number
  };

  ClassDef(THaDetMap,1)   //The standard detector map
};

using DigitizerHitInfo_t = THaDetMap::Iterator::HitInfo_t;

//________ inlines ____________________________________________________________
inline THaDetMap::Module* THaDetMap::GetModule( UInt_t i ) const {
  return i<fMap.size() ? uGetModule(i) : nullptr;
}

inline Bool_t THaDetMap::IsADC(Module* d) {
  if( !d ) return false;
  return d->IsADC();
}

inline Bool_t THaDetMap::IsTDC(Module* d) {
  if( !d ) return false;
  return d->IsTDC();
}

inline Int_t THaDetMap::GetModel(Module* d) {
  if( !d ) return 0;
  return d->GetModel();
}

inline Bool_t THaDetMap::IsADC( UInt_t i ) const {
  if( i >= fMap.size() ) return false;
  return uGetModule(i)->IsADC();
}

inline Bool_t THaDetMap::IsTDC( UInt_t i ) const {
  if( i >= fMap.size() ) return false;
  return uGetModule(i)->IsTDC();
}

inline Int_t THaDetMap::GetModel( UInt_t i ) const {
  if( i >= fMap.size() ) return 0;
  return uGetModule(i)->GetModel();
}

inline UInt_t THaDetMap::GetNchan( UInt_t i ) const {
  // Return the number of channels of the i-th module
  if( i >= fMap.size() ) return 0;
  return uGetModule(i)->GetNchan();
}

#endif
