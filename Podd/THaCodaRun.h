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
  THaCodaRun& operator=( const THaRunBase& ) override;
  ~THaCodaRun() override;

  Int_t          Close() override;
  const UInt_t*  GetEvBuffer() const override;
  Bool_t         IsOpen() const override;
  Int_t          ReadEvent() override;
  Int_t          SetDataVersion( Int_t version ) override;
  Int_t          GetCodaVersion();
  Int_t          SetCodaVersion( Int_t version );

protected:
  static Int_t ReturnCode( Int_t coda_retcode);

  std::unique_ptr<Decoder::THaCodaData> fCodaData;  //! CODA data associated with this run

  ClassDefOverride(THaCodaRun,2)    // ABC for a run based on CODA data
};

//________________________ inlines ____________________________________________
inline Int_t THaCodaRun::GetCodaVersion()
{
  return GetDataVersion();
}

inline Int_t THaCodaRun::SetCodaVersion( Int_t version )
{
  return SetDataVersion(version);
}

#endif
