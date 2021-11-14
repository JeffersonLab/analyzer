#ifndef Podd_THaEvent_h_
#define Podd_THaEvent_h_

//////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include <vector>

class THaVar;

class THaEventHeader {

public:
  THaEventHeader() :
    fEvtTime(0), fEvtNum(0), fEvtType(0), fEvtLen(0), fHelicity(0),
    fTrigBits(0), fRun(0) {}
  virtual ~THaEventHeader() = default;

  void Set( UInt_t num, UInt_t type, UInt_t len, ULong64_t time,
            Int_t hel, UInt_t tbits, UInt_t run ) {
    fEvtNum    = num; 
    fEvtType   = type;
    fEvtLen    = len;
    fEvtTime   = time;
    fHelicity  = hel;
    fTrigBits  = tbits;
    fRun       = run;
  }
  UInt_t    GetEvtNum()    const  { return fEvtNum; }
  UInt_t    GetEvtType()   const  { return fEvtType; }
  UInt_t    GetEvtLen()    const  { return fEvtLen; }
  ULong64_t GetEvtTime()   const  { return fEvtTime; }
  Int_t     GetHelicity()  const  { return fHelicity; }
  UInt_t    GetTrigBits()  const  { return fTrigBits; }
  UInt_t    GetRun()       const  { return fRun; }

private:
  // The units of these data are entirely up to the experiment
  ULong64_t fEvtTime;         // Event time stamp
  UInt_t    fEvtNum;          // Event number
  UInt_t    fEvtType;         // Event type
  UInt_t    fEvtLen;          // Event length
  Int_t     fHelicity;        // Beam helicity
  UInt_t    fTrigBits;        // Trigger bitpattern
  UInt_t    fRun;             // Run number

  ClassDef(THaEventHeader,7)  // Header for analyzed event data in ROOT file
};


class THaEvent : public TObject {

public:
  THaEvent();
  virtual ~THaEvent() = default;

  THaEventHeader*   GetHeader() { return &fEvtHdr; }

  virtual void      Clear( Option_t *opt = "" );
  virtual Int_t     Init();
  virtual Int_t     Fill();
  virtual void      Reset( Option_t *opt = "" );

protected:

  THaEventHeader    fEvtHdr;     // Event header

  Bool_t            fInit;       //! Fill() initialized

  class DataMap {
  public:
    Int_t        ncopy;          //! Number of elements to copy
    const char*  name;           //! Global variable name
    void*        dest;           //! Address of corresponding member variable
    Int_t*       ncopyvar;       //! Variable holding value of ncopy (if ncopy=-1)
    THaVar*      pvar;           //! Pointer to global variable object
  };
  std::vector<DataMap> fDataMap; //! Map of global variables to copy

  ClassDef(THaEvent,3)  //Base class for event structure definition
};


#endif

