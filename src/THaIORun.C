
#include "THaIORun.h"
#include "THaCodaData.h"
#include "THaCodaFile.h"

//__________________________________________________________________________________
THaIORun::THaIORun( const char* filename, const char* description, const char* mode)
  : THaRun(filename,description), fIOmode(mode) { }

//__________________________________________________________________________________
Int_t THaIORun::OpenFile() {
  // Open CODA file for access set

  if( fFilename.IsNull() )
    return -2;  // filename not set

  return fCodaData ? fCodaData->codaOpen( fFilename, fIOmode, 1 ) : -3;
}

//__________________________________________________________________________________
Int_t THaIORun::OpenFile( const char* filename ) {
  SetFilename( filename );
  return OpenFile();
}

//__________________________________________________________________________________
Int_t THaIORun::OpenFile( const char* filename, const char* mode ) {
  SetFilename( filename );
  SetMode( mode );
  return OpenFile();
}

//__________________________________________________________________________________
Int_t THaIORun::PutEvent( const Int_t* buffer ) {
  THaCodaFile* cf = dynamic_cast<THaCodaFile*>(fCodaData);
  // need to case away const, since codaWrite 
  return cf ? cf->codaWrite( const_cast<Int_t*>(buffer) ) : -255;
}

//__________________________________________________________________________________
void THaIORun::SetMode( const char* mode ) {
  // Set read/write mode. Should be 'r' or 'w'
  fIOmode = mode;
}

//__________________________________________________________________________________
THaIORun::~THaIORun() { }

ClassImp(THaIORun)
