#ifndef ToyPhysicsEvtHandler_
#define ToyPhysicsEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   NEW STUFF (the entire class)
//   ToyPhysicsEvtHandler
//   Class to handle physics triggers, which contain ROCs
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "DecoderGlobals.h"
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
  void ClearLoad() { isLoaded=kFALSE; }    
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

   ToyPhysicsEvtHandler(const char*, const char*);
   virtual ~ToyPhysicsEvtHandler();  

   Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time );

private:

   ToyPhysicsEvtHandler(const ToyPhysicsEvtHandler &fh);
   ToyPhysicsEvtHandler& operator=(const ToyPhysicsEvtHandler &fh);

   Int_t FindRocs(const Int_t *evbuffer);
   Int_t *rocnum, *rocpos, *roclen;
   vector <FlagData *> flagdata;
   Int_t ProcFlags(THaEvData*, Int_t loc1);
   struct RocDat_t {           // ROC raw data descriptor
     Int_t pos;                // position in evbuffer[]
     Int_t len;                // length of data
   } rocdat[32];
 
   ClassDef(ToyPhysicsEvtHandler,0)  // Physics Event handler

};

#endif
