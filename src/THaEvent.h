#ifndef ROOT_THaEvent
#define ROOT_THaEvent

//////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaVar;

class THaEventHeader {

public:
  THaEventHeader() : 
    fEvtTime(0), fEvtNum(0), fEvtType(0), fEvtLen(0), fHelicity(0),
    fTargetPol(0), fRun(0) {}
  virtual ~THaEventHeader() {}

  void Set( UInt_t num, Int_t type, Int_t len, ULong64_t time,
	    Int_t hel, Int_t pol, Int_t run ) { 
    fEvtNum    = num; 
    fEvtType   = type;
    fEvtLen    = len;
    fEvtTime   = time;
    fHelicity  = hel;
    fTargetPol = pol;
    fRun       = run;
  }
  // Old Set() for backwards compatibility
  void Set( UInt_t num, Int_t type, Int_t len, Double_t time,
	    Int_t hel, Int_t run ) { 
    Set( num, type, len, static_cast<ULong64_t>( 1e6*time ),
	 hel, 0, run );
  }
  UInt_t    GetEvtNum()    const  { return fEvtNum; }
  Int_t     GetEvtType()   const  { return fEvtType; }
  Int_t     GetEvtLen()    const  { return fEvtLen; }
  ULong64_t GetEvtTime()   const  { return fEvtTime; }
  Int_t     GetHelicity()  const  { return fHelicity; }
  Int_t     GetTargetPol() const  { return fTargetPol; }
  Int_t     GetRun()       const  { return fRun; }

private:
  // The units of these data are entirely up to the experiment
  ULong64_t fEvtTime;         // Event time stamp
  UInt_t    fEvtNum;          // Event number
  Int_t     fEvtType;         // Event type
  Int_t     fEvtLen;          // Event length
  Int_t     fHelicity;        // Beam helicity
  Int_t     fTargetPol;       // Target polarization
  Int_t     fRun;             // Run number

  ClassDef(THaEventHeader,5)  // Header for analyzed event data in ROOT file
};


class THaEvent : public TObject {

public:
  THaEvent();
  virtual ~THaEvent();

  THaEventHeader*   GetHeader() { return &fEvtHdr; }

  virtual void      Clear( Option_t *opt = "" );
  virtual Int_t     Init();
  virtual Int_t     Fill();
  virtual void      Reset( Option_t *opt = "" );

protected:

  THaEventHeader    fEvtHdr;     // Event header

  Bool_t            fInit;       //! Fill() initialized

  struct DataMap {
    Int_t        ncopy;          //! Number of elements to copy
    const char*  name;           //! Global variable name
    void*        dest;           //! Address of corresponding member variable
    Int_t*       ncopyvar;       //! Variable holding value of ncopy (if ncopy=-1)
    THaVar*      pvar;           //! Pointer to global variable object
  };
  DataMap*       fDataMap;       //! Map of global variables to copy

  ClassDef(THaEvent,3)  //Base class for event structure definition
};


#endif

