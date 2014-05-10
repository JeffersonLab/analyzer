#ifndef ToyPhysicsEvtHandler_
#define ToyPhysicsEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   NEW STUFF (the entire class)
//   ToyPhysicsEvtHandler
//   Abstract class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "THaString.h"

class ToyEvtTypeHandler;
class THaEvData;

using THaString::CmpNoCase;

class DecoderRules {
  // Utility class used by PhysicsHandler to store decoder rules
 public:
 DecoderRules(string ctype) : {
    isFastbus=0;
    isVme=0;
    isFlag=1;
    if (CmpNoCase("ctype","fastbus")==0) isFastbus=1; 
    if (CmpNoCase("ctype","vme")==0) isVme=1;
    if (CmpNoCase("ctype","flag")==0) isFlag=1;
  }
  Int_t GetHeader(Int_t index) {
    if (index >= 0 && index < headers.size()) return headers[index];
    return -1;
  }
  Int_t GetMask(Int_t index) {
    if (index >= 0 && index < masks.size()) return masks[index];
    return -1;
  }
  Int_t GetOffset(Int_t index) {
    if (index >= 0 && index < offsets.size()) return offsets[index];
    return -1;
  }
  Int_t GetSlot(Int_t rdat) {
    if (isFastbus) return (rdat>>fbslotshift);
    for (Int_t i = 0 ; i < headers.size(); i++) {
      if ((rdat & masks[i])==headers[i]) return slots[i];
    }
    return -1;
  }
  Int_t LoadSlotInfo(Int_t header, Int_t mask, Int_t off=0,  Int_t slot=-1) {
     headers.push_back(header);
     masks.push_back(mask);
     slots.push_back(slot);
     offsets.push_back(slot);
    return 0;
  }
private:
  Int_t slot, isFasbus, isVme, isFlag;
  static const Int_t fbslotshift=27;
  vector <Int_t> headers, masks, offsets, slots;
}

class ToyPhysicsEvtHandler : public ToyEvtTypeHandler {

public:

   ToyPhysicsEvtHandler();
   virtual ~ToyPhysicsEvtHandler();  

   Int_t Decode(THaEvData *evdata);
   Int_t Init(THaCrateMap *map);
   Int_t SetCrateRule(Int_t i);

private:

   ToyPhysicsEvtHandler(const ToyPhysicsEvtHandler &fh);
   ToyPhysicsEvtHandler& operator=(const ToyPhysicsEvtHandler &fh);

   Int_t FindRocs(const Int_t *evbuffer);
   Int_t irn[MAXROC];
   struct RocDat_t {           // ROC raw data descriptor
        Int_t pos;             // position in evbuffer[]
        Int_t len;             // length of data
   } rocdat[MAXROC];

   Int_t *idxdr;
   vector <DecoderRules *> drules;
   Int_t ProcFlags(THaEvData*, Int_t loc1);


   ClassDef(ToyPhysicsEvtHandler,0)  // Physics Event handler

};

#endif
