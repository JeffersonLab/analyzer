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


#include "TString.h"
#include "Decoder.h"
#include <fstream>
#include <cstdio>  // for FILE
#include <cassert>
#include <iostream>

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
     UInt_t getNslot( UInt_t crate ) const;         // Returns num occupied slots
     UInt_t getMinSlot( UInt_t crate ) const;       // Returns min slot number
     UInt_t getMaxSlot( UInt_t crate ) const;       // Returns max slot number

 // This class must inform the crateslot where the modules are.

     Int_t  getModel( UInt_t crate, UInt_t slot ) const;       // Return module type
     UInt_t getHeader( UInt_t crate, UInt_t slot ) const;      // Return header
     UInt_t getMask( UInt_t crate, UInt_t slot ) const;        // Return header mask
     Int_t  getBank( UInt_t crate, UInt_t slot ) const;        // Return bank number
     UInt_t getScalerCrate( UInt_t word) const;            // Return scaler crate if word=header
     const char* getScalerLoc( UInt_t crate ) const;     // Return scaler crate location
     Int_t  setCrateType( UInt_t crate, const char* stype ); // set the crate type
     Int_t  setModel( UInt_t crate, UInt_t slot, Int_t mod,
                      UInt_t nchan= MAXCHAN,
                      UInt_t ndata= MAXDATA );            // set the module type
     Int_t  setHeader( UInt_t crate, UInt_t slot, UInt_t head );  // set the header
     Int_t  setMask( UInt_t crate, UInt_t slot, UInt_t mask );    // set the header mask
     Int_t  setBank( UInt_t crate, UInt_t slot, Int_t bank );    // set the bank
     Int_t  setScalerLoc( UInt_t crate, const char* location ); // Sets the scaler location
     UInt_t getNchan( UInt_t crate, UInt_t slot ) const;  // Max number of channels
     UInt_t getNdata( UInt_t crate, UInt_t slot ) const;  // Max number of data words
     bool slotDone( UInt_t slot ) const;                       // Used to speed up decoder
     bool crateUsed( UInt_t crate ) const;               // True if crate is used
     bool slotUsed( UInt_t crate, UInt_t slot ) const;      // True if slot in crate is used
     bool slotClear( UInt_t crate, UInt_t slot ) const;     // Decide if not clear ea event
     void setSlotDone( UInt_t slot );                    // Used to speed up decoder
     void setSlotDone();                            // Used to speed up decoder
     void setUnused( UInt_t crate, UInt_t slot );            // Disables this crate,slot
     int init(TString the_map);                     // Initialize from text-block
     int init(ULong64_t time = 0);                  // Initialize by Unix time.
     int init( FILE* fi, const TString& fname );    // Initialize from given file
     void print(std::ostream& os = std::cout) const;

     static const Int_t CM_OK;
     static const Int_t CM_ERR;

     const char* GetName() const { return fDBfileName.Data(); }

 private:

     enum ECrateCode { kUnknown, kFastbus, kVME, kScaler, kCamac };

     TString fDBfileName;             // Database file name
     struct CrateInfo_t {           // Crate Information data descriptor
       TString crate_type;
       bool bank_structure;
       ECrateCode crate_code;
       UInt_t nslot, minslot, maxslot;
       bool crate_used;
       bool slot_used[MAXSLOT], slot_clear[MAXSLOT];
       Int_t model[MAXSLOT];
       UInt_t header[MAXSLOT], headmask[MAXSLOT];
       Int_t bank[MAXSLOT];
       UInt_t nchan[MAXSLOT], ndata[MAXSLOT];
       TString scalerloc;
     } crdat[MAXROC];

     bool didslot[MAXSLOT];

     void  incrNslot( UInt_t crate );
     void  setUsed( UInt_t crate, UInt_t slot );
     void  setClear( UInt_t crate, UInt_t slot, bool clear);
     Int_t SetModelSize( UInt_t crate, UInt_t slot, UInt_t model );

     ClassDef(THaCrateMap,0) // Map of modules in DAQ crates
};

//=============== inline functions ================================
inline
bool THaCrateMap::isFastBus( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return (crdat[crate].crate_code == kFastbus);
}

inline
bool THaCrateMap::isVme( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return  (crdat[crate].crate_code == kVME ||
	   crdat[crate].crate_code == kScaler );
}

inline
bool THaCrateMap::isCamac( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return (crdat[crate].crate_code == kCamac);
}

inline
bool THaCrateMap::isScalerCrate( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return (crdat[crate].crate_code == kScaler);
}

inline
bool THaCrateMap::isBankStructure( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return (crdat[crate].bank_structure);
}

inline
bool THaCrateMap::crateUsed( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return crdat[crate].crate_used;
}

inline
bool THaCrateMap::slotUsed( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  if( crate >= MAXROC || slot >= MAXSLOT )
    return false;
  return crdat[crate].slot_used[slot];
}

inline
bool THaCrateMap::slotClear( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].slot_clear[slot];
}

inline
Int_t THaCrateMap::getModel( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].model[slot];
}

inline
UInt_t THaCrateMap::getMask( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].headmask[slot];
}

inline
Int_t THaCrateMap::getBank( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].bank[slot];
}

inline
UInt_t THaCrateMap::getNchan( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].nchan[slot];
}

inline
UInt_t THaCrateMap::getNdata( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].ndata[slot];
}

inline
UInt_t THaCrateMap::getNslot( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return crdat[crate].nslot;
}

inline
const char* THaCrateMap::getScalerLoc( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return crdat[crate].scalerloc.Data();
}

inline
UInt_t THaCrateMap::getMinSlot( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return crdat[crate].minslot;
}

inline
UInt_t THaCrateMap::getMaxSlot( UInt_t crate ) const
{
  assert( crate < MAXROC );
  return crdat[crate].maxslot;
}

inline
UInt_t THaCrateMap::getHeader( UInt_t crate, UInt_t slot ) const
{
  assert( crate < MAXROC && slot < MAXSLOT );
  return crdat[crate].header[slot];
}

inline
void THaCrateMap::setUsed( UInt_t crate, UInt_t slot )
{
  assert( crate < MAXROC && slot < MAXSLOT );
  crdat[crate].crate_used = true;
  crdat[crate].slot_used[slot] = true;
}

inline
void THaCrateMap::setUnused( UInt_t crate, UInt_t slot )
{
  assert( crate < MAXROC && slot < MAXSLOT );
  crdat[crate].slot_used[slot] = false;
}

inline
void THaCrateMap::setClear( UInt_t crate, UInt_t slot, bool clear)
{
  assert( crate < MAXROC && slot < MAXSLOT );
  crdat[crate].slot_clear[slot] = clear;
}

inline
bool THaCrateMap::slotDone( UInt_t slot ) const
{
  assert( slot < MAXSLOT );
  return didslot[slot];
}

inline
void THaCrateMap::setSlotDone( UInt_t slot )
{
  assert( slot < MAXSLOT );
  didslot[slot] = true;
}

inline
void THaCrateMap::setSlotDone()
{
  // initialize
  for( bool& slotflag : didslot ) {
    slotflag = false;
  }
}

}

#endif
