#ifndef Podd_THaCodaFileh_h_
#define Podd_THaCodaFileh_h_

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

#include "THaCodaData.h"
#include "Decoder.h"
#include "TArrayI.h"

namespace Decoder {

class THaCodaFile : public THaCodaData {

public:

  THaCodaFile();
  explicit THaCodaFile(const char* filename, const char* rw="r");
  virtual ~THaCodaFile();
  virtual Int_t codaOpen(const char* filename, Int_t mode=1);
  virtual Int_t codaOpen(const char* filename, const char* rw, Int_t mode=1);
  virtual Int_t codaClose();
  virtual Int_t codaRead();
  Int_t codaWrite(const UInt_t* evbuffer);
  Int_t filterToFile(const char* output_file); // filter to an output file
  void  addEvTypeFilt(Int_t evtype_to_filt);   // add an event type to list
  void  addEvListFilt(Int_t event_to_filt);    // add an event num to list
  void  setMaxEvFilt(Int_t max_event);         // max num events to filter
  virtual bool isOpen() const;

private:

  THaCodaFile(const THaCodaFile &fn);
  THaCodaFile& operator=(const THaCodaFile &fn);
  void init(const char* fname="");
  void initFilter();
  Int_t ffirst;
  Int_t max_to_filt;
  Int_t maxflist,maxftype;
  TArrayI evlist, evtypes;

  ClassDef(THaCodaFile,0)   //  File of CODA data

};

}

#endif
