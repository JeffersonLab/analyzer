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
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "evio.h"

using namespace std;

namespace Decoder {

//Constructors

  THaCodaFile::THaCodaFile()
    : ffirst(0), max_to_filt(0), handle(0), maxflist(0), maxftype(0) {
    // Default constructor. Do nothing (must open file separately).
  }
  THaCodaFile::THaCodaFile(const char* fname, const char* readwrite)
    : ffirst(0), max_to_filt(0), handle(0), maxflist(0), maxftype(0) {
    // Standard constructor
    Int_t status = codaOpen(fname,readwrite);  // pass read or write flag
    staterr("open",status);
  }

  THaCodaFile::~THaCodaFile () {
    //Destructor
       Int_t status = codaClose();
       staterr("close",status);
  };

  Int_t THaCodaFile::codaOpen(const char* fname, Int_t mode) {
       init(fname);
       // evOpen really wants char*, so we need to do this safely. (The string
       // _might_ be modified internally ...) Silly, really.
       char *d_fname = strdup(fname), *d_flags = strdup("r");
       Int_t status = evOpen(d_fname,d_flags,&handle);
       free(d_fname); free(d_flags);
       staterr("open",status);
       return ReturnCode(status);
  };

  Int_t THaCodaFile::codaOpen(const char* fname, const char* readwrite, Int_t mode) {
      init(fname);
      char *d_fname = strdup(fname), *d_flags = strdup(readwrite);
      Int_t status = evOpen(d_fname,d_flags,&handle);
      free(d_fname); free(d_flags);
      staterr("open",status);
      return ReturnCode(status);
  };


  Int_t THaCodaFile::codaClose() {
// Close the file. Do nothing if file not opened.
    if( handle ) {
      Int_t status = evClose(handle);
      handle = 0;
      return ReturnCode(status);
    }
    return S_SUCCESS;
  }


  Int_t THaCodaFile::codaRead() {
// codaRead: Reads data from file, stored in evbuffer.
// Must be called once per event.
    Int_t status;
    if ( handle ) {
       status = evRead(handle, evbuffer, MAXEVLEN);
       staterr("read",status);
    } else {
      if(CODA_VERBOSE) {
         cout << "codaRead ERROR: tried to access a file with handle = 0" << endl;
         cout << "You need to call codaOpen(filename)" << endl;
         cout << "or use the constructor with (filename) arg" << endl;
      }
      status = S_EVFILE_BADHANDLE;
    }
    return ReturnCode(status);
  };


  Int_t THaCodaFile::codaWrite(const UInt_t* evbuf) {
// codaWrite: Writes data from 'evbuf' to file
     Int_t status;
     if ( handle ) {
       status = evWrite(handle, evbuf);
       staterr("write",status);
     } else {
       cout << "codaWrite ERROR: tried to access file with handle = 0" << endl;
       status = S_EVFILE_BADHANDLE;
     }
     return ReturnCode(status);
   };



  bool THaCodaFile::isOpen() const {
    return (handle!=0);
  }

  Int_t THaCodaFile::filterToFile(const char* output_file) {
// A call to filterToFile filters from present file to output_file
// using filter criteria defined by evtypes, evlist, and max_to_filt
// which are loaded by public methods of this class.  If no conditions
// were loaded, it makes a copy of the input file (i.e. no filtering).

       Int_t i;
       if(filename == output_file) {
	 if(CODA_VERBOSE) {
           cout << "filterToFile: ERROR: ";
           cout << "Input and output files cannot be same " << endl;
           cout << "This is to protect you against overwriting data" << endl;
         }
         return CODA_ERROR;
       }
       FILE *fp;
       if ((fp = fopen(output_file,"r")) != NULL) {
          if(CODA_VERBOSE) {
  	    cout << "filterToFile:  ERROR:  ";
            cout << "Output file `" << output_file << "' exists " << endl;
            cout << "You must remove it by hand first. " << endl;
            cout << "This forces you to think and not overwrite data." << endl;
	  }
          fclose(fp);
          return CODA_ERROR;
       }
       THaCodaFile* fout = new THaCodaFile(output_file,"w");
       Int_t nfilt = 0;

       while (codaRead() == S_SUCCESS) {
           UInt_t* rawbuff = getEvBuffer();
           Int_t evtype = rawbuff[1]>>16;
           Int_t evnum = rawbuff[4];
           Int_t oktofilt = 1;
           if (CODA_DEBUG) {
	     cout << "Input evtype " << dec << evtype;
             cout << "  evnum " << evnum << endl;
             cout << "max_to_filt = " << max_to_filt << endl;
             cout << "evtype size = " << evtypes[0] << endl;
             cout << "evlist size = " << evlist[0] << endl;
	   }
           if ( evtypes[0] > 0 ) {
               oktofilt = 0;
               for (i=1; i<=evtypes[0]; i++) {
                   if (evtype == evtypes[i]) {
                       oktofilt = 1;
                       goto Cont1;
	 	   }
	       }
	   }
Cont1:
           if ( evlist[0] > 0 ) {
               oktofilt = 0;
               for (i=1; i<=evlist[0]; i++) {
                   if (evnum == evlist[i]) {
                       oktofilt = 1;
                       goto Cont2;
		   }
	       }
	   }
Cont2:
	   if (oktofilt) {
             nfilt++;
             if (CODA_DEBUG) {
	       cout << "Filtering event, nfilt " << dec << nfilt << endl;
	     }
	     Int_t status = fout->codaWrite(getEvBuffer());
             if (status != S_SUCCESS) {
	       if (CODA_VERBOSE) {
		 cout << "Error in filterToFile ! " << endl;
                 cout << "codaWrite returned status " << status << endl;
	       }
               goto Finish;
	     }
             if (max_to_filt > 0) {
    	        if (nfilt == max_to_filt) {
                  goto Finish;
	        }
	     }
	   }
       }
Finish:
       delete fout;
       return S_SUCCESS;
  };



  void THaCodaFile::addEvTypeFilt(Int_t evtype_to_filt)
// Function to set up filtering by event type
  {
     initFilter();
     if (evtypes[0] >= maxftype-1) {
        TArrayI temp = evtypes;
        maxftype = maxftype + 100;
        evtypes.Set(maxftype);
        for (Int_t i=0; i<=temp[0]; i++) evtypes[i]=temp[i];
        temp.~TArrayI();
     }
     evtypes[0] = evtypes[0] + 1;  // 0th element = num elements in list
     Int_t n = evtypes[0];
     evtypes[n] = evtype_to_filt;
     return;
  };


  void THaCodaFile::addEvListFilt(Int_t event_num_to_filt)
// Function to set up filtering by list of event numbers
  {
     initFilter();
     if (evlist[0] >= maxflist-1) {
        TArrayI temp = evlist;
        maxflist = maxflist + 100;
        evlist.Set(maxflist);
        for (Int_t i=0; i<=temp[0]; i++) evlist[i]=temp[i];
     }
     evlist[0] = evlist[0] + 1;  // 0th element = num elements in list
     Int_t n = evlist[0];
     evlist[n] = event_num_to_filt;
     return;
  };

  void THaCodaFile::setMaxEvFilt(Int_t max_event)
// Function to set up the max number of events to filter
  {
     max_to_filt = max_event;
     return;
  };



void THaCodaFile::staterr(const char* tried_to, Long64_t status) {
// staterr gives the non-expert user a reasonable clue
// of what the status returns from evio mean.
// Note: severe errors can cause job to exit(0)
// and the user has to pay attention to why.
    if (status == S_SUCCESS) return;  // everything is fine.
    if (status == EOF) {
      if(CODA_VERBOSE) {
	cout << "Normal end of file " << filename << " encountered" << endl;
      }
      return;
    }
    cerr << Form("THaCodaFile: ERROR while trying to %s %s: ",
		 tried_to, filename.Data());
    switch (status) {
      case S_EVFILE_TRUNC :
	cerr << "Truncated event on file read. Evbuffer size is too small. "
	     << endl;
      case S_EVFILE_BADBLOCK :
        cerr << "Bad block number encountered " << endl;
        break;
      case S_EVFILE_BADHANDLE :
        cerr << "Bad handle (file/stream not open) " << endl;
        break;
      case S_EVFILE_ALLOCFAIL :
        cerr << "Failed to allocate event I/O" << endl;
        break;
      case S_EVFILE_BADFILE :
        cerr << "File format error" << endl;
        break;
      case S_EVFILE_UNKOPTION :
        cerr << "Unknown file open option specified" << endl;
        break;
      case S_EVFILE_UNXPTDEOF :
        cerr << "Unexpected end of file while reading event" << endl;
        break;
      default:
	errno = status;
	perror(0);
      }
  };

  void THaCodaFile::init(const char* fname) {
    if( filename != fname ) {
      codaClose();
      filename = fname;
    }
    handle = 0;
  };

  void THaCodaFile::initFilter() {
    if (!ffirst) {
       ffirst = 1;
       maxflist = 100;
       maxftype = 100;
       evlist.Set(maxflist);
       evtypes.Set(maxftype);
       evlist[0] = 0;
       evtypes[0] = 0;
    }
  };

}

ClassImp(Decoder::THaCodaFile)
