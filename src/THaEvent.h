#ifndef ROOT_THaEvent
#define ROOT_THaEvent

//////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "Htypes.h"

class THaEvData;

class THaEventHeader {

public:
  THaEventHeader() : fEvtNum(0), fRun(0), fDate(0) { }
  virtual ~THaEventHeader() { }

  void   Set(Int_t i, Int_t r, Int_t d) { fEvtNum = i; fRun = r; fDate = d; }
  Int_t  GetEvtNum() const  { return fEvtNum; }
  Int_t  GetRun()    const  { return fRun; }
  Int_t  GetDate()   const  { return fDate; }

private:
  Int_t   fEvtNum;
  Int_t   fRun;
  Int_t   fDate;

  ClassDef(THaEventHeader,1)  //THaEvent Header
};


class THaEvent : public TObject {

public:

  THaEvent();
  virtual ~THaEvent();

  void              SetHeader( Int_t i, Int_t run, Int_t date )
                                { fEvtHdr.Set( i, run, date ); }
  THaEventHeader*   GetHeader() { return &fEvtHdr; }

  virtual Int_t     Fill( THaEvData& evdata );

protected:

  THaEventHeader    fEvtHdr;     // Event header
  Int_t event_number, event_type, event_length, run_number;
  Int_t helicity;

private:

  ClassDef(THaEvent,2)  // Output event data.
};


#endif


