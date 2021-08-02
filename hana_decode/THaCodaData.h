#ifndef Podd_THaCodaData_h_
#define Podd_THaCodaData_h_

/////////////////////////////////////////////////////////////////////
//
//   THaCodaData
//   Abstract class of CODA data.
//
//   THaCodaData is an abstract class of CODA data.  Derived
//   classes will be typically either a CODA file (a disk
//   file) or a connection to the ET system.  Public methods
//   allow to open (i.e. set up), read, write, etc.
//
//   author Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "Rtypes.h"
#include "TString.h"
#include "Decoder.h"
#include "CustomAlloc.h"
#include <cstdio>   // for EOF
#include <vector>
#include <cassert>

// Return codes from codaNNN routines
#define CODA_OK     0      // OK
#define CODA_EOF    EOF    // End of file
#define CODA_ERROR  -128   // Generic error return code
#define CODA_FATAL  -255   // Fatal error

namespace Decoder {

// Dynamically-sized event buffer
class EvtBuffer {
public:
  explicit EvtBuffer( UInt_t initial_size = 0 );

  void    recordSize();
  void    updateSize() {
    if( (fNevents % fUpdateInterval) == 0 &&
        fChanged && !fDidGrow && fSaved.size() == fMaxSaved )
      updateImpl();
  }
  Bool_t  grow( UInt_t newsize = 0 );
  UInt_t  operator[]( UInt_t i ) { assert(i < size()); return fBuffer[i]; }
  UInt_t* get()        { return fBuffer.data(); }
  UInt_t  size() const { return fBuffer.size(); }
  void    reset();

private:
  VectorUIntNI  fBuffer;             // Raw data buffer
  VectorUInt    fSaved;
  UInt_t        fMaxSaved;
  UInt_t        fNevents;
  UInt_t        fUpdateInterval;
  Bool_t        fChanged;
  Bool_t        fDidGrow;

  void updateImpl();
};

class THaCodaData {

public:

   THaCodaData();
   THaCodaData(const THaCodaData &fn) = delete;
   THaCodaData& operator=(const THaCodaData &fn) = delete;
   virtual ~THaCodaData() = default;
   virtual Int_t codaOpen(const char* file_name, Int_t mode=1) = 0;
   virtual Int_t codaOpen(const char* file_name, const char* session, Int_t mode=1) = 0;
   virtual Int_t codaClose()=0;
   virtual Int_t codaRead()=0;
   UInt_t*       getEvBuffer() { return evbuffer.get(); }
   UInt_t        getBuffSize() const { return evbuffer.size(); }
   virtual Bool_t isOpen() const = 0;
   virtual Int_t getCodaVersion();
   void          setVerbosity(int level) { verbose = level; }
   Bool_t        isGood() const { return fIsGood; }

protected:
   static Int_t ReturnCode( Int_t evio_retcode );
   void staterr(const char* tried_to, Int_t status) const;

   EvtBuffer     evbuffer;    // Dynamically-sized event buffer
   TString       filename;
   Int_t         handle;      // EVIO data handle
   Int_t         verbose;     // Message verbosity (0=quiet, 1=verbose, 2=debug)
   Bool_t        fIsGood;

   ClassDef(THaCodaData,0) // Base class of CODA data (file, ET conn, etc)

};

}

#endif
