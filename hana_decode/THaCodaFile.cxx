/////////////////////////////////////////////////////////////////////
//
//  THaCodaFile
//  File of CODA data
//
//  CODA data file, and facilities to open, close, read,
//  write, filter CODA data to disk files, etc.
//
//  This is largely a wrapper around the JLAB EVIO library.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaFile.h"
#include "Helper.h"
#include "TSystem.h"
#include "evio.h"
#include <iostream>
#include <memory>
#include <algorithm>

using namespace std;

namespace Decoder {

//Constructors

//_____________________________________________________________________________
  THaCodaFile::THaCodaFile()
    : max_to_filt(0), maxflist(0), maxftype(0)
  {
    // Default constructor. Do nothing (must open file separately).
  }

//_____________________________________________________________________________
  THaCodaFile::THaCodaFile(const char* fname, const char* readwrite)
    : max_to_filt(0), maxflist(0), maxftype(0)
  {
    // Standard constructor. Pass read or write flag
    THaCodaFile::codaOpen(fname, readwrite);
  }

//_____________________________________________________________________________
  THaCodaFile::~THaCodaFile ()
  {
    //Destructor
    THaCodaFile::codaClose();
  }

//_____________________________________________________________________________
  Int_t THaCodaFile::codaOpen(const char* fname, Int_t mode )
  {
    // Open CODA file 'fname' in read-only mode
    return codaOpen( fname, "r", mode );
  }

//_____________________________________________________________________________
  Int_t THaCodaFile::codaOpen(const char* fname, const char* readwrite,
                              Int_t /* mode */ )
  {
    // Open CODA file 'fname' with 'readwrite' access
    init(fname);
    Int_t status = evOpen((char*)fname, (char*)readwrite, &handle);
    fIsGood = (status == S_SUCCESS);
    staterr("open",status);
    return ReturnCode(status);
  }

//_____________________________________________________________________________
  Int_t THaCodaFile::codaClose() {
// Close the file. Do nothing if file not opened.
    if( !handle ) {
      return ReturnCode(S_SUCCESS);
    }
    Int_t status = evClose(handle);
    handle = 0;
    fIsGood = (status == S_SUCCESS);
    staterr("close",status);
    return ReturnCode(status);
  }

//_____________________________________________________________________________
  Int_t THaCodaFile::codaRead() {
// codaRead: Reads data from file, stored in evbuffer.
// Must be called once per event.
    if( !handle ) {
      if (verbose > 0) {
        cout << "codaRead ERROR: tried to access a file with handle = 0" << endl;
        cout << "You need to call codaOpen(filename)" << endl;
        cout << "or use the constructor with (filename) arg" << endl;
      }
      return ReturnCode(S_EVFILE_BADHANDLE);
    }
    Int_t status = S_SUCCESS;
    do {
      evbuffer.updateSize();
      status = evRead(handle, getEvBuffer(), getBuffSize());
      if( status == S_EVFILE_TRUNC ) {
        // At least with EVIO version 5.2, probably earlier and hopefully later
        // versions too, evRead has not consumed any buffer data if this
        // error occurs. Thus growing the buffer and retrying is safe.
        // Unfortunately, the EVIO C-API does not provide any means to access
        // the actual event length here, so we have to guess how much more
        // space is needed.  TODO: Make an EVIO feature request?
        if( !evbuffer.grow() )
          break;
      }
    } while( status == S_EVFILE_TRUNC );

    if( status == S_SUCCESS )
      evbuffer.recordSize();

    fIsGood = (status == S_SUCCESS || status == EOF );
    staterr("read",status);
    return ReturnCode(status);
  }


//_____________________________________________________________________________
  Int_t THaCodaFile::codaWrite(const UInt_t* evbuf) {
// codaWrite: Writes data from 'evbuf' to file
    if( !handle ) {
      cout << "codaWrite ERROR: tried to access file with handle = 0" << endl;
      return ReturnCode(S_EVFILE_BADHANDLE);
    }
    Int_t status = evWrite(handle, evbuf);
    fIsGood = (status == S_SUCCESS);
    staterr("write",status);
    return ReturnCode(status);
  }

  bool THaCodaFile::isOpen() const {
    return (handle!=0);
  }

//_____________________________________________________________________________
template<typename T>
struct Equals {
  const T m_val;
  explicit Equals(T val) : m_val(val) {}
  bool operator()(T val) const { return val == m_val; }
};

//_____________________________________________________________________________
Int_t THaCodaFile::filterToFile( const char* output_file )
{
// A call to filterToFile filters from present file to output_file
// using filter criteria defined by evtypes, evlist, and max_to_filt
// which are loaded by public methods of this class.  If no conditions
// were loaded, it makes a copy of the input file (i.e. no filtering).

  if( filename == output_file ) {
    if (verbose > 0) {
      cout << "filterToFile: ERROR: ";
      cout << "Input and output files cannot be same " << endl;
      cout << "This is to protect you against overwriting data" << endl;
    }
    return CODA_ERROR;
  }
  if( !gSystem->AccessPathName(output_file, kReadPermission) ) {
    if (verbose > 0) {
      cout << "filterToFile:  ERROR:  ";
      cout << "Output file `" << output_file << "' exists " << endl;
      cout << "You must remove it by hand first. " << endl;
      cout << "This forces you to think and not overwrite data." << endl;
    }
    fIsGood = false;
    return CODA_FATAL;
  }
  unique_ptr<THaCodaFile> fout{new THaCodaFile(output_file, "w")};
  if( !fout->isGood()) {
    fIsGood = false;
    return CODA_FATAL;
  }

  UInt_t nfilt = 0;
  Int_t status = CODA_OK, fout_status = CODA_OK;
  while( (status = codaRead()) == CODA_OK ) {
    UInt_t* rawbuff = getEvBuffer();
    UInt_t evtype = rawbuff[1] >> 16;
    UInt_t evnum = rawbuff[4];
    if (verbose > 1) {
      cout << "Input evtype " << dec << evtype;
      cout << "  evnum " << evnum << endl;
      cout << "max_to_filt = " << max_to_filt << endl;
      cout << "evtype size = " << evtypes.size() << endl;
      cout << "evlist size = " << evlist.size() << endl;
    }

    Bool_t oktofilt = true;
    if( !evtypes.empty() )
      oktofilt = any_of(ALL(evtypes), Equals<UInt_t>(evtype));
    // JOH: Added this test to let the filter act as a logical AND of
    // the configured event types and event numbers, which is more general.
    // Empty event type or event number lists always pass. I.e. if both lists
    // are empty, the filter passes all events. If only event types are set,
    // it passes only those types, regardless of event number. Etc.
    // The previous behavior was to ignore any configured event types if
    // event numbers were also configured. Obviously, that's a special case of
    // the above which one can achieve by leaving the event type list empty.
    if( !oktofilt )
      continue;
    if( !evlist.empty() )
      oktofilt = any_of(ALL(evlist), Equals<UInt_t>(evnum));
    if( !oktofilt )
      continue;

    nfilt++;
    if (verbose > 1) {
      cout << "Filtering event, nfilt " << dec << nfilt << endl;
    }
    fout_status = fout->codaWrite(getEvBuffer());
    if( fout_status != CODA_OK ) {
      if (verbose > 0) {
        cout << "Error in filterToFile ! " << endl;
        cout << "codaWrite returned status " << fout_status << endl;
      }
      break;
    }
    if( max_to_filt > 0 ) {
      if( nfilt == max_to_filt ) {
        break;
      }
    }
  }
  if( status == CODA_EOF) // EOF is normal
    status = CODA_OK;
  fIsGood = (status == CODA_OK);

  fout_status = fout->codaClose();

  return fIsGood ? fout_status : status;
}

//_____________________________________________________________________________
  void THaCodaFile::addEvTypeFilt(UInt_t evtype_to_filt)
// Function to set up filtering by event type
  {
     if( evtypes.capacity() < 16 )
       // Typical filtering scenarios involve a small number of event types
       evtypes.reserve(16);
     evtypes.push_back(evtype_to_filt);
  }

//_____________________________________________________________________________
  void THaCodaFile::addEvListFilt(UInt_t event_to_filt)
// Function to set up filtering by list of event numbers
  {
     if( evlist.capacity() < 1024 )
       // Event lists tend to be lengthy, so start out with a generous size
       evlist.reserve(1024);
     evlist.push_back(event_to_filt);
  }

//_____________________________________________________________________________
  void THaCodaFile::setMaxEvFilt(UInt_t max_event)
// Function to set up the max number of events to filter
  {
     max_to_filt = max_event;
  }

//_____________________________________________________________________________
  void THaCodaFile::init(const char* fname) {
    if( filename != fname ) {
      codaClose();
      filename = fname;
    }
    handle = 0;
  }
}

//_____________________________________________________________________________
ClassImp(Decoder::THaCodaFile)
