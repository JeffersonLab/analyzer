#ifndef THaCodaData_h
#define THaCodaData_h

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
#include <cstdio>

// Return cods from codaNNN routines
#define CODA_OK     0      // OK
#define CODA_EOF    EOF    // End of file
#define CODA_ERROR  -128   // Generic error return code
#define CODA_FATAL  -255   // Fatal error

#define MAXEVLEN 100005   // Maximum size of events
#define CODA_VERBOSE 1    // Errors explained verbosely (recommended)
#define CODA_DEBUG  0     // Lots of printout (recommend to set = 0)

namespace Decoder {

class THaCodaData {

public:

   THaCodaData();
   virtual ~THaCodaData();
   virtual Int_t codaOpen(const char* file_name, Int_t mode=1)=0;
   virtual Int_t codaOpen(const char* file_name, const char* session, Int_t mode=1)=0;
   virtual Int_t codaClose()=0;
   virtual Int_t codaRead()=0;
   virtual UInt_t* getEvBuffer() { return evbuffer; }
   virtual Int_t getBuffSize() const { return MAXEVLEN; }
   virtual Bool_t isOpen() const = 0;

protected:
   static Int_t ReturnCode( Long64_t evio_retcode );

private:

   THaCodaData(const THaCodaData &fn);
   THaCodaData& operator=(const THaCodaData &fn);

protected:

   TString  filename;
   UInt_t*  evbuffer;     // Raw data

   ClassDef(THaCodaData,0) // Base class of CODA data (file, ET conn, etc)

};

}

#endif
