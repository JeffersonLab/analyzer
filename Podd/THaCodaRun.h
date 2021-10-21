#ifndef Podd_THaCodaRun_h_
#define Podd_THaCodaRun_h_

//////////////////////////////////////////////////////////////////////////
//
// THaCodaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunBase.h"
#include "Decoder.h"
#include <memory>

class THaCodaRun : public THaRunBase {
  
public:
  explicit THaCodaRun( const char* description="" );
  THaCodaRun( const THaCodaRun& rhs );
  virtual THaCodaRun& operator=( const THaRunBase& );
  virtual ~THaCodaRun();
  
  virtual Int_t          Close();
  virtual const UInt_t*  GetEvBuffer() const;
  virtual Bool_t         IsOpen() const;
  virtual Int_t          ReadEvent();
  virtual Int_t          GetDataVersion();
  virtual Int_t          SetDataVersion( Int_t version );
  Int_t                  GetCodaVersion();
  Int_t                  SetCodaVersion( Int_t version );

protected:
  static Int_t ReturnCode( Int_t coda_retcode);

  std::unique_ptr<Decoder::THaCodaData> fCodaData;  //! CODA data associated with this run

  ClassDef(THaCodaRun,2)    // ABC for a run based on CODA data
};

//________________________ inlines ____________________________________________
inline Int_t THaCodaRun::GetDataVersion()
{
  return GetCodaVersion();
}

inline Int_t THaCodaRun::SetDataVersion( Int_t version )
{
  return SetCodaVersion(version);
}

#endif
