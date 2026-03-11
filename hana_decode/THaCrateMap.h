#ifndef Podd_THaCrateMap_h_
#define Podd_THaCrateMap_h_

/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e. whether slots are
//  fastbus or vme, what the module types are, and
//  what header info to expect.  Probably nobody needs
//  to know about this except the author, and at present
//  an object of this class is a private member of the decoder.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "Decoder.h"
#include "TDatime.h"
#include <fstream>
#include <cstdio>  // for FILE
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <set>

namespace Decoder {
class THaCrateMap {
public:
  explicit THaCrateMap( const char* db = "cratemap" ); // Construct uninitialized

  int    init( const std::string& the_map );           // Initialize from text-block
  int    init( Long64_t tloc = 0 );                    // Initialize by Unix time.
  int    init( FILE* fi, const char* fname );          // Initialize from given file
  void   print( std::ostream& os = std::cout ) const;
  void   setDebug( Byte_t bits = BIT(0) ) { fDebug = bits; }

  bool   isInit() const { return fIsInit; }            // Successfully initialized
  void   Clear();                                      // Clear all data
  UInt_t GetNcrates() const;                           // Total number of used crates
  UInt_t GetSize() const;                              // Total number of used slots
  Long64_t GetInitTime() const { return fInitTime; }   // Initialization time, if given
  const char* GetName() const { return fDBfileName.c_str(); }
  const std::set<UInt_t>& GetUsedCrates() const;
  const std::set<UInt_t>& GetUsedSlots( UInt_t crate ) const;

  bool   isFastBus( UInt_t crate ) const;              // True if fastbus crate;
  bool   isVme( UInt_t crate ) const;                  // True if VME crate;
  bool   isCamac( UInt_t crate ) const;                // True if CAMAC crate;
  bool   isScalerCrate( UInt_t crate ) const;          // True if a Scaler crate
  bool   isBankStructure( UInt_t crate ) const;        // True if modules in banks
  bool   isAllBanks( UInt_t crate ) const;             // True if all modules in banks
  UInt_t getNslot( UInt_t crate ) const;               // Returns num occupied slots
  UInt_t getMinSlot( UInt_t crate ) const;             // Returns min slot number
  UInt_t getMaxSlot( UInt_t crate ) const;             // Returns max slot number
  UInt_t getTSROC() const;                             // Returns the crate number of the Trig. Super.

  Int_t  getModel( UInt_t crate, UInt_t slot ) const;  // Return module type
  UInt_t getHeader( UInt_t crate, UInt_t slot ) const; // Return header
  UInt_t getMask( UInt_t crate, UInt_t slot ) const;   // Return header mask
  Int_t  getBank( UInt_t crate, UInt_t slot ) const;   // Return bank number
  UInt_t getScalerCrate( UInt_t word ) const;          // Return scaler crate if word=header
  std::string getScalerLoc( UInt_t crate ) const;      // Return scaler crate location
  std::string getConfigStr( UInt_t crate, UInt_t slot ) const; // Configuration string
  UInt_t getNchan( UInt_t crate, UInt_t slot ) const;  // Max number of channels
  [[deprecated("Ndata is automatically adjusted internally. User code should ignore this value.")]]
  UInt_t getNdata( UInt_t, UInt_t ) const { return MAXDATA; };  // Max number of data words NOLINT(*-convert-member-functions-to-static)
  bool   crateUsed( UInt_t crate ) const;              // True if crate is used
  bool   slotUsed( UInt_t crate, UInt_t slot ) const;  // True if slot in crate is used
  bool   slotClear( UInt_t crate, UInt_t slot ) const; // Decide if not clear ea event
  void   setUnused( UInt_t crate, UInt_t slot );       // Disables this slot in crate
  void   setUnused( UInt_t crate );                    // Disables this crate

  static constexpr Int_t  CM_OK   = 0;
  static constexpr Int_t  CM_ERR  = -1;
  static const     UInt_t DEFAULT_TSROC;
  // FIXME remove these arbitrary limits
  static constexpr UInt_t MAXCHAN = 8192;
  static constexpr UInt_t MAXDATA = 65536;

  enum ECrateMapDebug : Byte_t { kAllowMissingModel = BIT(6) };

private:
  std::string fDBfileName;     // Database file name
  Long64_t    fInitTime;       // Database time stamp (Unix time)
  UInt_t      fTSROC;          // Crate (aka ROC) of the trigger supervisor
  bool        fIsInit;         // Successfully initialized
  Byte_t      fDebug;          // Enable debugging (bitmask)

  class SlotInfo_t {
  public:
    SlotInfo_t() : model(0), header(0), headmask(0xffffffff), bank(-1),
      nchan(0), slot(0), do_clear(true), type(EModuleType::kUndefined) {}
    Int_t    model;
    UInt_t   header;
    UInt_t   headmask;
    Int_t    bank;
    UShort_t nchan;
    UShort_t slot;     // self-reference, redundant but sometimes useful
    bool     do_clear;
    EModuleType type;
    std::string cfgstr;
  };

  // Crate information
  class CrateInfo_t {
  public:
    CrateInfo_t() : crate(kMaxUInt), crate_code(kUnknown), has_banks(false),
      all_banks(false), sltdat(MAXSLOT) {}
    using enum ECrateCode; // from Decoder.h
    Int_t ParseSlotInfo( const THaCrateMap* crmap, std::string& line );
    bool  IsFastBus()     const { return crate_code == kFastbus; }
    bool  IsVME()         const { return crate_code == kVME || crate_code == kScalerCrate; }
    bool  IsCamac()       const { return crate_code == kCamac; }
    bool  IsScalerCrate() const { return crate_code == kScalerCrate; }
    void  SetBankInfo();
    UInt_t       crate;     // Crate number, redundant but sometimes useful
    ECrateCode   crate_code;
    bool         has_banks;
    bool         all_banks;
    std::unordered_map<UInt_t, SlotInfo_t> sltdat;
    std::set<UInt_t> used_slots;
    std::string  scalerloc;
  };

  std::unordered_map<UInt_t, CrateInfo_t> fCrateDat;

  std::set<UInt_t> fUsedCrates;
  std::map<Int_t, std::string> fModuleCfg;

  CrateInfo_t& MakeCrate( UInt_t crate );
  const SlotInfo_t& FindSlot( UInt_t crate, UInt_t slot ) const;
  Int_t loadConfig( std::string& line, std::string& cfgstr ) const;
  Int_t setCrateType( UInt_t crate, const char* stype ); // set the crate type
  void  setUsed( UInt_t crate, UInt_t slot );
  Int_t ParseCrateInfo( const std::string& line, UInt_t& crate );

  static Int_t readFile( FILE* fi, std::string& text );

  template<class Proj,
    typename Result = std::invoke_result_t<Proj, CrateInfo_t>>
  auto GetCrateInfo( UInt_t crate, const Proj& p, Result defval = {} ) const
  {
    if( auto it = fCrateDat.find(crate); it != fCrateDat.end() )
      return std::invoke(p, it->second);
    return defval;
  }

  template<class Proj,
    typename Result = std::invoke_result_t<Proj, SlotInfo_t>>
  auto GetSlotInfo( UInt_t crate, UInt_t slot, const Proj& p, Result defval = {} ) const
  {
    const auto& sl =
      GetCrateInfo( crate, &CrateInfo_t::sltdat );
    if( auto jt = sl.find(slot); jt != sl.end() )
      return std::invoke(p, jt->second);
    return defval;
  }

  ClassDefNV(THaCrateMap, 0) // Map of modules in DAQ crates
};

//=============== inline functions ================================
inline
UInt_t THaCrateMap::GetNcrates() const
{
  return fUsedCrates.size();
}

inline
bool THaCrateMap::isFastBus( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::IsFastBus);
}

inline
bool THaCrateMap::isVme( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::IsVME);
}

inline
bool THaCrateMap::isCamac( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::IsCamac);
}

inline
bool THaCrateMap::isScalerCrate( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::IsScalerCrate);
}

inline
bool THaCrateMap::isBankStructure( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::has_banks);
}

inline
bool THaCrateMap::isAllBanks( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::all_banks);
}

inline
bool THaCrateMap::crateUsed( UInt_t crate ) const
{
  return fCrateDat.contains(crate);
}

inline
bool THaCrateMap::slotUsed( UInt_t crate, UInt_t slot ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::sltdat).contains(slot);
}

inline
bool THaCrateMap::slotClear( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::do_clear, true );
}

inline
Int_t THaCrateMap::getModel( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::model );
}

inline
UInt_t THaCrateMap::getMask( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::headmask, 0xffffffff );
}

inline
Int_t THaCrateMap::getBank( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::bank, -1 );
}

inline
UInt_t THaCrateMap::getNchan( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::nchan );
}

inline
UInt_t THaCrateMap::getNslot( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::sltdat).size();
}

inline
std::string THaCrateMap::getScalerLoc( UInt_t crate ) const
{
  return GetCrateInfo(crate, &CrateInfo_t::scalerloc);
}

inline
UInt_t THaCrateMap::getMinSlot( UInt_t crate ) const
{
  const auto& used = GetCrateInfo(crate, &CrateInfo_t::used_slots);
  if( used.empty() )
    return kMaxUInt;
  return *used.begin();
}

inline
UInt_t THaCrateMap::getMaxSlot( UInt_t crate ) const
{
  const auto& used = GetCrateInfo(crate, &CrateInfo_t::used_slots);
  if( used.empty() )
    return 0;
  return *used.rbegin();
}

inline
UInt_t THaCrateMap::getTSROC() const
{
  return fTSROC;
}

inline
UInt_t THaCrateMap::getHeader( UInt_t crate, UInt_t slot ) const
{
  return GetSlotInfo(crate, slot, &SlotInfo_t::header );
}

inline
const std::set<UInt_t>& THaCrateMap::GetUsedCrates() const
{
  return fUsedCrates;
}

inline
const std::set<UInt_t>& THaCrateMap::GetUsedSlots( UInt_t crate ) const
{
  static const std::set<UInt_t> nullset;
  if( auto it = fCrateDat.find(crate); it != fCrateDat.end() )
    return it->second.used_slots;
  return nullset;
}

inline const THaCrateMap::SlotInfo_t&
THaCrateMap::FindSlot( UInt_t crate, UInt_t slot ) const
{
  static const SlotInfo_t nullslot;
  if( auto it = fCrateDat.find(crate); it != fCrateDat.end() ) {
    const auto& slots = it->second.sltdat;
    if( auto jt = slots.find(slot); jt != slots.end() && jt->second.model != 0 )
      return jt->second;
  }
  return nullslot;
}

} // namespace Decoder

#endif
