#ifndef Podd_THaCodaFileh_h_
#define Podd_THaCodaFileh_h_

/////////////////////////////////////////////////////////////////////
//
//  THaCodaFile
//
//  Class for accessing a CODA data file. Provides facilities to open,
//  close, read, write, filter CODA data to disk files, etc.
//
//  This is largely a wrapper around the JLAB EVIO library.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaData.h"
#include "Decoder.h"
#include <vector>

namespace Decoder {

class THaCodaFile : public THaCodaData {

public:

  THaCodaFile();
  explicit THaCodaFile(const char* filename, const char* rw="r");
  THaCodaFile(const THaCodaFile &fn) = delete;
  THaCodaFile& operator=(const THaCodaFile &fn) = delete;
  ~THaCodaFile() override;
  Int_t codaOpen(const char* filename, Int_t mode=1) override;
  Int_t codaOpen(const char* filename, const char* rw, Int_t mode=1) override;
  Int_t codaClose() override;
  Int_t codaRead() override;
  Int_t codaWrite(const UInt_t* evbuffer);
  Int_t filterToFile(const char* output_file); // filter to an output file
  void  addEvTypeFilt(UInt_t evtype_to_filt);  // add an event type to list
  void  addEvListFilt(UInt_t event_to_filt);   // add an event num to list
  void  setMaxEvFilt(UInt_t max_event);        // max num events to filter
  bool isOpen() const override;

private:

  void init(const char* fname="");
  UInt_t max_to_filt;
  UInt_t maxflist,maxftype;
  std::vector<UInt_t> evlist, evtypes;

  ClassDefOverride(THaCodaFile,0)   //  File of CODA data

};

} // namespace Decoder

#endif
