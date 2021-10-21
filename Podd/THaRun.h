#ifndef Podd_THaRun_h_
#define Podd_THaRun_h_

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "THaCodaRun.h"
#include "TString.h"
#include <vector>

class THaRun : public THaCodaRun {

public:
  explicit THaRun( const char* filename="", const char* description="" );
  THaRun( const THaRun& run );
  THaRun( const std::vector<TString>& pathList, const char* filename,
	  const char* description="" );
  virtual THaRun& operator=( const THaRunBase& rhs );
  virtual ~THaRun();

  virtual void         Clear( Option_t* opt="" );
  virtual Int_t        Compare( const TObject* obj ) const;
          const char*  GetFilename() const { return fFilename.Data(); }
          Int_t        GetSegment()  const { return fSegment; }
          Int_t        GetStream()   const { return fStream; }
  virtual Int_t        Open();
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        SetFilename( const char* name );
          void         SetNscan( UInt_t n );

protected:

  TString  fFilename;  // File name
  UInt_t   fMaxScan;   // Max. no. of events to prescan (0=don't scan)
  Int_t    fSegment;   // Segment number (for split runs). -1: unset
  Int_t    fStream;    // Event stream number (for parallel streams). -1: unset

  virtual Bool_t   FindSegmentNumber();
  virtual Int_t    PrescanFile();
  virtual Bool_t   ProvidesInitInfo();
  virtual Int_t    ReadInitInfo( Int_t level );
  virtual TString  GetInitInfoFileName( TString fname );
  virtual TString  FindInitInfoFile( const TString& fname );

  ClassDef(THaRun,7)  // A run based on a CODA data file on disk
};


#endif
