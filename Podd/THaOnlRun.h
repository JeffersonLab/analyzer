#ifndef Podd_THaOnlRun_h_
#define Podd_THaOnlRun_h_

//////////////////////////////////////////////////////////////////////////
//
// THaOnlRun
//
// Description of an online run using ET system. 
//
//////////////////////////////////////////////////////////////////////////

#include "THaCodaRun.h"
#include "TString.h"

class THaOnlRun : public THaCodaRun {
  
public:
  THaOnlRun();
  THaOnlRun(const char* computer, const char* session, Int_t mode);
  THaOnlRun( const THaOnlRun& rhs );
  virtual THaOnlRun& operator=( const THaRunBase& rhs );

  virtual  Int_t  Open();
  virtual  Int_t  OpenConnection( const char* computer, const char* session, 
				  Int_t mode);
  
protected:
  TString  fComputer;   // computer where DAQ is running, e.g. 'adaql2'
  TString  fSession;    // SESSION = unique ID of DAQ, usually an env. var., 
                        // e.g 'onla'
  Int_t    fMode;       // mode (0=wait forever for data, 1=time out, recommend 1)

  ClassDef(THaOnlRun,2)  // A connection to CODA data via the ET system
};

#endif
