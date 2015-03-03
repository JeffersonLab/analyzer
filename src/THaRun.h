#ifndef ROOT_THaRun
#define ROOT_THaRun

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaCodaRun.h"
#include "Decoder.h"

class THaRun : public THaCodaRun {

public:
  THaRun( const char* filename="", const char* description="" );
  THaRun( const THaRun& run );
  virtual THaRun& operator=( const THaRunBase& rhs );
  virtual ~THaRun();

  virtual void         Clear( Option_t* opt="" );
  virtual Int_t        Compare( const TObject* obj ) const;
          const char*  GetFilename() const { return fFilename.Data(); }
          Int_t        GetSegment()  const { return fSegment; }
  virtual Int_t        Open();
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        SetFilename( const char* name );
          void         SetNscan( UInt_t n );

protected:

  TString       fFilename;     //  File name
  UInt_t        fMaxScan;      //  Max. no. of events to prescan (0=don't scan)
  Int_t         fSegment;      //  Segment number (for split runs)

          Int_t FindSegmentNumber();
  virtual Int_t ReadInitInfo();

  ClassDef(THaRun,6)           // A run based on a CODA data file on disk
};


#endif
