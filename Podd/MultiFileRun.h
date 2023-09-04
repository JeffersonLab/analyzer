#ifndef Podd_MultiFileRun_h_
#define Podd_MultiFileRun_h_

//////////////////////////////////////////////////////////////////////////
//
// Podd::MultiFileRun
//
// This class presents multiple CODA input files as if they were a
// single file through the THaRun API. It supports files split into
// consecutive "segments", files written in parallel by CODA in
// multi-stream mode, and a combination of both. Both CODA 2 and CODA 3
// file formats are supported, as with THaRun.
//
// For convenience, wildcard and regular expression file names are
// supported. File directory paths can be either part of the file name,
// with support for wildcards/regexp as well, or be specified in a vector
// of strings (again with wildcard/regexp support). Filenames and paths may
// contain environment variables, which will be expanded if
// defined. (Undefined environment variables generate an error.)
//
// The file name convention is <name>.<stream_index>.<segment_index> or
// <name>.<segment_index>. If the stream index is missing, a single stream
// is assumed. These patterns can be changed by overriding certain virtual
// methods.
//
// Examples:
//
// #include "MultiFileRun.h"
//
// auto run =
//   make_unique<Podd::MultiFileRun>("/daq/data[1-4]/e1234_evio.?.[0-9]+",
//                                   "Run 1234 all files");
//
// vector<string> paths{"/daq/data[1-4]", "/cache/hallx/raw", "$CACHE/copies"};
// auto run =
//   make_unique<Podd::MultiFileRun>(paths, "e1234_evio.*.*",
//                             "Run 1234 from daq and cache all files");
//
// run->Init();
// run->Print();
//
// With the powerful wildcard/regexp matching capability, one has to take
// care that files from different runs are not mixed accidentally.  The
// class will refuse to process files whose stream and segment (or only the
// segment in case of a single stream) are identical.  Additionally, a
// warning will be printed if the file name stem (<name> in the pattern
// above) is not identical for all files.  Also, different CODA formats
// cannot be mixed.
//
// File segments are read sequentially. File streams are read in parallel,
// where data from the stream with the lowest current physics event number
// will be presented first. With the usual CODA round-robin write strategy,
// this will normally yield consecutive event numbers on consecutive calls
// to ReadEvent().
//
// Special events (e.g. slow controls, scalers) may not be delivered in the
// exact order in which they were written. This behavior may be fine-tuned
// in a future release.
//
// The current input file, segment number, and stream number can be
// obtained from GetFilename(), GetSegment(), and GetStream(),
// respectively, after each call to ReadEvent().
//
// Initialization info is retrieved from segment 0, as with THaRun. If this
// segment is not part of the input file list, the code will attempt to
// locate it in the current run's directory and in any directories in the
// search path list combined with any directory components in the file list.
//
// Currently, it is assumed that segment 0 of any stream provides init info.
// If only stream 0 provides it, for example, it will be necessary to override
// THaRun::ProvidesInitInfo() and THaRun::GetInitInfoFileName to reflect that.
// Details naturally depend on the experiment-specific DAQ configuration.
//
// Like THaRun, this class can be persisted through ROOT I/O. This will
// save, among other data, the paths and stream/segment info of all input
// files matched at initialization time.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRun.h"
#include "THaCodaData.h"
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <functional>    // std::function

class TRegexp;

namespace Podd {

class MultiFileRun : public THaRun {
public:
  explicit MultiFileRun( const char* filenamePattern = "",
                         const char* description = "",
                         bool is_regex = false );
  explicit MultiFileRun( std::vector<std::string> pathList,
                         const char* filenamePattern = "",
                         const char* description = "",
                         bool is_regex = false );
  explicit MultiFileRun( std::vector<std::string> pathList,
                         std::vector<std::string> fileList,
                         const char* description = "",
                         bool is_regex = false );
  MultiFileRun( const MultiFileRun& run );
  MultiFileRun& operator=( const THaRunBase& rhs );

  virtual void   Clear( Option_t* opt="" );
  virtual Int_t  Close();
  virtual Int_t  Compare( const TObject* obj ) const;
  virtual Int_t  GetDataVersion();
  virtual const UInt_t* GetEvBuffer() const;
  // Get the last-seen physics event number. For CODA 3 in block mode, this
  // is the event number of the first event in the block.
  virtual UInt_t GetEvNum() const;
  virtual Bool_t IsOpen() const;
  virtual Int_t  Open();
  virtual void   Print( Option_t* opt="" ) const;
  virtual Int_t  ReadEvent();
  virtual Int_t  SetFilename( const char* name );
  bool           SetFileList( std::vector<std::string> filelist );
  bool           SetPathList( std::vector<std::string> pathlist );
  void           SetFlags( UInt_t set ) { fFlags = set; }
  UInt_t         GetFlags() const { return fFlags; }

  // These getters will return valid data after Init()
  // Number of input files found in all streams
  UInt_t         GetNFiles() const;
  // Number of streams found
  UInt_t         GetNStreams() const;
  // Lowest segment number found in any stream (>= fFirstSegment)
  Int_t          GetStartSegment() const;
  // Highest segment number found in any stream
  Int_t          GetLastSegment() const;
  // Lowest stream number found (-1 = no stream index found)
  Int_t          GetStartStream() const;
  // Highest stream number found (-1 = no stream index found)
  Int_t          GetLastStream() const;
  // Complete list of input files found
  std::vector<std::string> GetFiles() const;
  // Number of successful calls to ReadEvent()
  UInt_t         GetNevRead() const { return fNevRead; }

  // Configuration of segment/stream ranges. If called, must Init() again
  void           SetFirstSegment( Int_t n );
  void           SetFirstStream( Int_t n );
  void           SetMaxSegments( Int_t n );
  void           SetMaxStreams( Int_t n );

  // True if filename pattern is to be interpreted as a full ROOT TRegexp,
  // false if interpreted as a wildcard expression
  Bool_t         IsNameRegexp() const { return fNameIsRegexp; }

  struct FileInfo {
    FileInfo() : fSegment{-1} {}
    FileInfo( std::string path, std::string stem, Int_t seg );
    bool operator< ( const FileInfo& rhs ) const {
      return fSegment < rhs.fSegment;
    }
    std::string fPath;       // Full file path
    std::string fStem;       // File basename stem (without stream/segment no)
    Int_t       fSegment;    // File segment number
    ClassDefNV(FileInfo, 1)    // CODA file descriptor for MultiFileRun
  } __attribute__((aligned(64)));

  struct StreamInfo {
    StreamInfo();
    explicit StreamInfo( Int_t id );
    StreamInfo( const StreamInfo& rhs );
    StreamInfo& operator=( const StreamInfo& rhs );
    bool operator==( const StreamInfo& rhs ) const {
      return fID == rhs.fID && fVersion == rhs.fVersion;
    }
    bool operator<( const StreamInfo& rhs ) const {
      if( fID != rhs.fID ) return fID < rhs.fID;
      return fVersion < rhs.fVersion;
    }
    Int_t Open();
    Int_t Read();
    Int_t Close();
    Bool_t IsGood() const;
    const UInt_t* GetEvBuffer() const;
    const std::string& GetFilename() const;

    std::unique_ptr<Decoder::THaCodaData> fCodaData;  //! Coda data (file)
    std::vector<FileInfo> fFiles;
    Int_t  fID;              // Stream ID (-1 = default/none)
    Int_t  fVersion;         // CODA version
    Int_t  fFileIndex;       //! Index of currently open file
    UInt_t fEvNum;           //! Number of most recent physics event
    Bool_t fActive;          //! Stream has not yet reached EOF
  private:
    Int_t OpenCurrent();
    Int_t FetchEventNumber();
    ClassDefNV(StreamInfo, 1)  // CODA stream descriptor for MultiFileRun
  } __attribute__((aligned(64)));

  //TODO: not yet implemented, may change
  enum EFlags {
    kRequireAllSegments,     // Require all segments in range present
    kRequireAllFiles,        // Require all non-wildcard files present
    kDoNotSkipDupFileNames   // Keep searching even if file already found
  };

protected:
  // Configuration
  std::vector<std::string> fFileList;  // File name pattern (wildcard or regexp)
  std::vector<std::string> fPathList;  // List of search paths
  std::vector<StreamInfo>  fStreams;   // Event streams to process
  Int_t  fFirstSegment;                // First segment number to process
  Int_t  fFirstStream;                 // First stream number to process
  UInt_t fMaxSegments;                 // Maximum number of segments to process
  UInt_t fMaxStreams;                  // Maximum number of streams to process
  UInt_t fFlags;                       // Flags (see EFlags)
  Bool_t fNameIsRegexp;                // Interpret path/file names as TRegexp
  // Working data
  Int_t  fLastUsedStream;              //! Index of last stream that was read
  Int_t  fNActive;                     //! Number of active streams
  UInt_t fNevRead;                     //! Number of events read

  virtual Int_t    BuildInputList();
  virtual Bool_t   FindSegmentNumber();
  virtual Int_t    FindNextStream() const;
  virtual TString  FindInitInfoFile( const TString& fname );

  bool  CheckWarnAbsFilename();
  void  ClearStreams();
  void  ExpandFileName( std::string& str ) const;
  bool  HasWildcards( const TString& str ) const;
  void  PrintStreamInfo() const;
  void  PrintFileInfo() const;

private:
  using path_t = std::pair<TString, TString>;
  using action_t = std::function<Int_t( const TString&, const TString& )>;
  Int_t AddFile( const TString& file, const TString& dir );
  void  AssembleFilePaths( std::vector<path_t>& candidates );
  void  AssembleFilePaths( std::vector<path_t>& candidates,
                           const std::vector<std::string>& file_list );
  Int_t BuildInputListFromWildcardDir( const path_t& path, const action_t& action );
  Int_t BuildInputListFromTopDir( const path_t& path, const action_t& action );
  Int_t CheckFilesConsistency();
  Int_t DescendInto( const TString& curpath, const std::vector<TString>& splitpath,
                     Int_t level, const action_t& action );
  Int_t ForEachMatchItemInDir( const TString& dir, const TRegexp& match_re,
                               const action_t& action );
  Int_t ScanForFilename( const path_t& path, bool regex_mode, const action_t& action );
  Int_t ScanForSubdirs( const TString& curdir, const std::vector<TString>& splitpath,
                        Int_t level, bool regex_mode, const action_t& action );
  void  SortStreams();

  enum { kResolvingWildcard = BIT(18) };

  ClassDef(MultiFileRun, 2)            // CODA data from multiple files
};

} //namespace Podd

#endif //Podd_MultiFileRun_h_
