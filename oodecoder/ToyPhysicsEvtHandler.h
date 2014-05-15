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
#include "TNamed.h"

class ToyEvtTypeHandler;
class THaEvData;

using THaString::CmpNoCase;

class FlagData : public TNamed {

 public: FlagData(const char *name, const char *descr, Int_t hdr, Int_t msk, Int_t offs, Int_t siz) : TNamed(name,descr),  header(hdr), offset(offs), size(siz) {
    isLoaded = kFALSE;
    if (size <= 0) size=1;
    fData = new Float_t[size];
  }
  ~FlagData() { 
    if (size>0) delete []fData;
  }
  void LoadFlag(Int_t *rdat, Int_t ptr) {
    if ((rdat[ptr]&mask)==header) {
      for (Int_t i=0; i<size; i++) {
	fData[i]=rdat[offset+ptr++];
      }
    }
    isLoaded=kTRUE;
  }
  Bool_t IsLoaded() { return isLoaded; };
  void Clear() { isLoaded=kFALSE; }    
  // I think there is a GetName for TNamed -- check
  Float_t GetData(Int_t i=0) { 
    if (i >= 0 &&  i < size) return fData[i];
    return 0;
  }
 private:
  Int_t header, mask, offset, size;
  Bool_t toOutput, isLoaded;
  Float_t *fData;

};


class ToyPhysicsEvtHandler : public ToyEvtTypeHandler {

public:

   ToyPhysicsEvtHandler();
   virtual ~ToyPhysicsEvtHandler();  

   Int_t Decode(THaEvData *evdata);
   Int_t Init(THaCrateMap *map);

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
   vector <FlagData *> flagdata;
   Int_t ProcFlags(THaEvData*, Int_t loc1);


   ClassDef(ToyPhysicsEvtHandler,0)  // Physics Event handler

};

#endif
