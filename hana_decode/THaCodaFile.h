#ifndef THaCodaFile_h
#define THaCodaFile_h

/////////////////////////////////////////////////////////////////////
//
//  THaCodaFile
//  File of CODA data
//
//  CODA data file, and facilities to open, close, read,
//  write, filter CODA data to disk files, etc.
//  A lot of this relies on the "evio.cpp" code from
//  DAQ group which is a C++ rendition of evio.c that
//  we have used for years, but here are some useful
//  added features.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include "THaCodaData.h"
#include "TArrayI.h"

class THaCodaFile : public THaCodaData {

public:

  THaCodaFile();
  THaCodaFile(const char* filename, const char* rw="r");
  ~THaCodaFile();
  int codaOpen(const char* filename, int mode=1);
  int codaOpen(const char* filename, const char* rw, int mode=1);
  int codaClose();
  int codaRead(); 
  int codaWrite(const int* evbuffer);
  int *getEvBuffer();     
  int filterToFile(const char* output_file); // filter to an output file
  void addEvTypeFilt(int evtype_to_filt);    // add an event type to list
  void addEvListFilt(int event_to_filt);     // add an event num to list
  void setMaxEvFilt(int max_event);          // max num events to filter
  virtual bool isOpen() const;

private:

  THaCodaFile(const THaCodaFile &fn);
  THaCodaFile& operator=(const THaCodaFile &fn);
  void init(const char* fname="");
  void initFilter();
  void staterr(const char* tried_to, int status);  // Can cause job to exit(0)
  int ffirst;
  int max_to_filt,handle;
  int maxflist,maxftype;
  TArrayI evlist, evtypes;

  ClassDef(THaCodaFile,0)   //  File of CODA data

};

#endif






