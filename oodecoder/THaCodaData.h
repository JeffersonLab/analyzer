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
#define CODA_ERROR -128   // Generic error return code
#define CODA_OK  0        // Means return is ok.
#define MAXEVLEN 100005   // Maximum size of events
#define CODA_VERBOSE 1    // Errors explained verbosely (recommended)
#define CODA_DEBUG  0     // Lots of printout (recommend to set = 0)

class THaCodaData {

public:

   THaCodaData();
   virtual ~THaCodaData();
   virtual int codaOpen(const char* file_name, int mode=1)=0;
   virtual int codaOpen(const char* file_name, const char* session, int mode=1)=0;
   virtual int codaClose()=0;
   virtual int codaRead()=0; 
   virtual int *getEvBuffer() { return evbuffer; }
   virtual int getBuffSize() const { return MAXEVLEN; }
   virtual bool isOpen() const = 0;

private:

   THaCodaData(const THaCodaData &fn);
   THaCodaData& operator=(const THaCodaData &fn);

protected:

   TString filename;
   int *evbuffer;                    // Raw data     

   ClassDef(THaCodaData,0) // Base class of CODA data (file, ET conn, etc)

};

#endif







