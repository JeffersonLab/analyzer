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

#include "Decoder.h"    // for EModuleType, Rtypes
#include "Helper.h"     // for ToInt
#include <memory>       // for unique_ptr
#include <string>       // for string
#include <string_view>  // for string_view
#include <utility>      // for cmp_less
#include <vector>       // for vector

class THaEvData;

class THaDetMap {

public:
  class Iterator;
  class MultiHitIterator;

  // Configuration data for a frontend module
  struct Module {
    UInt_t   crate;
    UInt_t   slot;
    UInt_t   lo;
    UInt_t   hi;
    UInt_t   first;      // logical number of first channel
    Int_t    model;      // model number of module (for ADC/TDC identification).
    UInt_t   plane;      // Detector plane
    UInt_t   signal;     // (eg. PosADC, NegADC, PosTDC, NegTDC)
    Int_t    refchan;    // for pipeline TDCs: reference channel number
    Int_t    refindex;   // for pipeline TDCs: index into reference channel map
    char     tag[5];     // Tag for grouping modules (4 chars max)
    Bool_t   reverse;    // Indicates that "first" corresponds to hi, not lo
    Bool_t   cmnstart;   // TDC in common start mode (default false)
    Decoder::EModuleType type;

    UInt_t GetNchan() const { return hi-lo+1; }
    Int_t  GetModel() const { return model; }
    Bool_t IsTDC()    const { return
        type == Decoder::EModuleType::kCommonStopTDC ||
        type == Decoder::EModuleType::kCommonStartTDC ||
        type == Decoder::EModuleType::kMultiFunctionTDC; }
    Bool_t IsADC()    const { return
        type == Decoder::EModuleType::kADC ||
        type == Decoder::EModuleType::kMultiFunctionADC; }
    Bool_t IsCommonStart() const { return cmnstart; }
    void   SetModel( Int_t model );
    // For legacy modules
    void   SetTDCMode( Bool_t cstart );
    void   MakeTDC();
    void   MakeADC();
    // Get logical detector channel from physical one, starting at 0.
    // For historical reasons, 'first' starts at 1, so subtract 1 here.
    Int_t  ConvertToLogicalChannel( UInt_t chan ) const
    { return Podd::ToInt(first + (reverse ? hi-chan : chan-lo) - 1); }
  };

  // Flags for GetMinMaxChan()
  enum ECountMode : Byte_t {
    kLogicalChan         = 0,
    kRefIndex            = 1
  };
  // Flags for Fill(). Fill(string_view) ignores all except kDoNotClear
  enum EFillFlags { // NOLINT(*-enum-size)
    kDoNotClear          = BIT(0),    // Don't clear the map first
    kSkipLogicalChannel  = BIT(9),    // Parse but ignore the logical channel number
    kFillLogicalChannel  = BIT(10),   // Parse the logical channel number
    kFillModel           = BIT(11),   // Parse the model number
    kFillRefIndex        = BIT(12),   // Parse the reference index
    kFillRefChan         = BIT(13),   // Parse the reference channel
    kFillPlane           = BIT(14),   // Parse the detector plane (for Hall C)
    kFillSignal          = BIT(15)    // Parse the signal type (for Hall C)
  };

  // Return codes from Fill() (returns negative EFillStatus values if error)
  enum EFillStatus : Byte_t { kOK, kNotEnoughData, kConversionError,
    kInvalidNumber, kFormatError, kUnknownTag, kUnknownModel, kIndexOutOfBounds
  };

  THaDetMap() : fStartAtZero(false), fHasTags(false) {}
  THaDetMap( const THaDetMap& );
  THaDetMap( THaDetMap&& ) = default;
  THaDetMap& operator=( const THaDetMap& );
  THaDetMap& operator=( THaDetMap&& ) = default;
  ~THaDetMap() = default;

  Iterator MakeIterator( const THaEvData& evdata );
  MultiHitIterator MakeMultiHitIterator( const THaEvData& evdata );

  Int_t   AddModule( UInt_t crate, UInt_t slot, UInt_t chan_lo, UInt_t chan_hi,
                     UInt_t first = 0, Int_t model = 0,
                     Int_t refindex = -1, Int_t refchan = -1,
                     UInt_t plane = 0, UInt_t signal = 0,
                     const std::string& tag = {} );
  void    Clear();
  Module* Find( UInt_t crate, UInt_t slot, UInt_t chan );
  Int_t   Fill( const std::vector<Int_t>& values, UInt_t flags = 0 );
  Int_t   Fill( std::string_view dbtxt, UInt_t flags = 0 );
  void    GetMinMaxChan( UInt_t& min, UInt_t& max,
                         ECountMode mode = kLogicalChan ) const;
  Module* GetModule( UInt_t i ) const;
  UInt_t  GetNchan( UInt_t i ) const;
  UInt_t  GetTotNumChan() const;
  UInt_t  GetSize() const { return fMap.size(); }
  Int_t   GetModel( UInt_t i ) const;
  Bool_t  IsADC( UInt_t i ) const;
  Bool_t  IsTDC( UInt_t i ) const;

  void    Print( Option_t* opt="" ) const;
  void    Sort();

  void    SetStartAtZero( Bool_t value ) { fStartAtZero = value; }

  static  Int_t  GetModel( const Module* d );
  static  Bool_t IsADC( const Module* d );
  static  Bool_t IsTDC( const Module* d );
  static  Int_t  InitCmap( Long64_t tloc = 0 );
  static  Decoder::THaCrateMap* GetCrateMap();

private:
  using ModuleVec_t = std::vector<std::unique_ptr<Module>>;
  ModuleVec_t fMap;          // Modules of this detector map
  Bool_t      fStartAtZero;  // Channels in this map start counting at 0
  Bool_t      fHasTags;      // At least one module has a tag string

  Module* uGetModule( UInt_t i ) const { return fMap[i].get(); }
  void    CopyMap( const ModuleVec_t& map );

  struct FillResult {
    EFillStatus err;
    Int_t       ret{0};
    std::string field;
  };
  FillResult FillImpl( std::string_view dbtxt, UInt_t flags );

  ClassDefNV(THaDetMap,2) // List of frontend modules associated with a detector

  //========================= Hit Iterators =====================================
public:
  // CRTP mix-in to get the right operator signatures ...
  template<class Derived>
  struct IncrOp {
  private:
    IncrOp() = default;
  public:
    Derived& operator++() {
      auto* obj = static_cast<Derived*>(this);
      obj->Derived::incr();
      return *obj;
    }
    Derived operator++(int) & {
      auto* obj = static_cast<Derived*>(this);
      Derived clone(*obj);
      ++(*obj);
      return clone;
    }
    friend Derived;
  };
  //___________________________________________________________________________
  // Utility classes for iterating over active channels in current event data
  class Iterator : public IncrOp<Iterator> {
  public:
    Iterator( const THaDetMap& detmap, const THaEvData& evdata,
              bool do_init = true );
    Iterator() = delete;
    Iterator( const Iterator& ) = default;
    Iterator( Iterator&& ) = default;
    Iterator& operator=( const Iterator& ) = default;
    Iterator& operator=( Iterator&& ) = default;
    virtual ~Iterator() = default;

    // Positioning
    virtual void incr();
    virtual void reset();
    UInt_t size() const { return fNTotChan; }

    // Status
    explicit virtual operator bool() const {
      return fIMod >= 0 and std::cmp_less(fIMod, fNMod) and
             fIChan >= 0 and std::cmp_less(fIChan, fNChan);
    }
    bool operator!() const { return !static_cast<bool>(*this); }

    // Current value
    class HitInfo_t {
    public:
      HitInfo_t() :
        module{nullptr}, type{Decoder::EModuleType::kUndefined},
        modtype{Decoder::EModuleType::kUndefined},
        ev{kMaxULong64}, crate{kMaxUInt}, slot{kMaxUInt}, chan{kMaxUInt},
        nhit{0}, hit{kMaxUInt}, lchan{-1} {}
      void set_crate_slot( const THaDetMap::Module* mod ) {
        if( mod->IsADC() )
          type = Decoder::EModuleType::kADC;
        else if( mod->IsTDC() )
          type = mod->IsCommonStart() ? Decoder::EModuleType::kCommonStartTDC
                                      : Decoder::EModuleType::kCommonStopTDC;
        modtype = mod->type;
        crate = mod->crate;
        slot  = mod->slot;
      }
      void reset() {
        module = nullptr; type = modtype = Decoder::EModuleType::kUndefined;
        ev = kMaxULong64;
        crate = slot = chan = hit = kMaxUInt; nhit = 0; lchan = -1;
      }
      Decoder::Module*     module; // Current frontend module being decoded
      Decoder::EModuleType type;   // Measurement type for current channel (ADC/TDC)
      Decoder::EModuleType modtype; // Module type (ADC/TDC/MultiFunctionADC etc.)
      ULong64_t  ev;   // Event number (for error messages)
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
    const THaDetMap* fDetMap;
    const THaEvData* fEvData;
    const Module*    fMod;
    HitInfo_t        fHitInfo;   // Current state of this iterator
    UInt_t fNMod;         // Number of modules in detector map (cached)
    UInt_t fNTotChan;     // Total number of detector map channels (cached)
    UInt_t fNChan;        // Number of channels active in current module
    Int_t  fIMod;         // Current module index in detector map
    Int_t  fIChan;        // Current channel

    // Formatter for exception messages
    std::string msg( const char* txt ) const;
  };

  class MultiHitIterator : public Iterator, public IncrOp<MultiHitIterator> {
  public:
    MultiHitIterator( const THaDetMap& detmap, const THaEvData& evdata,
                      bool do_init = true );
    MultiHitIterator() = delete;
    explicit operator bool() const override
    {
      return Iterator::operator bool() and fIHit >= 0 and
             std::cmp_less(fIHit, fHitInfo.nhit);
    }
    using IncrOp<MultiHitIterator>::operator++;
    void incr() override;
    void reset() override;
  protected:
    Int_t fIHit;         // Current raw hit number
  };
};

using DigitizerHitInfo_t = THaDetMap::Iterator::HitInfo_t;

//________ inlines ____________________________________________________________
inline THaDetMap::Module* THaDetMap::GetModule( UInt_t i ) const {
  return i<fMap.size() ? uGetModule(i) : nullptr;
}

inline Bool_t THaDetMap::IsADC( const Module* d ) {
  if( !d ) return false;
  return d->IsADC();
}

inline Bool_t THaDetMap::IsTDC( const Module* d ) {
  if( !d ) return false;
  return d->IsTDC();
}

inline Int_t THaDetMap::GetModel( const Module* d ) {
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
