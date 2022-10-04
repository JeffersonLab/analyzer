//*-- Author :    Ole Hansen   02-Nov-21

//////////////////////////////////////////////////////////////////////////
//
// Podd::MultiFileRun
//
//////////////////////////////////////////////////////////////////////////

#include "MultiFileRun.h"
#include "THaCodaFile.h"
#include "THaPrintOption.h"
#include "CodaDecoder.h"
#include "TRegexp.h"
#include "TSystem.h"
#include <utility>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <type_traits>   // std::make_signed
#include <functional>    // std::function
#include <limits>

using namespace std;
using namespace Decoder;

#if __cplusplus >= 201402L
# define MKCODAFILE make_unique<Decoder::THaCodaFile>()
#else
# define MKCODAFILE unique_ptr<Decoder::THaCodaFile>(new Decoder::THaCodaFile)
#endif
#define ALL(c) (c).begin(), (c).end()
#define SINT(i) static_cast<std::make_signed<decltype(i)>::type>(\
(i) > std::numeric_limits<std::make_signed<decltype(i)>::type>::max() ? -1 :(i))
#define SSIZE(c) SINT((c).size())

namespace Podd {

//_____________________________________________________________________________
MultiFileRun::MultiFileRun( const char* filenamePattern,
                            const char* description, bool is_regex )
  : THaRun(filenamePattern, description)
  , fFilenamePattern{filenamePattern}
  , fFirstSegment{0}
  , fFirstStream{0}
  , fMaxSegments{0}
  , fMaxStreams{0}
  , fNameIsRegexp{is_regex}
  , fLastUsedStream{-1}
  , fNActive{0}
  , fNevRead{0}
{
  fCodaData.reset(); // Not used

  // Expand environment variables and leading "~" in file name pattern
  ExpandFileName(fFilenamePattern);
}

//_____________________________________________________________________________
MultiFileRun::MultiFileRun( std::vector<std::string> pathList,
                            const char* filenamePattern,
                            const char* description, bool is_regex )
  : THaRun(filenamePattern, description)
  , fFilenamePattern{filenamePattern}
  , fPathList{std::move(pathList)}
  , fFirstSegment{0}
  , fFirstStream{0}
  , fMaxSegments{0}
  , fMaxStreams{0}
  , fNameIsRegexp{is_regex}
  , fLastUsedStream{-1}
  , fNActive{0}
  , fNevRead{0}
{
  fCodaData.reset(); // Not used

  // Expand environment variables and leading "~" in file paths
  ExpandFileName(fFilenamePattern);
  for( auto& path: fPathList )
    ExpandFileName(path);

  CheckWarnAbsFilename();
}

//_____________________________________________________________________________
MultiFileRun::MultiFileRun( const MultiFileRun& rhs )
  : THaRun(rhs)
  , fFilenamePattern{rhs.fFilenamePattern}
  , fPathList{rhs.fPathList}
  , fStreams{rhs.fStreams}
  , fFirstSegment{rhs.fFirstSegment}
  , fFirstStream{rhs.fFirstStream}
  , fMaxSegments{rhs.fMaxSegments}
  , fMaxStreams{rhs.fMaxStreams}
  , fNameIsRegexp{rhs.fNameIsRegexp}
  , fLastUsedStream{-1}
  , fNActive{0}
  , fNevRead{0}
{
  fCodaData.reset(); // Not used. THaRun copy c'tor sets it
}

//_____________________________________________________________________________
MultiFileRun& MultiFileRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator

  if( this != &rhs ) {
    THaRun::operator=(rhs);

    fCodaData.reset();  // Not used. THaRun::operator= sets it
    fLastUsedStream = -1;
    fNActive = 0;
    fNevRead = 0;
    try {
      const auto& mfr = dynamic_cast<const MultiFileRun&>(rhs);
      fFilenamePattern = mfr.fFilenamePattern;
      fPathList        = mfr.fPathList;
      fStreams         = mfr.fStreams;
      fFirstSegment    = mfr.fFirstSegment;
      fFirstStream     = mfr.fFirstStream;
      fMaxSegments     = mfr.fMaxSegments;
      fMaxStreams      = mfr.fMaxStreams;
      fNameIsRegexp    = mfr.fNameIsRegexp;
    }
    catch( const std::bad_cast& ) {
      // Assigning from a different class. Not a good idea, but anyway.
      fFilenamePattern = fFilename;
      fPathList.clear();
      fFirstSegment = fFirstStream = 0;
      fMaxSegments = fMaxStreams = 0;
      fNameIsRegexp = false;
    }
  }
  return *this;
}


//_____________________________________________________________________________
void MultiFileRun::Clear( Option_t* opt )
{
  THaPrintOption sopt(opt);
  sopt.ToUpper();
  bool doing_init = (sopt.Contains("INIT"));

  // If initializing, keep explicitly set parameters
  if( !doing_init ) {
    fFirstSegment = fFirstStream = 0;
    fMaxSegments = fMaxStreams = 0;
  }
  // Clear the base classes. This calls our Close(), which resets fNActive etc.
  THaRun::Clear(opt);

  fLastUsedStream = -1;
  fNActive = 0;
  fNevRead = 0;
  fStreams.clear();
}

//_____________________________________________________________________________
Int_t MultiFileRun::Close()
{
  if( !IsOpen() )
    return READ_OK;

  fOpened = false;
  fNActive = 0;
  fNevRead = 0;

  Int_t err = CODA_OK;
  for( auto& stream: fStreams ) {
    Int_t st = stream.Close();
    if( st != CODA_OK )
      err = st;
  }
  return ReturnCode(err);
}

//_____________________________________________________________________________
Int_t MultiFileRun::Compare( const TObject* obj ) const
{
  // Compare this MultiFileRun object to another run. Returns 0 when equal,
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).
  // Used by ROOT containers.

  if (this == obj) return 0;
  const auto* rhs = dynamic_cast<const THaRunBase*>(obj);
  if( !rhs ) return -1;
  // Compare run numbers
  if( *this < *rhs )       return -1;
  else if( *rhs < *this )  return  1;
  const auto* mfr = dynamic_cast<const MultiFileRun*>(rhs);
  if( !mfr ) return 1;
  if( fStreams > mfr->fStreams ) return 1;
  if( fStreams < mfr->fStreams ) return -1;
  return 0;
}

//_____________________________________________________________________________
Int_t MultiFileRun::GetDataVersion()
{
  // fDataVersion is either 0 if we're uninitialized or set in BuildInputList.
  return fDataVersion;
}

//_____________________________________________________________________________
const UInt_t* MultiFileRun::GetEvBuffer() const
{
  if( fCodaData ) {
    // This is set only during ReadInitInfo when prescanning a separate file
    assert(fCodaData->isOpen());
    return fCodaData->getEvBuffer();
  }

  if( !IsOpen() ) {
    cerr << "Not open" << endl;
    return nullptr;
  }
  if( fNevRead == 0 ) {
    cerr << "No events read" << endl;
    return nullptr;
  }

  assert(!fStreams.empty());
  if( fStreams.empty() )
    return nullptr;
  const auto& curstr = fStreams[fLastUsedStream];
  assert(curstr.fActive);
  if( !curstr.fActive ) {
    // The stream has seen EOF and so the buffer contains no valid data
    cerr << "At EOF" << endl;
    return nullptr;
  }
  return curstr.GetEvBuffer();
}

//_____________________________________________________________________________
UInt_t MultiFileRun::GetEvNum() const
{
  assert(!fStreams.empty());
  if( fStreams.empty() )
    return 0;
  const auto& curstr = fStreams[fLastUsedStream];
  assert(curstr.fActive);
  if( !curstr.fActive )
    return 0;
  return curstr.fEvNum;
}

//_____________________________________________________________________________
Bool_t MultiFileRun::IsOpen() const
{
  return fOpened;
}

//_____________________________________________________________________________
static Int_t CheckSetFileDataVersion( const string& path,
                                      Int_t ver, Int_t& data_ver )
{
  Int_t err = CODA_OK;
  if( ver > 0 ) {          // got a valid version from file
    if( data_ver == 0 ) {  // not yet set
      data_ver = ver;
    } else if( ver != data_ver ) {
      // all input files must the same format
      cerr << "Inconsistent CODA version " << ver << " for file "
           << path << ", expected " << data_ver << endl;
      err = CODA_ERROR;
    }
    //else already set and agrees
  } else {
    // error getting format
    cerr << "Error getting CODA version from file " << path << endl;
    err = CODA_ERROR;
  }
  return err;
}

//_____________________________________________________________________________
// If there are multiple input streams and/or files, check for consistency
// of file naming (assumed to be representative of the run number) and
// each file's CODA version
Int_t MultiFileRun::CheckFilesConsistency()
{
  Int_t err = CODA_OK, file_data_version = 0;
  TString stem;
  for( auto& stream: fStreams ) {
    assert(!stream.fFiles.empty());
    for( const auto& file: stream.fFiles ) {
      // Since CODA does not currently store the run number in each file
      // segment, we have to rely on filename heuristics to guess whether the
      // user is trying to do something unwise, like mixing different runs.
      if( stem.IsNull() )
        stem = file.fStem;
      else if( file.fStem != stem ) {
        cerr << "Warning: Inconsistent file naming: \"" << file.fStem
             << "\" vs. \"" << stem << ". Are these the same runs?" << endl;
      }
      assert(stream.fCodaData);  // else bad stream constructor
      Int_t ret = stream.fCodaData->codaOpen(file.fPath.c_str());
      if( ret != CODA_OK ) {
        cerr << "Error " << ret << " opening CODA file " << file.fPath << endl;
      } else {
        ret = CheckSetFileDataVersion(
          file.fPath, stream.fCodaData->getCodaVersion(), file_data_version);
      }
      if( ret != CODA_OK )
        err = ret;
      stream.Close();
    }
  }
  if( err == CODA_OK ) {
    assert(file_data_version > 0);
    if( fDataVersion == 0 )
      fDataVersion = file_data_version;

  } else {
    cerr << "One or more files had errors" << endl;
  }
  return ReturnCode(err);
}

//_____________________________________________________________________________
// Extract segment and stream number of 'file' and add the file path to
// the list of files for the corresponding stream
Int_t MultiFileRun::AddFile( const TString& file, const TString& dir )
{
  Int_t seg = -1, str = -1;
  TString stem;
  if( StdFindSegmentNumber(file, stem, seg, str)
      && (seg == -1 || seg >= fFirstSegment)
      && (str == -1 || str >= fFirstStream) ) {
    TString path = (dir != ".") ? dir + "/" + file : file;
    auto it = find_if(ALL(fStreams), [str]( const StreamInfo& ifo ) {
      return ifo.fID == str;
    });
    if( it == fStreams.end() ) {
      fStreams.emplace_back(str);
      it = std::prev(fStreams.end());
    }
    auto& files = it->fFiles;
    auto jt = find_if(ALL(files), [seg]( const FileInfo& fi ) {
      return fi.fSegment == seg;
    });
    if( jt == files.end() )
      files.emplace_back(path.Data(), stem.Data(), seg);
    else {
      if( jt->fStem == stem ) {
        cerr << "Warning: Duplicate segment number: "
             << jt->fPath << " : " << path << endl
             << "Ignoring apparently identical file " << path << endl;
      } else {
        cerr << "Error: Duplicate segment number: "
             << jt->fPath << " : " << path << endl
             << "Files seem different, check your filename pattern." << endl;
        return READ_ERROR;
      }
    }
  }
  return READ_OK;
}

//_____________________________________________________________________________
// Sort streams by number. Sort files of each stream by segment number.
// Limit number of streams and files to fMaxStreams and fMaxSegments, resp.
void MultiFileRun::SortStreams()
{
  sort(ALL(fStreams));

  if( fMaxStreams > 0 && fStreams.size() > fMaxStreams ) {
    fStreams.resize(fMaxStreams);
  }

  // Sort each stream's files by segment number
  size_t nfiles = 0;
  for( auto& stream: fStreams ) {
    auto& files = stream.fFiles;
    sort(ALL(files));
    if( fMaxSegments > 0 && files.size() > fMaxSegments )
      files.resize(fMaxSegments);
    nfiles += files.size();
  }
  cout << "MultiFileRun: " << nfiles << " file";
  if( nfiles != 1 ) cout << 's';
  cout << " in " << fStreams.size() << " stream";
  if( fStreams.size() != 1 ) cout << 's';
  cout << endl;
}

//_____________________________________________________________________________
static vector<TString> SplitPath( const string& path )
{
  vector<TString> vs;
  TString dirn = path;
  while( dirn != "/" && dirn != "." ) {
    vs.emplace_back(gSystem->BaseName(dirn));
    dirn = gSystem->GetDirName(dirn);
  }
  vs.emplace_back(dirn);  // top directory
  reverse(ALL(vs));
  return vs;
}

//_____________________________________________________________________________
static inline TString JoinPath( const TString& base, const TString& to_join )
{
  return base + (base.EndsWith("/") ? "" : "/") + to_join;
}

//_____________________________________________________________________________
static bool item_is_dir( const TString& itempath )
{
  FileStat_t buf;
  Int_t ret = gSystem->GetPathInfo(itempath, buf);
  assert(ret == 0); // 'itempath' definitely exists at this point
  return ( ret == 0 && R_ISDIR(buf.fMode)
           && !gSystem->AccessPathName(itempath, kReadPermission)
           && !gSystem->AccessPathName(itempath, kExecutePermission) );
};

//_____________________________________________________________________________
static bool item_is_not_dir( const TString& itempath )
{
  FileStat_t buf;
  Int_t ret = gSystem->GetPathInfo(itempath, buf);
  assert(ret == 0); // 'itempath' definitely exists at this point
  return ( ret == 0 && !R_ISDIR(buf.fMode) );
};

//_____________________________________________________________________________
static Int_t ForEachMatchItemInDir(
  const TString& dir, const TRegexp& match_re,
  const std::function<Int_t( const TString&, const TString& )>& action )
{
  auto* dirp = gSystem->OpenDirectory(dir);
  assert( dirp ); // else item_is_dir() faulty
  if( !dirp ) {
    cerr << "Directory " << dir << " unexpectedly cannot be opened." << endl;
    return THaRunBase::READ_ERROR;
  }
  while( TString item = gSystem->GetDirEntry(dirp) ) {
    if( item.IsNull() )
      break;
    if( item == "." || item == ".." )
      continue;
    Ssiz_t len = 0;
    if( match_re.Index(item, &len) == 0 && len == item.Length() ) {
       Int_t ret = action(dir, item);
       if( ret ) {
         gSystem->FreeDirectory(dirp);
         return ret;
       }
    }
  }
  gSystem->FreeDirectory(dirp);
  return THaRunBase::READ_OK;
}

//_____________________________________________________________________________
Int_t MultiFileRun::ScanForFilename
  ( const TString& path, const TString& pattern, bool regex_mode )
{
  auto add_file = [this]( const TString& curpath, const TString& file ) -> Int_t
  {
    auto itempath = JoinPath(curpath, file);
    if( item_is_not_dir(itempath) ) {
      return AddFile(file, curpath);
    } else {
      cerr << itempath << " is a directory. Expected file." << endl;
    }
    return READ_OK;
  };

  if( HasWildcards(pattern.Data()) ) {
    TRegexp file_re(
      gSystem->BaseName(pattern), !regex_mode);
    if( file_re.Status() != TRegexp::kOK ) {
      cerr << "Bad filename pattern \"" << pattern << "\", err = "
           << file_re.Status() << endl;
      return READ_ERROR;
    }
    return ForEachMatchItemInDir(path, file_re, add_file);

  } else {
    return add_file(path, pattern);
  }
}

//_____________________________________________________________________________
Int_t MultiFileRun::ScanForSubdirs   // NOLINT(misc-no-recursion)
  ( const TString& path, const std::vector<TString>& splitpath, Int_t level,
    bool regex_mode )
{
  auto do_directory = [this, level, &splitpath]  // NOLINT(misc-no-recursion)
    ( const TString& curpath, const TString& subdir ) -> Int_t
  {
    auto itempath = JoinPath(curpath, subdir);
    if( item_is_dir(itempath) ) {
      return DescendInto(itempath, splitpath, level + 1);
    } else {
      cerr << itempath << " is not a directory" << endl;
    }
    return READ_OK;
  };

  const auto& subdir = splitpath[level];
  if( HasWildcards(subdir.Data()) ) {
    TRegexp subdir_re(subdir, !regex_mode);
    if( subdir_re.Status() != TRegexp::kOK ) {
      cerr << "Bad directory pattern \"" << subdir << "\", err = "
           << subdir_re.Status() << endl;
      return READ_ERROR;
    }
    return ForEachMatchItemInDir(path, subdir_re, do_directory);
  } else {
    return do_directory(path, subdir);
  }
}

//_____________________________________________________________________________
Int_t MultiFileRun::DescendInto  // NOLINT(misc-no-recursion)
  ( const TString& curpath, const std::vector<TString>& splitpath, Int_t level )
{
  if( level >= SSIZE(splitpath) ) {
    // Base case: Lowest directory level. Look for the filename pattern.
    return ScanForFilename(curpath, fFilenamePattern, fNameIsRegexp);
  }
  return ScanForSubdirs(curpath, splitpath, level, fNameIsRegexp);
}

//_____________________________________________________________________________
bool MultiFileRun::HasWildcards( const string& path ) const
{
  // Return true if 'path' should be interpreted as a regular expression.
  // This searches for the first occurrence of a character that will be treated
  // specially, so that 'path' needs to be "compiled" by TRegexp.

  // TODO this is hard to test (too many possible cases) and so this function
  //  may still be buggy for edge cases.

  if( !fNameIsRegexp ) {
    // ROOT's TRegexp with wildcard = true supports shell-style wildcards
    // including character ranges like '[a-z]', but also '[a-z]+'.
    return path.find_first_of("?*[") != string::npos;
  }
  // Handle full (= non-wildcard) regular expressions.
  // These characters immediately give away the string as a regexp:
  auto is_regex_char = []( string::value_type c ) {
    return (c == '.' || c == '[' ||
            c == '*' || c == '+' || c == '?');
  };
  string::size_type pos = 0;
  while( true ) {
    pos = path.find_first_of("^$.[*+?\\", pos);
    if( pos == string::npos )
      return false;
    auto c = path[pos];
    if( is_regex_char(c) )
      return true;
    // ROOT TRegexp treats '^' and '$' as normal characters except at the start
    // and end of the expression, respectively. So, except at those positions,
    // these characters alone do not make 'path' a regexp. (They make path an
    // invalid directory name, but that is checked elsewhere.)
    if( (c == '^' && pos == 0) || (c == '$' && pos + 1 == path.length()) )
      return true;
    // A '\' requires regex treatment only if it precedes one of the special
    // regex characters. Otherwise, C-string interpretation applies, e.g. "\t"
    if( c == '\\' && pos + 1 < path.length()
        && is_regex_char(path[pos + 1]) )
      return true;
    ++pos;
  }
}

//_____________________________________________________________________________
// Build the list of input files
Int_t MultiFileRun::BuildInputList()
{
  if( fPathList.empty() )
    fPathList.emplace_back(gSystem->GetDirName(fFilenamePattern.c_str()));

  for( const auto& dir: fPathList ) {
    assert(!dir.empty());
    if( HasWildcards(dir) ) {
      // Traverse directory tree, resolving wildcard/regexp in directory names
      auto dircomp = SplitPath(dir);
      assert(!dircomp.empty());
      const auto topdir = dircomp[0];
      Int_t ret = DescendInto(topdir, dircomp, 1);
      if( ret ) return ret;
    } else {
      // If the directory path contains no wildcards/regexp, simply scan
      // that directory directly
      Int_t ret = ScanForFilename(dir, fFilenamePattern, fNameIsRegexp);
      if( ret ) return ret;
    }
  }
  if( fStreams.empty() ) {
    cerr << "MultiFileRun::Open: no matching files found" << endl;
    return READ_ERROR;
  }

  SortStreams();

  Int_t ret = CheckFilesConsistency();

  return ret;
}

//_____________________________________________________________________________
Int_t MultiFileRun::Open()
{
  // Prepare for reading multiple files (segments, streams)

  if( IsOpen() )
    return READ_OK;

  if( fFilenamePattern.empty() ) {
    cerr << "CODA file name not set. Cannot open the run." << endl;
    return READ_FATAL;  // filename not set
  }
  // Find existing files matching the given pattern
  if( fStreams.empty() && !FindSegmentNumber() ) {
    return READ_FATAL;
  }

  // Now actually open the file(s). Streams will be read in parallel,
  // and so one file of each stream is opened simultaneously.
  for( auto& stream: fStreams ) {
    stream.fVersion = fDataVersion;
#ifndef NDEBUG
    Int_t ret =
#endif
      stream.Open();
    assert(ret == CODA_OK);  // else checks in BuildInputList faulty
  }
  fLastUsedStream = 0;
  FindSegmentNumber();
  fFilename = fStreams[fLastUsedStream].GetFilename();
  fNActive = SSIZE(fStreams);
  fOpened = true;
  return READ_OK;
}

//_____________________________________________________________________________
Int_t MultiFileRun::ReadEvent() // NOLINT(misc-no-recursion)
{
  if( fCodaData ) {
    // This is set only during ReadInitInfo when prescanning a separate file
    assert(fCodaData->isOpen());
    return ReturnCode(fCodaData->codaRead());
  }

  if( !IsOpen() ) {
    cerr << "Not open" << endl;
    return READ_ERROR;
  }
  if( fNActive <= 0 )
    return READ_EOF;

  // If multiple streams are being read, find the smallest event number of
  // all open streams so that GetEvBuffer knows what to return next
  fLastUsedStream = FindNextStream();
#ifndef NDEBUG
  Bool_t success =
#endif
    FindSegmentNumber();
  assert(success);
  fFilename = fStreams[fLastUsedStream].GetFilename();

  auto& curstr = fStreams[fLastUsedStream];
  Int_t st = curstr.Read();
  if( st == CODA_EOF ) {     // no more data
    curstr.fActive = false;
    if( --fNActive > 0 )
      return ReadEvent();    // advance to next active stream
    assert(fNActive == 0);
  }
  if( st != CODA_OK )
    return ReturnCode(st);

  ++fNevRead;
  return st;
}

//_____________________________________________________________________________
// Check which of the currently active streams has the lowest physics event
// number and return its index. If multiple streams match, match the one with
// the lowest stream number.
Int_t MultiFileRun::FindNextStream() const
{
  assert(fNActive >= 1);
  Int_t ret = fLastUsedStream;
  auto sz = SSIZE(fStreams);
  if( fNActive > 1 ) {
    Int_t minidx = -1;
    UInt_t minev = kMaxUInt;
    for( Int_t i = 0; i < sz; ++i ) {
      const auto& stream = fStreams[i];
      if( stream.fActive && stream.fEvNum < minev ) {
        minidx = i;
        minev = stream.fEvNum;
      }
    }
    assert(minidx != -1);
    ret = minidx;
  } else {
#ifndef NDEBUG
    bool found = false;
#endif
    for( int i = 0; i < sz; ++i ) {
      if( fStreams[i].fActive ) {
        ret = i;
#ifndef NDEBUG
        assert(!found);   // else fNActive incorrect
        found = true;
#else
        break;
#endif
      }
    }
#ifndef NDEBUG
    assert(found);        // else fNActive incorrect
#endif
  }
  return ret;
}

//_____________________________________________________________________________
void MultiFileRun::Print( Option_t* opt ) const
{
  THaPrintOption sopt(opt);
  sopt.ToUpper();
  if( sopt.Contains("NAMEDESC") ) {
    cout << "\"file://" << fFilenamePattern << "\"";
    if( strcmp(GetTitle(), "") != 0 )
      cout << "  \"" << GetTitle() << "\"";
    return;
  }
  THaCodaRun::Print(opt); // NOLINT(bugprone-parent-virtual-call)
  if( fPathList.size() > 1
      || (!fPathList.empty()
          && fPathList.front() !=
             fFilenamePattern.substr(0, fPathList.front().length())) ) {
    cout << "Search path";
    if( fPathList.size() > 1 )
      cout << "s:" << endl;
    else
      cout << ":";
    for( const auto& path: fPathList ) {
      cout << "  " << path << endl;
    }
  }
  cout << "File name";
  if( HasWildcards(fFilenamePattern) )
    cout << (fNameIsRegexp ? " (regexp): " : " (wildcards): ");
  else
    cout << ": ";
  cout << fFilenamePattern << endl;
  if( fFirstSegment != 0 )
    cout << "First segment:         " << fFirstSegment << endl;
  if( fMaxSegments != 0 )
    cout << "Max segments:          " << fMaxSegments << endl;
  if( fFirstStream != 0 )
    cout << "First stream:          " << fFirstStream << endl;
  if( fMaxStreams != 0 )
    cout << "Max streams:           " << fMaxStreams << endl;
  PrintStreamInfo();
}

//_____________________________________________________________________________
void MultiFileRun::PrintStreamInfo() const
{
  for( const auto& stream: fStreams ) {
    if( stream.fID >= 0 )
      cout << "Stream " << stream.fID;
    else
      cout << "Default stream";
    cout << ": " << endl;
    for( const auto& file: stream.fFiles ) {
      cout << "  " << file.fPath << endl;
    }
  }
}

//_____________________________________________________________________________
void MultiFileRun::ClearStreams()
{
  fLastUsedStream = -1;
  fStreams.clear();
  fIsInit = false;
}

//_____________________________________________________________________________
Int_t MultiFileRun::SetFilename( const char* name )
{
  // Set the file name pattern for the input file(s).
  // If the name changes, the existing input, if any, will be closed.
  // Return -1 if illegal name, 1 if name not changed, 0 otherwise.

  static const char* const here = "MultiFileRun::SetFilename";

  if( !name || !*name ) {
    Error( here, "Illegal file name." );
    return -1;
  }

  if( fFilenamePattern == name )
    return 1;

  Close();
  fFilenamePattern = name;

  // Assume we have to reinitialize. A new file name generally means a new
  // run date, run number, etc.
  ClearStreams();

  CheckWarnAbsFilename();

  return 0;
}

//_____________________________________________________________________________
bool MultiFileRun::SetPathList( std::vector<std::string> pathlist )
{
  fPathList = std::move(pathlist);
  return CheckWarnAbsFilename();
}

//_____________________________________________________________________________
void MultiFileRun::SetFirstSegment( Int_t n )
{
  if( n < 0 ) n = 0;
  fFirstSegment = n;
  ClearStreams();
}

//_____________________________________________________________________________
void MultiFileRun::SetFirstStream( Int_t n )
{
  if( n < 0 ) n = 0;
  fFirstStream = n;
  ClearStreams();
}

//_____________________________________________________________________________
void MultiFileRun::SetMaxSegments( Int_t n )
{
  if( n < 0 ) n = 0;
  fMaxSegments = n;
  ClearStreams();
}

//_____________________________________________________________________________
void MultiFileRun::SetMaxStreams( Int_t n )
{
  if( n < 0 ) n = 0;
  fMaxStreams = n;
  ClearStreams();
}

//_____________________________________________________________________________
// Set fSegment and fStream to those of the current input.
// The value of -1 for either variable means that this info is not specified
// (i.e. the filename has no stream or segment number).
//
// If necessary, this function builds the list of input files.
//
// Returns true if successful, false otherwise.
Bool_t MultiFileRun::FindSegmentNumber()
{
  if( fStreams.empty() ) {
    Int_t ret = BuildInputList();
    if( ret != READ_OK || fStreams.empty() )
      return false;
    fLastUsedStream = 0;
  }
  assert(fLastUsedStream >= 0 && fLastUsedStream < SSIZE(fStreams));
  const auto& curstr = fStreams[fLastUsedStream];
  assert(curstr.fFileIndex < SSIZE(curstr.fFiles));
  fSegment = curstr.fFiles[curstr.fFileIndex].fSegment;
  fStream = curstr.fID;
  return true;
}

//_____________________________________________________________________________
bool MultiFileRun::CheckWarnAbsFilename()
{
  if( fFilenamePattern.length() > 0 && fFilenamePattern.front() == '/'
      && !fPathList.empty() ) {
    cerr << "MultiFileRun: Warning: file name is an absolute path, but "
            "path list also specified. Ignoring path list." << endl;
    fPathList.clear();
    return true;
  }
  return false;
}

//_____________________________________________________________________________
void MultiFileRun::ExpandFileName( string& str ) const
{
  TString path = str;
  if( fNameIsRegexp && path.EndsWith("$") )
    path.Chop();

  if( gSystem->ExpandPathName(path) ) {
    string s = "MultiFileRun: Undefined environment variable in path \"";
    s.append(str).append("\"");
    throw std::invalid_argument(s);
  }
  str = path.Data();
}

//_____________________________________________________________________________
UInt_t MultiFileRun::GetNFiles() const
{
  size_t nfiles = 0;
  for( const auto& stream: fStreams ) {
    nfiles += stream.fFiles.size();
  }
  return nfiles;
}

//_____________________________________________________________________________
UInt_t MultiFileRun::GetNStreams() const
{
  return fStreams.size();
}

//_____________________________________________________________________________
Int_t MultiFileRun::GetStartSegment() const
{
  Int_t minseg = kMaxInt;
  for( const auto& stream: fStreams ) {
    for( const auto& file: stream.fFiles ) {
      if( file.fSegment < minseg )
        minseg = file.fSegment;
    }
  }
  return minseg;
}

//_____________________________________________________________________________
Int_t MultiFileRun::GetLastSegment() const
{
  Int_t maxseg = kMinInt;
  for( const auto& stream: fStreams ) {
    for( const auto& file: stream.fFiles ) {
      if( file.fSegment > maxseg )
        maxseg = file.fSegment;
    }
  }
  return maxseg;
}

//_____________________________________________________________________________
Int_t MultiFileRun::GetStartStream() const
{
  Int_t minstr = kMaxInt;
  for( const auto& stream: fStreams ) {
    if( stream.fID < minstr )
      minstr = stream.fID;
  }
  return minstr;
}

//_____________________________________________________________________________
Int_t MultiFileRun::GetLastStream() const
{
  Int_t maxstr = kMinInt;
  for( const auto& stream: fStreams ) {
    if( stream.fID > maxstr )
      maxstr = stream.fID;
  }
  return maxstr;
}

//_____________________________________________________________________________
vector<string> MultiFileRun::GetFiles() const
{
  vector<string> files;
  for( const auto& stream: fStreams ) {
    for( const auto& file: stream.fFiles ) {
      files.push_back(file.fPath);
    }
  }
  return files;
}

//_____________________________________________________________________________
MultiFileRun::FileInfo::FileInfo( std::string path, std::string stem, Int_t seg )
  : fPath{std::move(path)}
  , fStem{std::move(stem)}
  , fSegment{seg}
{}

//_____________________________________________________________________________
MultiFileRun::StreamInfo::StreamInfo()
  : fCodaData{MKCODAFILE}
  , fID{-1}
  , fVersion{0}
  , fFileIndex{0}
  , fEvNum{0}
  , fActive{false}
{}

//_____________________________________________________________________________
MultiFileRun::StreamInfo::StreamInfo( Int_t id )
  : fCodaData{MKCODAFILE}
  , fID{id}
  , fVersion{0}
  , fFileIndex{0}
  , fEvNum{0}
  , fActive{false}
{}

//_____________________________________________________________________________
MultiFileRun::StreamInfo::StreamInfo( const MultiFileRun::StreamInfo& rhs )
  : fCodaData{MKCODAFILE}
  , fFiles{rhs.fFiles}
  , fID{rhs.fID}
  , fVersion{rhs.fVersion}
  , fFileIndex{rhs.fFileIndex}
  , fEvNum{rhs.fEvNum}
  , fActive{rhs.fActive}
{}

//_____________________________________________________________________________
MultiFileRun::StreamInfo&
MultiFileRun::StreamInfo::operator=( const MultiFileRun::StreamInfo& rhs )
{
  if( this != &rhs ) {
    fCodaData = MKCODAFILE;
    fFiles = rhs.fFiles;
    fID = rhs.fID;
    fVersion = rhs.fVersion;
    fFileIndex = rhs.fFileIndex;
    fEvNum = rhs.fEvNum;
    fActive = rhs.fActive;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t MultiFileRun::StreamInfo::Open()
{
  assert(fCodaData);
  if( fCodaData->isOpen() )
    return CODA_OK;
  fFileIndex = 0;
  fActive = true;
  return OpenCurrent();
}

//_____________________________________________________________________________
Int_t MultiFileRun::StreamInfo::OpenCurrent()
{
  assert(fCodaData);
  if( fCodaData->isOpen() )
    fCodaData->codaClose();
  auto& fn = fFiles[fFileIndex].fPath;
  return fCodaData->codaOpen(fn.c_str());
}

//_____________________________________________________________________________
Int_t MultiFileRun::StreamInfo::Read()
{
  assert(fCodaData);
  if( !fCodaData->isOpen() ) {
    cerr << "Not open" << endl;
    return CODA_ERROR;
  }
  if( !fActive )
    return CODA_EOF;

  Int_t st = CODA_OK;
  while( IsGood() ) {
    st = fCodaData->codaRead();
    if( st == CODA_OK ) {
      st = FetchEventNumber();
      break;
    }
    if( st == CODA_EOF ) {
      if( ++fFileIndex >= SSIZE(fFiles) ) {
        fCodaData->codaClose();
        return CODA_EOF;
      }
      cout << "MultiFileRun::Read: Switching to next segment idx = "
           << fFileIndex << ", file = \"" << fFiles[fFileIndex].fPath << "\""
           << endl;
      st = OpenCurrent();
    }
  }
  return st;
}

//_____________________________________________________________________________
Int_t MultiFileRun::StreamInfo::Close()
{
  assert(fCodaData);
  fEvNum = 0;
  fFileIndex = 0;
  fActive = false;
  return fCodaData->codaClose();
}

//_____________________________________________________________________________
Bool_t MultiFileRun::StreamInfo::IsGood() const
{
  assert(fCodaData);
  return fCodaData->isGood();
}

//_____________________________________________________________________________
const UInt_t* MultiFileRun::StreamInfo::GetEvBuffer() const
{
  assert(fCodaData);
  assert(fActive);
  if( !fCodaData || !fActive )
    return nullptr; // if not active, the buffer does not contain valid data
  return fCodaData->getEvBuffer();
}

//_____________________________________________________________________________
// Extract even number from CODA 2 format data.
// 'evbuf' must hold a physics event.
static Int_t GetEvNumV2( const UInt_t* evbuf, UInt_t& num )
{
  auto evlen = evbuf[0] + 1;
  if( evlen < 5 ) {
    cerr << "Physics event too short, len = " << evlen << ", need >= 5"
         << endl;
    return CODA_ERROR;
  }
  if( evbuf[4] <= num )
    cerr << "MultiFileRun::FetchEventNumber: Warning: unexpected "
         << "event number change " << num << " -> " << evbuf[4]
         << endl;
  num = evbuf[4];

  return CODA_OK;
}

//_____________________________________________________________________________
// Extract even number from CODA 3 format data.
// 'evbuf' must hold a physics event.
static Int_t GetEvNumV3( const UInt_t* evbuf, UInt_t& num )
{
  auto block_size = evbuf[1] & 0xff;
  if( block_size == 0 ) {
    cerr << "CODA 3 format error: Physics event with block size 0" << endl;
    return CODA_ERROR;
  }
  // The physics event number that we're after is stored in the trigger bank
  CodaDecoder::TBOBJ tbank;
  try {
    tbank.Fill(evbuf + 2, block_size,
      /* The TSROC number does not matter as we are not using
         its data */21);
  }
  catch( const CodaDecoder::coda_format_error& e ) {
    cerr << "CODA 3 format error: " << e.what() << endl;
    return CODA_ERROR;
  }
  if( tbank.evtNum <= num )
    cerr << "MultiFileRun::FetchEventNumber: Warning: unexpected "
         << "event number change " << num << " -> " << tbank.evtNum
         << endl;
  num = tbank.evtNum;

  return CODA_OK;
}

//_____________________________________________________________________________
// Test if the current event is a physics event. If so, extract its event
// number. This requires some CODA-specific low-level decoding.
Int_t MultiFileRun::StreamInfo::FetchEventNumber()
{
  assert(fCodaData && fCodaData->isOpen());
  auto buflen = fCodaData->getBuffSize();
  if( buflen == 0 ) {
    cerr << "Invalid event buffer size = 0" << endl;
    return CODA_ERROR;
  }
  const auto* evbuf = fCodaData->getEvBuffer();
  auto evlen = evbuf[0] + 1;
  if( evlen > buflen ) {
    cerr << "Invalid event length " << evlen
         << " > buffer size " << buflen << endl;
    return CODA_ERROR;
  }
  if( evlen < 2 ) {
    cerr << "Event too short, len = " << evlen << ", need >= 2" << endl;
    return CODA_ERROR;
  }
  auto tag = evbuf[1] >> 16;
  if( fVersion == 2 ) {
    auto evtyp = tag;
    if( evtyp <= MAX_PHYS_EVTYPE ) {
      Int_t ret = GetEvNumV2(evbuf, fEvNum);
      if( ret )
        return ret;
    }
  }
  else if( fVersion == 3 ) {
    auto evtyp = CodaDecoder::InterpretBankTag(tag);
    if( evtyp == 1 ) {  // CODA 3 physics event
      Int_t ret = GetEvNumV3(evbuf, fEvNum);
      if( ret )
        return ret;
    }
  }
  else {
    cerr << "Unsupported CODA version " << fVersion << endl;
    return CODA_ERROR;
  }
  return CODA_OK;
}

//_____________________________________________________________________________
const std::string& MultiFileRun::StreamInfo::GetFilename() const
{
  static const string nullstr;
  if( fFiles.empty() )
    return nullstr;
  assert(fFileIndex >= 0 && fFileIndex < SSIZE(fFiles));
  return fFiles[fFileIndex].fPath;
}

//_____________________________________________________________________________

} //namespace Podd

ClassImp(Podd::MultiFileRun::StreamInfo)
ClassImp(Podd::MultiFileRun::FileInfo)
ClassImp(Podd::MultiFileRun)
