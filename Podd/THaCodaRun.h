#ifndef Podd_THaCodaRun_h_
#define Podd_THaCodaRun_h_

//////////////////////////////////////////////////////////////////////////
//
// THaCodaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunBase.h"
#include "Decoder.h"

class THaCodaRun : public THaRunBase {
  
public:
  THaCodaRun( const char* description="" );
  THaCodaRun( const THaCodaRun& rhs );
  virtual THaCodaRun& operator=( const THaRunBase& );
  virtual ~THaCodaRun();
  
  virtual Int_t  Close();
  virtual const UInt_t* GetEvBuffer() const;
  virtual Bool_t IsOpen() const;
  virtual Int_t  ReadEvent();
  virtual Int_t  GetDataVersion() { return GetCodaVersion(); }
  virtual Int_t  SetDataVersion( Int_t version ) { return SetCodaVersion(version); }
  Int_t GetCodaVersion();
  Int_t SetCodaVersion( Int_t version );

protected:
  static Int_t ReturnCode( Int_t coda_retcode);

  Decoder::THaCodaData*  fCodaData;  //! CODA data associated with this run

  ClassDef(THaCodaRun,1)    // ABC for a run based on CODA data
};


#endif
