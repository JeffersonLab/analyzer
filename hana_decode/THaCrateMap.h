#ifndef THaCrateMap_
#define THaCrateMap_

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


#include "Rtypes.h"
#include "TString.h"

//#define CHECK_RANGE

class THaCrateMap
{


 public:
     static const UShort_t MAXCHAN;
     static const UShort_t MAXDATA;

     THaCrateMap() {}                               // Construct, but not initialized
     virtual ~THaCrateMap() {}
     bool isFastBus(int crate) const;               // True if fastbus crate;
     bool isVme(int crate) const;                   // True if VME crate;
     bool isCamac(int crate) const;                 // True if CAMAC crate;
     bool isScalerCrate(int crate) const;           // True if a Scaler crate;
     int getModel(int crate, int slot) const;       // Return module type
     int getHeader(int crate, int slot) const;      // Return header
     int getMask(int crate, int slot) const;        // Return header mask
     int getScalerCrate(int word) const;            // Return scaler crate if word=header
     const char* getScalerLoc(int crate) const;     // Return scaler crate location 
     int setCrateType(int crate, const char* type); // set the crate type
     int setModel(int crate, int slot, UShort_t mod,
		  UShort_t nchan=MAXCHAN, 
		  UShort_t ndata=MAXDATA);          // set the module type
     int setHeader(int crate, int slot, int head);  // set the header
     int setMask(int crate, int slot, int mask);    // set the header mask
     int setScalerLoc(int crate, const char* location); // Sets the scaler location
     int getNslot(int crate) const;                 // Returns num occupied slots
     UShort_t getNchan(int crate, int slot) const;  // Max number of channels
     UShort_t getNdata(int crate, int slot) const;  // Max number of data words
     bool slotDone(int slot) const;                       // Used to speed up decoder
     bool crateUsed(int crate) const;               // True if crate is used
     bool slotUsed(int crate, int slot) const;      // True if slot in crate is used
     bool slotClear(int crate, int slot) const;     // Decide if not clear ea event
     void setSlotDone(int slot);                    // Used to speed up decoder
     void setSlotDone();                            // Used to speed up decoder
     int init(TString the_map);                     // Initialize from text-block
     int init(UInt_t time);                         // Initialize by Unix time.
     int init();                                    // Let me initialize everything 
                                                    // (recommend to call this once)
     int init_hc(UInt_t time);                      // Hard-coded crate-map
     void print() const;

     static const int CM_OK;
     static const int CM_ERR;

 private:

     enum ECrateCode { kUnknown, kFastbus, kVME, kScaler, kCamac };

     static const int MAXROC = 20;
     static const int MAXSLOT = 27;
     TString crate_type[MAXROC];
     ECrateCode crate_code[MAXROC];
     int nslot[MAXROC];               // Number of slots used
     bool didslot[MAXSLOT];
     bool crate_used[MAXROC];
     bool slot_used[MAXROC][MAXSLOT];
     bool slot_clear[MAXROC][MAXSLOT];
     UShort_t model[MAXROC][MAXSLOT]; // Model number
     int header[MAXROC][MAXSLOT];     // Header signature
     int headmask[MAXROC][MAXSLOT];   // Mask for header signature bits
     UShort_t nchan[MAXROC][MAXSLOT]; // Number of channels for device
     UShort_t ndata[MAXROC][MAXSLOT]; // Number of datawords
     TString scalerloc[MAXROC];
     void incrNslot(int crate);
     void setUsed(int crate,int slot);
     void setClear(int crate,int slot,bool clear);
     int  SetModelSize(int crate, int slot, UShort_t model );

     ClassDef(THaCrateMap,0) // Map of modules in DAQ crates
};     

//=============== inline functions ================================
inline
bool THaCrateMap::isFastBus(int crate) const {
#ifdef CHECK_RANGE
  if ( crate < 0 || crate >= MAXROC ) 
    return false;
#endif
  return (crate_code[crate] == kFastbus);
}

inline
bool THaCrateMap::isVme(int crate) const {
#ifdef CHECK_RANGE
  if ( crate < 0 || crate >= MAXROC ) 
    return false;
#endif
  return  (crate_code[crate] == kVME ||
	   crate_code[crate] == kScaler );
}

inline
bool THaCrateMap::isCamac(int crate) const {
#ifdef CHECK_RANGE
  if ( crate < 0 || crate >= MAXROC ) 
    return false;
#endif
  return (crate_code[crate] == kCamac);
}

inline
bool THaCrateMap::isScalerCrate(int crate) const {
#ifdef CHECK_RANGE
  if ( crate < 0 || crate >= MAXROC ) 
    return false;
#endif
  return (crate_code[crate] == kScaler);
}

inline
bool THaCrateMap::crateUsed(int crate) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC) 
    return false;
#endif
  return crate_used[crate];
}

inline
bool THaCrateMap::slotUsed(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return false;
#endif
  return slot_used[crate][slot];
}

inline
bool THaCrateMap::slotClear(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return true;
#endif
  return slot_clear[crate][slot];
}

inline
int THaCrateMap::getModel(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return CM_ERR;
#endif
  return model[crate][slot];
}

inline
int THaCrateMap::getMask(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return CM_ERR;
#endif
  return headmask[crate][slot];
}

inline
UShort_t THaCrateMap::getNchan(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return CM_ERR;
#endif
  return nchan[crate][slot];
}

inline
UShort_t THaCrateMap::getNdata(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return CM_ERR;
#endif
  return ndata[crate][slot];
}

inline
int THaCrateMap::getNslot(int crate) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
#endif
  return nslot[crate];
}

inline
const char* THaCrateMap::getScalerLoc(int crate) const {
#ifdef CHECK_RANGE
  if ( crate < 0 || crate >= MAXROC ) 
    return "unknown";
#endif
  return scalerloc[crate].Data();
}

inline
int THaCrateMap::getHeader(int crate, int slot) const {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return CM_ERR;
#endif
  return header[crate][slot];
}

inline
void THaCrateMap::setUsed(int crate, int slot) {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ) 
    return;
#endif
  crate_used[crate] = true;
#ifdef CHECK_RANGE
  if (slot < 0 || slot >= MAXSLOT ) 
    return;
#endif
  slot_used[crate][slot] = true;
}

inline
void THaCrateMap::setClear(int crate, int slot, bool clear) {
#ifdef CHECK_RANGE
  if (crate < 0 || crate >= MAXROC ||
      slot < 0  || slot  >= MAXSLOT )
    return;
#endif
  slot_clear[crate][slot] = clear;
}

inline
bool THaCrateMap::slotDone(int slot) const {
#ifdef CHECK_RANGE
  if (slot < 0 || slot >= MAXSLOT ) 
    return false;
#endif
  return didslot[slot];
}

inline
void THaCrateMap::setSlotDone(int slot) {
#ifdef CHECK_RANGE
  if (slot < 0 || slot >= MAXSLOT ) 
    return;
#endif
  didslot[slot] = true;
}

inline
void THaCrateMap::setSlotDone() {        // initialize
  for (int i=0; i<MAXSLOT; i++)
    didslot[i] = false;
}

inline
int THaCrateMap::init() {
  return init(0);
}

#endif
