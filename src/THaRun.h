#ifndef ROOT_THaRun
#define ROOT_THaRun

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaCodaRun.h"
#include "TDatime.h"

class THaRun : public THaCodaRun {
  
public:
  THaRun( const char* filename="", const char* description="" );
  THaRun( const THaRun& run );
  virtual THaRun& operator=( const THaRunBase& rhs );
  virtual ~THaRun();
  
  virtual void         Clear( Option_t* opt="" );
          const char*  GetFilename() const { return fFilename.Data(); }
  virtual Int_t        Open();
  virtual void         Print( Option_t* opt="" ) const;
  virtual void         SetFilename( const char* name );
          void         SetNscan( UInt_t n );

protected:

  TString       fFilename;     //  File name
  UInt_t        fMaxScan;      //  Max. no. of events to prescan (0=don't scan)

  virtual Int_t ReadInitInfo();

  ClassDef(THaRun,5)           // A run based on a CODA data file on disk
};


#endif
