#ifndef ROOT_THaOnlRun
#define ROOT_THaOnlRun

//////////////////////////////////////////////////////////////////////////
//
// THaOnlRun
//
// Description of an online run using ET system. 
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TDatime.h"

#include "THaRun.h"

class THaCodaData;

class THaOnlRun : public THaRun {
  
protected:
  TString  fComputer;   // computer where DAQ is running, e.g. 'adaql2'
  TString  fSession;    // SESSION = unique ID of DAQ, usually an env. var., e.g 'onla'
  UInt_t   fMode;       // mode (0=wait forever for data, 1=time out, recommend 1)
  UInt_t   fOpened;     // status

public:
  THaOnlRun();
  THaOnlRun(const char* computer, const char* session, UInt_t mode);
  THaOnlRun& operator=( const THaOnlRun& rhs );
  THaOnlRun( const THaOnlRun& rhs );

  virtual  Int_t  OpenFile();
           Int_t  OpenFile( const char* computer, const char* session, UInt_t mode);
  
  ClassDef(THaOnlRun,0)   //Description of an online run using ET system
};

#endif
