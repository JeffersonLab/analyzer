#ifndef Podd_THaCrateMap_h_
#define Podd_THaCrateMap_h_

/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e whether slots are
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
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <array>

namespace Decoder {

class THaCrateMap {
 public:
     static const UInt_t MAXCHAN;
     static const UInt_t MAXDATA;

     explicit THaCrateMap( const char* db = "cratemap" ); // Construct uninitialized
     virtual ~THaCrateMap() = default;
     bool isFastBus( UInt_t crate ) const;          // True if fastbus crate;
     bool isVme( UInt_t crate ) const;              // True if VME crate;
     bool isCamac( UInt_t crate ) const;            // True if CAMAC crate;
     bool isScalerCrate( UInt_t crate ) const;      // True if a Scaler crate
     bool isBankStructure( UInt_t crate ) const;    // True if modules in banks
     bool isAllBanks( UInt_t crate ) const;         // True if all modules in banks
     UInt_t getNslot( UInt_t crate ) const;         // Returns num occupied slots
     UInt_t getMinSlot( UInt_t crate ) const;       // Returns min slot number
     UInt_t getMaxSlot( UInt_t crate ) const;       // Returns max slot number
     UInt_t getTSROC() const;                       // Returns the crate number of the Trig. Super.

 // This class must inform the crateslot where the modules are.

     Int_t  getModel( UInt_t crate, UInt_t slot ) const; // Return module type
     UInt_t getHeader( UInt_t crate, UInt_t slot ) const;// Return header
     UInt_t getMask( UInt_t crate, UInt_t slot ) const;  // Return header mask
     Int_t  getBank( UInt_t crate, UInt_t slot ) const;  // Return bank number
     UInt_t getScalerCrate( UInt_t word) const;          // Return scaler crate if word=header
     const char* getScalerLoc( UInt_t crate ) const;     // Return scaler crate location
     const char* getConfigStr( UInt_t crate, UInt_t slot ) const; // Configuration string
     UInt_t getNchan( UInt_t crate, UInt_t slot ) const; // Max number of channels
     UInt_t getNdata( UInt_t crate, UInt_t slot ) const; // Max number of data words
     bool crateUsed( UInt_t crate ) const;               // True if crate is used
     bool slotUsed( UInt_t crate, UInt_t slot ) const;   // True if slot in crate is used
     bool slotClear( UInt_t crate, UInt_t slot ) const;  // Decide if not clear ea event
     void setUnused( UInt_t crate, UInt_t slot );   // Disables this slot in crate
     void setUnused( UInt_t crate );                // Disables this crate
     int  init(const std::string& the_map);         // Initialize from text-block
     int  init(ULong64_t time = 0);                 // Initialize by Unix time.
     int  init( FILE* fi, const char* fname );      // Initialize from given file
     void print(std::ostream& os = std::cout) const;

     const std::vector<UInt_t>& GetUsedCrates() const;
     const std::vector<UInt_t>& GetUsedSlots( UInt_t crate ) const;

     static const Int_t CM_OK;
     static const Int_t CM_ERR;

     static const UInt_t DEFAULT_TSROC;

     const char* GetName() const { return fDBfileName.c_str(); }

 private:

     enum ECrateCode { kUnknown, kFastbus, kVME, kScaler, kCamac };

     std::string fDBfileName;     // Database file name
     TDatime     fInitTime;       // Database time stamp
     UInt_t      fTSROC;          // Crate (aka ROC) of the trigger supervisor

     class SlotInfo_t {
     public:
       SlotInfo_t() :
         model(0), header(0), headmask(0xffffffff), bank(-1),
         nchan(0), ndata(0), used(false), clear(true) {}
       Int_t  model;
       UInt_t header;
       UInt_t headmask;
       Int_t  bank;
       UInt_t nchan;
       UInt_t ndata;
       std::string cfgstr;
       bool   used;
       bool   clear;
     };

     class CrateInfo_t {            // Crate Information data descriptor
     public:
       CrateInfo_t();
       Int_t ParseSlotInfo( THaCrateMap* crmap, UInt_t crate, std::string& line );
       ECrateCode  crate_code;
       std::string crate_type_name;
       std::string scalerloc;
       bool crate_used;
       bool bank_structure;
       bool all_banks;
       std::vector<UInt_t> used_slots;
       std::array<SlotInfo_t, MAXSLOT> sltdat;
     };
     std::vector<CrateInfo_t> crdat;

     std::vector<UInt_t> used_crates;

     Int_t  loadConfig( std::string& line, std::string& cfgstr );
     Int_t  resetCrate( UInt_t crate );
     Int_t  setCrateType( UInt_t crate, const char* stype ); // set the crate type
     Int_t  setModel( UInt_t crate, UInt_t slot, Int_t mod,
                      UInt_t nchan= MAXCHAN,
                      UInt_t ndata= MAXDATA );           // set the module type
     void   setUsed( UInt_t crate, UInt_t slot );
     Int_t  SetModelSize( UInt_t crate, UInt_t slot, UInt_t model );
     Int_t  ParseCrateInfo( const std::string& line, UInt_t& crate );
     Int_t  SetBankInfo();

     static Int_t readFile( FILE* fi, std::string& text );

     ClassDef(THaCrateMap,0) // Map of modules in DAQ crates
};

//=============== inline functions ================================
inline
bool THaCrateMap::isFastBus( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return (crdat[crate].crate_code == kFastbus);
}

inline
bool THaCrateMap::isVme( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return  (crdat[crate].crate_code == kVME ||
	   crdat[crate].crate_code == kScaler );
}

inline
bool THaCrateMap::isCamac( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return (crdat[crate].crate_code == kCamac);
}

inline
bool THaCrateMap::isScalerCrate( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return (crdat[crate].crate_code == kScaler);
}

inline
bool THaCrateMap::isBankStructure( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return (crdat[crate].bank_structure);
}

inline
bool THaCrateMap::isAllBanks( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return (crdat[crate].all_banks);
}

inline
bool THaCrateMap::crateUsed( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return crdat[crate].crate_used;
}

inline
bool THaCrateMap::slotUsed( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  if( crate >= crdat.size() || slot >= crdat[crate].sltdat.size() )
    return false;
  return crdat[crate].sltdat[slot].used;
}

inline
bool THaCrateMap::slotClear( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].clear;
}

inline
Int_t THaCrateMap::getModel( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].model;
}

inline
UInt_t THaCrateMap::getMask( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].headmask;
}

inline
Int_t THaCrateMap::getBank( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].bank;
}

inline
UInt_t THaCrateMap::getNchan( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].nchan;
}

inline
UInt_t THaCrateMap::getNdata( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].ndata;
}

inline
UInt_t THaCrateMap::getNslot( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return crdat[crate].used_slots.size();
}

inline
const char* THaCrateMap::getScalerLoc( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return crdat[crate].scalerloc.c_str();
}

inline
const char* THaCrateMap::getConfigStr( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].cfgstr.c_str();
}

inline
UInt_t THaCrateMap::getMinSlot( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  if( crdat[crate].used_slots.empty() )
    return kMaxUInt;
  return crdat[crate].used_slots.front();
}

inline
UInt_t THaCrateMap::getMaxSlot( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  if( crdat[crate].used_slots.empty() )
    return 0;
  return crdat[crate].used_slots.back();
}

inline
UInt_t THaCrateMap::getTSROC() const
{
  return fTSROC;
}

inline
UInt_t THaCrateMap::getHeader( UInt_t crate, UInt_t slot ) const
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  return crdat[crate].sltdat[slot].header;
}

inline
const std::vector<UInt_t>& THaCrateMap::GetUsedCrates() const
{
  return used_crates;
}

inline
const std::vector<UInt_t>& THaCrateMap::GetUsedSlots( UInt_t crate ) const
{
  assert( crate < crdat.size() );
  return crdat[crate].used_slots;
}

}

#endif
