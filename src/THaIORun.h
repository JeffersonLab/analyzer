#ifndef HALLA_THaIORun
#define HALLA_THaIORun

#include "THaRun.h"
#include "TString.h"

class THaIORun : public THaRun {
  // extend, to permit us to WRITE events
  // from the evbuffer to a file
 public:
  THaIORun( const char* filename="", const char* description="", const char* mode="r" );
  virtual ~THaIORun();
  
  virtual Int_t OpenFile();
  virtual Int_t OpenFile( const char* filename );
  virtual Int_t OpenFile( const char* filename, const char* mode );
  virtual void  SetMode( const char* mode );
  
  virtual Int_t PutEvent( const Int_t* buffer );
  
 protected:
  TString fIOmode;
 public:
  ClassDef(THaIORun,0)
};

#endif  /* HALLA_THaIORun */
