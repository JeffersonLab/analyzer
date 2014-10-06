#ifndef ROOT_THaCodaRun
#define ROOT_THaCodaRun

//////////////////////////////////////////////////////////////////////////
//
// THaCodaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunBase.h"

class THaCodaData;

class THaCodaRun : public THaRunBase {
  
public:
  THaCodaRun( const char* description="" );
  THaCodaRun( const THaCodaRun& rhs );
  virtual THaCodaRun& operator=( const THaRunBase& );
  virtual ~THaCodaRun();
  
  virtual Int_t        Close();
  virtual const UInt_t* GetEvBuffer() const;
  virtual Bool_t       IsOpen() const;
  virtual Int_t        ReadEvent();

protected:
  static Int_t ReturnCode( Int_t coda_retcode);

  THaCodaData*  fCodaData;  //! CODA data associated with this run

  ClassDef(THaCodaRun,1)    // ABC for a run based on CODA data
};


#endif
