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

class THaCrateMap
{

 public:

     THaCrateMap();                        // Construct, but not initialized
     virtual ~THaCrateMap();
     bool isFastBus(int crate) const;               // True if fastbus crate;
     bool isVme(int crate) const;                   // True if VME crate;
     bool isCamac(int crate) const;                 // True if CAMAC crate;
     bool isScalerCrate(int crate) const;           // True if a Scaler crate;
     int getModel(int crate, int slot) const;       // Return module type
     int getHeader(int crate, int slot) const;      // Return header
     int getMask(int crate, int slot) const;        // Return header mask
     int getScalerCrate(int word) const;            // Return scaler crate if word=header
     TString getScalerLoc(int crate) const;         // Return scaler crate location 
     int setCrateType(int crate, TString type);     // set the crate type
     int setModel(int crate, int slot, int mod);    // set the module type
     int setHeader(int crate, int slot, int head);  // set the header
     int setMask(int crate, int slot, int mask);    // set the header mask
     int setScalerLoc(int crate, TString location); // Sets the scaler location
     int getNslot(int crate) const;                 // Returns num occupied slots
     bool slotDone(int slot) const;                       // Used to speed up decoder
     bool crateUsed(int crate) const;               // True if crate is used
     bool slotUsed(int crate, int slot) const;      // True if slot in crate is used
     bool slotClear(int crate, int slot) const;     // Decide if not clear ea event
     void setSlotDone(int slot);                    // Used to speed up decoder
     void setSlotDone();                            // Used to speed up decoder
     int init(UInt_t time);                         // Initialize by Unix time.
     int init();                                    // Let me initialize everything 
                                                    // (recommend to call this once)
     int CM_OK,CM_ERR;

 private:

     static const int MAXROC = 20;
     static const int MAXSLOT = 27;
     TString crate_type[MAXROC];
     int nslot[MAXROC];
     bool didslot[MAXSLOT];
     bool crate_used[MAXROC];
     bool slot_used[MAXROC][MAXSLOT];
     bool slot_clear[MAXROC][MAXSLOT];
     int model[MAXROC][MAXSLOT];
     int header[MAXROC][MAXSLOT];
     int headmask[MAXROC][MAXSLOT];     
     TString scalerloc[MAXROC];
     void incrNslot(int slot);
     void setUsed(int crate,int slot);
     void setClear(int crate,int slot,bool clear);

     ClassDef(THaCrateMap,0) // Map of modules in DAQ crates
     
};

#endif




