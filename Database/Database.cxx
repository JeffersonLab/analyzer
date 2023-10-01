//*-- Author :    Ole Hansen   1-Jun-2021

//////////////////////////////////////////////////////////////////////////
//
// Podd
//
// Database support functions.
// These functions were previously part of the THaAnalysisObject class.
//
//////////////////////////////////////////////////////////////////////////

#include "Database.h"
#include "TDatime.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TString.h"
#include "TError.h"
#include "TSystem.h"

#include <cerrno>
#include <cctype>    // for isspace
#include <cstring>
#include <cstdlib>   // for atoi, strtod, strtol etc.
#include <iterator>  // for std::distance
#include <ctime>     // for struct tm
#include <limits>
#include <algorithm>
#include <type_traits>
#include <iostream>

// This is a well-known problem with strerror_r
#if defined(__linux__) && (defined(_GNU_SOURCE) || !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE > 600))
# define GNU_STRERROR_R
#endif

using namespace std;

//_____________________________________________________________________________
const char* Here( const char* method, const char* prefix )
{
  // Utility function for error messages. The return value points to a static
  // string buffer that is unique to the current thread.
  // There are two usage cases:
  // ::Here("method","prefix")        -> returns ("prefix")::method
  // ::Here("Class::method","prefix") -> returns Class("prefix")::method

  // One static string buffer per thread
  static thread_local TString buffer;

  TString txt;
  if( prefix && *prefix ) {
    TString full_prefix(prefix);
    // delete trailing dot of prefix, if any
    if( full_prefix.EndsWith(".") )
      full_prefix.Chop();
    full_prefix.Prepend("(\"");
    full_prefix.Append("\")");
    const char* scope = nullptr;
    if( method && *method && (scope = strstr(method, "::")) ) {
      assert(scope >= method);
      auto pos = static_cast<Ssiz_t>(std::distance(method, scope));
      txt = method;
      assert(pos >= 0 && pos < txt.Length());
      txt.Insert(pos, full_prefix);
      method = nullptr;
    } else {
      txt = full_prefix + "::";
    }
  }
  if( method )
    txt.Append(method);

  buffer = txt;

  return buffer.Data(); // pointer to the C-string of a static TString
}

//=============================================================================
namespace Podd {

//_____________________________________________________________________________
TString& GetObjArrayString( const TObjArray* array, Int_t i )
{
  // Get the string at index i in the given TObjArray

  return (static_cast<TObjString*>(array->At(i)))->String();
}

//_____________________________________________________________________________
vector<string> GetDBFileList( const char* name, const TDatime& date,
                              const char* here )
{
  // Return the database file searchlist as a vector of strings.
  // The file names are relative to the current directory.

  static const string defaultdir = "DEFAULT";
  static const string dirsep = "/", allsep = "/";

  vector<string> fnames;
  if( !name || !*name )
    return fnames;

  // If name contains a directory separator, we take the name verbatim
  string filename = name;
  if( filename.find_first_of(allsep) != string::npos ) {
    fnames.push_back(filename);
    return fnames;
  }

  // Build search list of directories
  vector<string> dnames;
  if( const char* dbdir = gSystem->Getenv("DB_DIR") )
    dnames.emplace_back(dbdir);
  dnames.emplace_back("DB");
  dnames.emplace_back("db");
  dnames.emplace_back(".");

  // Try to open the database directories in the search list.
  // The first directory that can be opened is taken as the database
  // directory. Subsequent directories are ignored.
  auto it = dnames.begin();
  void* dirp = nullptr;
  while( !(dirp = gSystem->OpenDirectory((*it).c_str())) &&
         (++it != dnames.end()) ) {}

  // None of the directories can be opened?
  if( it == dnames.end() ) {
    ::Error(here, "Cannot open any database directories. Check your disk!");
    return fnames;
  }

  // Pointer to database directory string
  string thedir = *it;

  // In the database directory, get the names of all subdirectories matching
  // a YYYYMMDD pattern.
  vector<string> time_dirs;
  bool have_defaultdir = false;
  while( const char* result = gSystem->GetDirEntry(dirp) ) {
    string item = result;
    if( item.length() == 8 ) {
      Int_t pos = 0;
      for( ; pos < 8; ++pos )
        if( !isdigit(item[pos]) ) break;
      if( pos == 8 )
        time_dirs.push_back(item);
    } else if( item == defaultdir )
      have_defaultdir = true;
  }
  gSystem->FreeDirectory(dirp);

  // Search a date-coded subdirectory that corresponds to the requested date.
  bool found = false;
  if( !time_dirs.empty() ) {
    sort(time_dirs.begin(), time_dirs.end());
    for( it = time_dirs.begin(); it != time_dirs.end(); ++it ) {
      Int_t item_date = atoi((*it).c_str());
      if( it == time_dirs.begin() && date.GetDate() < item_date )
        break;
      if( it != time_dirs.begin() && date.GetDate() < item_date ) {
        --it;
        found = true;
        break;
      }
      // Assume that the last directory is valid until infinity.
      if( it + 1 == time_dirs.end() && date.GetDate() >= item_date ) {
        found = true;
        break;
      }
    }
  }

  // Construct the database file name. It is of the form db_<prefix>.dat.
  // Subdetectors use the same files as their parent detectors!
  // If filename does not start with "db_", make it so
  if( filename.substr(0, 3) != "db_" )
    filename.insert(0, "db_");
    // If filename does not end with ".dat", make it so
#ifndef NDEBUG
  // should never happen
  assert(filename.length() >= 4);
#else
  if( filename.length() < 4 ) { fnames.clear(); return fnames; }
#endif
  if( *filename.rbegin() == '.' ) {
    filename += "dat";
  } else if( filename.substr(filename.length() - 4) != ".dat" ) {
    filename += ".dat";
  }

  // Build the searchlist of file names in the order:
  // ./filename <dbdir>/<date-dir>/filename
  //    <dbdir>/DEFAULT/filename <dbdir>/filename
  fnames.push_back(filename);
  if( found ) {
    string item = thedir + dirsep + *it + dirsep + filename;
    fnames.push_back(item);
  }
  if( have_defaultdir ) {
    string item = thedir + dirsep + defaultdir + dirsep + filename;
    fnames.push_back(item);
  }
  fnames.push_back(thedir + dirsep + filename);

  return fnames;
}

//_____________________________________________________________________________
FILE* OpenDBFile( const char* name, const TDatime& date, const char* here,
                  const char* filemode, int debug_flag, string& openpath )
{
  // Open database file and return a pointer to the C-style file descriptor.
  // openpath contains the absolute path to the opened file.

  // Ensure input is sane
  openpath.clear();
  if( !name || !*name )
    return nullptr;
  if( !here )
    here = "";
  if( !filemode )
    filemode = "r";

  const bool verbose = (debug_flag > 0);
  const bool detailed = (debug_flag > 1);

  // Get list of database file candidates and try to open them in turn
  FILE* fi = nullptr;
  vector<string> fnames(GetDBFileList(name, date, here));
  for( auto& fpath : fnames ) {
    if( detailed )
      cout << "Info in <" << here << ">: Opening database file " << fpath;

    fi = fopen(fpath.c_str(), filemode);
    if( fi ) {
      if( detailed )
	cout << " ... success" << endl;
      else if( verbose )
	::Info(here, "Opened database file %s", fpath.c_str());

      if( gSystem->IsAbsoluteFileName(fpath.c_str()) )
	openpath = std::move(fpath);
      else {
	const char* wd = gSystem->WorkingDirectory();
	if( wd && *wd ) {
	  openpath = wd;
	  openpath += "/";
	}
	openpath += fpath;
      }
      break;
    }
    else if( detailed )
      cout << " ... failed" << endl;

    // continue until we succeed
  }
  if( !fi && verbose ) {
    ::Error(here, "Cannot open database file db_%s%sdat", name,
            (name[strlen(name) - 1] == '.' ? "" : "."));
  }
  return fi;
}

//_____________________________________________________________________________
// FIXME: Poorly designed. To be removed. Do not use.
FILE* OpenDBFile( const char* name, const TDatime& date, const char* here,
		  const char* filemode, int debug_flag, const char*& openpath )
{
  string temp;
  FILE* fi = OpenDBFile(name, date, here, filemode, debug_flag, temp);
  // Caller must free() openpath.
  openpath = fi ? strdup(temp.c_str()) : nullptr;
  return fi;
}

//_____________________________________________________________________________
FILE* OpenDBFile( const char* name, const TDatime& date, const char* here,
                  const char* filemode, int debug_flag )
{
  string dummy;
  return OpenDBFile(name, date, here, filemode, debug_flag, dummy);
}

//---------- Database utility functions ---------------------------------------

static thread_local string errtxt;
static thread_local int loaddb_depth = 0; // Recursion depth in LoadDB
static thread_local string loaddb_prefix; // Actual prefix of object in LoadDB (for err msg)

// Local helper functions
namespace {
//_____________________________________________________________________________
Int_t IsDBdate( const string& line, TDatime& date, bool warn = true )
{
  // Check if 'line' contains a valid database time stamp. If so,
  // parse the line, set 'date' to the extracted time stamp, and return 1.
  // Else return 0;
  // Time stamps must be in SQL format: [ yyyy-mm-dd hh:mi:ss ]

  auto lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size() - 12 ) return 0;
  auto rbrk = line.find(']', lbrk);
  if( rbrk == string::npos || rbrk <= lbrk + 11 ) return 0;
  string ts = line.substr(lbrk+1,rbrk-lbrk-1);

  bool err = false;
  struct tm tmc{};
  const char* c = strptime(ts.c_str(), " %Y-%m-%d %H:%M:%S %z ", &tmc);
  if( !c || c != ts.c_str()+ts.size() ) {
    // No time zone field: try again without that field.
    // We allow this for compatibility with old databases.
    c = strptime(ts.c_str(), " %Y-%m-%d %H:%M:%S ", &tmc);
    if( !c || c != ts.c_str()+ts.size() )
      err = true;
    // If this succeeds, assume the value represents local time.
    // This may cause errors if the database and the local time zone differ!
  }
#ifdef __GLIBC__
  else {
    // Time zone offset explicitly given. If using glibc, strptime does not
    // apply the time zone offset to the result, so we need to explicitly
    // convert it to localtime, which is what TDatime wants.
    // Get localtime GMT offset at the specified date
    struct tm tml = tmc;
    tml.tm_isdst = -1;
    time_t tval = mktime(&tml);
    if( tval != -1 ) {
      long tzdiff = tml.tm_gmtoff - tmc.tm_gmtoff;
      if( tzdiff != 0 ) {
        tmc.tm_hour += (int) (tzdiff / 3600);
        tmc.tm_min += (int) ((tzdiff % 3600) / 60);
        tmc.tm_isdst = tml.tm_isdst;
        tval = mktime(&tmc);  // Normalizes tmc, converts to localtime
        if( tval == -1 )
          err = true;
      }
    } else {
      err = true;
    }
  }
#endif
  if( err || tmc.tm_year < 95 ) {
    if( warn )
      ::Warning("IsDBdate()", "Invalid date tag %s", line.c_str());
    return 0;
  }
  date.Set(tmc.tm_year + 1900, tmc.tm_mon + 1, tmc.tm_mday,
           tmc.tm_hour, tmc.tm_min, tmc.tm_sec);
  errno = 0;  // strptime() may set errno to spurious values despite succeeding
  return 1;
}

//_____________________________________________________________________________
Int_t IsDBkey( const string& line, const char* key, string& text )
{
  // Check if 'line' is of the form "key = value" and, if so, whether the key
  // equals 'key'. Keys are case-sensitive.
  // - If there is no '=', then return 0.
  // - If there is a '=', but the left-hand side doesn't match 'key',
  //   then return -1.
  // - If key found, parse the line, set 'text' to the whitespace-trimmed
  //   text after the "=" and return +1.
  // 'text' is not changed unless a valid key is found.
  //
  // Note: By construction in ReadDBline, 'line' is not empty, any comments
  // starting with '#' have been removed, and trailing whitespace has been
  // trimmed. Also, all tabs have been converted to spaces.

  // Search for "="
  const char* ln = line.c_str();
  const char* eq = strchr(ln, '=');
  if( !eq ) return 0;
  // Disregard "==", "!=", "<=", ">="
  if( (eq > ln && (*(eq-1) == '!' || *(eq-1) == '<' || *(eq-1) == '>')) || *(eq+1) == '=' )
    return 0;
  // Extract the key
  while( *ln == ' ' || *ln == '\t' ) ++ln; // find_first_not_of(" \t")
  assert(ln <= eq);
  if( ln == eq ) return -1;
  const char* p = eq - 1;
  assert(p >= ln);
  while( *p == ' ' || *p == '\t' ) --p; // find_last_not_of(" \t")
  size_t keylen = p - ln + 1;
  if( keylen != strlen(key) || strncmp(ln, key, keylen) != 0 ) return -1;
  // Key matches. Now extract the value, trimming leading whitespace.
  ln = eq + 1;
  assert(!*ln || (*(ln + strlen(ln) - 1) != ' ' &&
                  *(ln + strlen(ln) - 1) != '\t')); // Trailing space already trimmed
  while( *ln == ' ' || *ln == '\t' ) ++ln;
  text = ln;

  return 1;
}

//_____________________________________________________________________________
inline Int_t ChopPrefix( string& s )
{
  // Remove trailing level from prefix. Example "L.vdc." -> "L."
  // Return remaining number of dots, or zero if empty/invalid prefix

  auto len = s.size();
  if( len >= 2 ) {
    auto pos = s.rfind('.', len - 2);
    if( pos != string::npos ) {
      s.erase(pos + 1);
      auto ndot = std::count(s.begin(), s.end(), '.');
      if( ndot <= kMaxInt )
        return static_cast<Int_t>(ndot);
    }
  }
  s.clear();
  return 0;
}

//_____________________________________________________________________________
inline bool IsTag( const char* buf )
{
  // Return true if the string in 'buf' matches regexp ".*\[.+\].*",
  // i.e. it is a database section marker.  Generic utility function.

  const char* p = buf;
  while( *p && *p != '[' ) p++;
  if( !*p ) return false;
  p++;
  if( !*p || *p == ']' ) return false;
  p++;
  while( *p && *p != ']' ) p++;
  return (*p == ']');
}

//_____________________________________________________________________________
Int_t GetLine( FILE* file, char* buf, Int_t bufsiz, string& line )
{
  // Get a line (possibly longer than 'bufsiz') from 'file' using
  // the provided buffer 'buf'. Put result into string 'line'.
  // This is similar to std::getline, except that C-style I/O is used.
  // Also, convert all tabs to spaces.
  // Returns 0 on success, or EOF if no more data (or error).

  char* r = nullptr;
  line.clear();
  while( (r = fgets(buf, bufsiz, file)) ) {
    char* c = strchr(buf, '\n');
    if( c )
      *c = '\0';
    // Convert all tabs to spaces
    char* p = buf;
    while( (p = strchr(p, '\t')) ) *(p++) = ' ';
    // Append to string
    line.append(buf);
    // If newline was read, the line is finished
    if( c )
      break;
  }
  // Don't report EOF if we have any data
  if( !r && line.empty() )
    return EOF;
  return 0;
}

//_____________________________________________________________________________
inline Bool_t IsAssignment( const string& str )
{
  // Check if 'str' has the form of an assignment (<text> = [optional text]).
  // Properly handles comparison operators '==', '!=', '<=', '>='

  string::size_type pos = str.find('=');
  if( pos == string::npos )
    // No '='
    return false;
  const char *s = str.data(), *c = s;
  while( isspace(*c) ) ++c;
  if( c-s-pos == 0 )
    // Only whitespace before '=' or '=' at start of line
    return false;
  assert(pos > 0);
  // '!=', '<=', '>=' or '=='
  return str[pos - 1] != '!' && str[pos - 1] != '<' && str[pos - 1] != '>' &&
           (pos + 1 >= str.length() || str[pos + 1] != '=');
}

} // end anonymous namespace

//_____________________________________________________________________________
inline static
void prepare_line( string& linbuf, bool& comment, bool& continued,
                   bool& leading_space, bool& trailing_space )
{
  // Search for comment or continuation character.
  // If found, remove it and everything that follows.
  if( linbuf.empty() )
    return;
  auto pos = linbuf.find('#');
  if( pos == 0 ) {
    comment = true;
    linbuf.clear();
    return;
  } else {
    auto pos2 = linbuf.find('\\');
    pos = std::min(pos, pos2);
    if( pos != string::npos ) {
      if( pos == pos2 )
        continued = true;
      else
        comment = true;
      linbuf.erase(pos);
    }
  }
  // Trim leading and trailing space
  if( !linbuf.empty() ) {
    if( isspace(linbuf.front()) )
      leading_space = true;
    if( isspace(linbuf.back()) )
      trailing_space = true;
    if( leading_space || trailing_space )
      Trim(linbuf);
  }
}

//_____________________________________________________________________________
Int_t ReadDBline( FILE* file, char* buf, Int_t bufsiz, string& line )
{
  // Get a text line from the database file 'file'. Ignore all comments
  // (anything after a #). Trim trailing whitespace. Concatenate continuation
  // lines (ending with \).
  // Only returns if a non-empty line was found, or on EOF.

  line.clear();
  line.reserve(1023);

  Int_t r = 0;
  bool maybe_continued = false, unfinished = true;
  string linbuf; linbuf.reserve(255);
  fpos_t oldpos{};
  while( unfinished && fgetpos(file, &oldpos) == 0 &&
         (r = GetLine(file, buf, bufsiz, linbuf)) == 0 ) {
    bool continued = false, comment = false,
      trailing_space = false, leading_space = false, is_assignment = false;

    prepare_line(linbuf,comment,continued,leading_space, trailing_space);

    if( line.empty() && linbuf.empty() )
      // Nothing to do, i.e. no line building in progress and no data
      continue;

    if( !linbuf.empty() ) {
      is_assignment = IsAssignment(linbuf);
      // Tentative continuation is canceled by a subsequent line with a '='
      if( maybe_continued && is_assignment ) {
        // We must have data at this point, so we can exit. However, the line
        // we've just read is obviously a good one, so we must also rewind the
        // file to the previous position so this line can be read again.
        assert(!line.empty());  // else maybe_continued not set correctly
        fsetpos(file, &oldpos);
        break;
      }
      // If the line has data, it isn't a comment, even if there was a '#'
      //      comment = false;  // not used
    } else if( continued || comment ) {
      // Skip empty continuation lines and comments in the middle of a
      // continuation block
      continue;
    } else {
      // An empty line, except for a comment or continuation, ends continuation.
      // Since we have data here, and this line is blank and would later be
      // skipped anyway, we can simply exit
      break;
    }

    if( line.empty() && !continued && is_assignment ) {
      // If the first line of a potential result contains a '=', this
      // line may be continued by non-'=' lines up until the next blank line.
      // However, do not use this logic if the line also contains a
      // continuation mark '\'; the two continuation styles should not be mixed
      maybe_continued = true;
    }
    unfinished = (continued || maybe_continued);

    // Ensure that at least one space is preserved between continuations,
    // if originally present
    if( maybe_continued || (trailing_space && continued) )
      linbuf.push_back(' ');
    if( leading_space && !line.empty() && !isspace(line.back()))
      line.push_back(' ');

    // Append current data to result
    line.append(linbuf);
  }

  // Because of the '=' sign continuation logic, we may have hit EOF if the last
  // line of the file is a key. In this case, we need to back out.
  if( maybe_continued ) {
    if( r == EOF ) {
      fsetpos(file, &oldpos);
      r = 0;
    }
    // Also, whether we hit EOF or not, tentative continuation may have
    // added a tentative space, which we tidy up here
    assert(!line.empty());
    if( isspace(line.back()) )
      line.pop_back();
  }
  return r;
}

//_____________________________________________________________________________
Bool_t DBDatesDiffer( const TDatime& a, const TDatime& b )
{
  // Determine if there are any differences in the database contents for dates
  // 'a' and 'b'. If so, return true.
  // Currently simply returns whether 'a' and 'b' are different.
  // TODO: Fully implement this once database indexes are kept in memory.

  return a != b;
}

//_____________________________________________________________________________
Int_t LoadDBvalue( FILE* file, const TDatime& date, const char* key,
                   string& value )
{
  // Load a data value tagged with 'key' from the database 'file'.
  // Lines starting with "#" are ignored.
  // If 'key' is found, then the most recent value seen (based on time stamps
  // and position within the file) is returned in 'value'.
  // Values with time stamps later than 'date' are ignored.
  // This allows incremental organization of the database where
  // only changes are recorded with time stamps.
  // Return values:
  //    0: success
  //    1: key not found
  //   -1: unexpected error (errno != 0)
  // -255: bad argument

  if( !file || !key ) return -255;

  static const string here("LoadDBvalue");
  constexpr Int_t bufsiz = 256;
  unique_ptr<char[]> buf{new char[bufsiz]};
  char* const bufp = buf.get();

  TDatime keydate(950101, 0), prevdate(950101, 0);
  bool found = false, do_ignore = false;
  string dbline;
  vector<string> lines;

  errno = 0;
  errtxt.clear();
  rewind(file);
  if( errno )
    goto err;

  while( ReadDBline(file, bufp, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Replace text variables in this database line, if any. Multi-valued
    // variables are supported here, although they are only sensible on the LHS
    lines.assign(1, dbline);
    if( gHaTextvars )
      gHaTextvars->Substitute(lines);
    for( auto& line : lines ) {
      Int_t status = 0;
      if( !do_ignore && (status = IsDBkey(line, key, value)) != 0 ) {
        if( status > 0 ) {
          // Found a matching key for a newer date than before
          found = true;
          prevdate = keydate;
          // we do not set do_ignore to true here so that the _last_, not the
          // first, of multiple identical keys is evaluated.
        }
      } else if( IsDBdate(line, keydate) != 0 )
        do_ignore = (keydate > date || keydate < prevdate);
    }
  }

  if( errno ) {
err:
#ifdef GNU_STRERROR_R
    const char* ret = strerror_r(errno, bufp, bufsiz);
    errtxt = here + ": " + (ret ? ret : "unknown error " + to_string(errno));
#else
    strerror_r(errno, bufp, bufsiz);
    errtxt = here + ": " + bufp;
#endif
    return -1;
  }
  return found ? 0 : 1;
}

//_____________________________________________________________________________
static Int_t conversion_error( const char* key, const string& value )
{
  errtxt = key;
  errtxt += " = \"" + value + "\"";
  return -131;
}

// The following is not terribly elegant, but allows us to call the most
// efficient conversion function for a given type
//_____________________________________________________________________________
template<typename T,
  typename enable_if
    <is_integral<T>::value && is_signed<T>::value, bool>::type = true>
static inline
long long int convert_string( const char* p, char*& end )
{
  return strtoll(p, &end, 10);
}

//_____________________________________________________________________________
template<typename T,
  typename enable_if
    <is_integral<T>::value && is_unsigned<T>::value, bool>::type = true>
static inline
unsigned long long int convert_string( const char* p, char*& end )
{
  return strtoull(p, &end, 10);
}

//_____________________________________________________________________________
template<typename T,
  typename enable_if<is_same<T, float>::value, bool>::type = true>
static inline
T convert_string( const char* p, char*& end )
{
  return strtof(p, &end);
}

//_____________________________________________________________________________
template<typename T,
  typename enable_if<is_same<T, double>::value, bool>::type = true>
static inline
T convert_string( const char* p, char*& end )
{
  return strtod(p, &end);
}

//_____________________________________________________________________________
template<typename T,
  typename enable_if<is_same<T, long double>::value, bool>::type = true>
static inline
T convert_string( const char* p, char*& end )
{
  return strtold(p, &end);
}

//_____________________________________________________________________________
// Function to check if source value of type S will fit into target value
// of type T. T and S must either both be integer or floating point types.
// Trivial case of identical types.
template<typename T, typename S,
  typename enable_if<is_same<T,S>::value, bool>::type = true>
static inline bool is_in_range( S )
{
  return true;
}

//_____________________________________________________________________________
// Non-trivial case of different types.
// Currently only used for like-signed integers.
template<typename T, typename S,
  typename enable_if<not is_same<T,S>::value, bool>::type = true>
static inline bool is_in_range( S val )
{
  static_assert( (is_integral<T>::value && is_integral<S>::value) ||
                 (is_floating_point<T>::value && is_floating_point<S>::value),
                 "Inconsistent types");
  bool ret = (val <= numeric_limits<T>::max());
  if( ret ) {
    if( is_integral<T>::value ) {
      if( is_signed<S>::value ) {
        if( is_unsigned<T>::value )
          ret = (val >= 0);
        else
          ret = (val >= numeric_limits<T>::min());
      }
    } else if( is_floating_point<T>::value )
      ret = (val >= -numeric_limits<T>::max());
  }
  return ret;
}

//_____________________________________________________________________________
Int_t LoadDBvalue( FILE* file, const TDatime& date, const char* key,
                   TString& value )
{
  // Locate key in database, convert the text found to TString and return
  // result in 'value'.

  string _text;
  Int_t err = LoadDBvalue(file, date, key, _text);
  if( err == 0 )
    value = _text.c_str();
  return err;
}

//_____________________________________________________________________________
template<class T>
Int_t LoadDBvalue( FILE* file, const TDatime& date, const char* key, T& value )
{
  // Locate key in database, convert the text found to numerical type T,
  // and return result in 'value'.
  // Returns 0 if OK, 1 if key not found, and a negative number for error.

  static_assert(std::is_arithmetic<T>::value, "Value argument must be arithmetic");

  string text;
  if( Int_t err = LoadDBvalue(file, date, key, text) )
    return err;
  const char* p = text.c_str();
  char* end = nullptr;
  errno = 0;
  // Convert the value string depending on the requested type
  auto dval = convert_string<T>(p, end);
  if( p == end || (end && *end && !isspace(*end)) || errno != 0 ||
      !is_in_range<T>(dval) ) {
    return conversion_error(key, text);
  }
  value = static_cast<T>(dval);
  return 0;
}

//_____________________________________________________________________________
template<class T>
Int_t LoadDBarray( FILE* file, const TDatime& date, const char* key,
                   vector<T>& values )
{
  // Locate key in database, interpret the key as a whitespace-separated array
  // of arithmetic values of type T, convert each field, and return result in
  // the vector 'values'.
  // Returns 0 if OK, 1 if key not found, and a negative number for error.

  static_assert(std::is_arithmetic<T>::value, "Value argument must be arithmetic");

  string text;
  Int_t err = LoadDBvalue(file, date, key, text);
  if( err )
    return err;
  values.clear();
  // Determine number of elements to avoid resizing the vector multiple times
  size_t nelem = 0;
  bool in_field = false;
  for( char c : text ) {
    bool istxt = !isspace(c);
    if( istxt && !in_field )
      ++nelem;
    in_field = istxt;
  }
  values.reserve(nelem);
  const char* p = text.c_str(), * endp = p + text.length();
  char* end = nullptr;
  // Read the fields into the vector
  while( true ) {
    errno = 0;
    // Convert each value depending on the requested type
    auto dval = convert_string<T>(p, end);
    if( p == end || (end && *end && !isspace(*end)) || errno != 0 ||
        !is_in_range<T>(dval) ) {
      return conversion_error(key, text);
    }
    values.push_back(static_cast<T>(dval));
    if( end == endp )
      break;
    p = end;
  }
  return 0;
}

//_____________________________________________________________________________
template<class T>
Int_t LoadDBmatrix( FILE* file, const TDatime& date, const char* key,
                    vector<vector<T>>& values, UInt_t ncols )
{
  // Read a matrix of values of type T into a vector of vectors.
  // The matrix is rectangular with ncols columns.

  vector<T> tmpval;
  Int_t err = LoadDBarray(file, date, key, tmpval);
  if( err ) {
    return err;
  }
  if( (tmpval.size() % ncols) != 0 ) {
    errtxt = "key = ";
    errtxt += key;
    return -129;
  }
  values.clear();
  typename vector<vector<T>>::size_type nrows = tmpval.size() / ncols, irow;
  for( irow = 0; irow < nrows; ++irow ) {
    vector<T> row;
    for( typename vector<T>::size_type i = 0; i < ncols; ++i ) {
      row.push_back(tmpval.at(i + irow * ncols));
    }
    values.push_back(row);
  }
  return 0;
}

//_____________________________________________________________________________
template<typename T> static inline
Int_t load_and_assign( FILE* f, const TDatime& date, const char* key,
                       void* dest, UInt_t& nelem )
{
  Int_t st = -1;
  if( nelem < 2 ) {
    T val{};
    st = LoadDBvalue(f, date, key, val);
    if( st == 0 )
      memcpy(dest, &val, sizeof(T));
  } else {
    vector<T> vals;
    st = LoadDBarray(f, date, key, vals);
    if( st == 0 ) {
      if( vals.size() != nelem ) {
        nelem = vals.size();
        st = -130;
      } else {
        memcpy( dest, vals.data(), nelem*sizeof(T));
      }
    }
  }
  return st;
}

//_____________________________________________________________________________
template<typename T> static inline
Int_t load_and_assign_vector( FILE* f, const TDatime& date, const char* key,
                              void* dest, UInt_t& nelem )
{
  vector<T>& vec = *reinterpret_cast<vector<T>*>(dest);
  Int_t st = LoadDBarray(f, date, key, vec);
  if( st == 0 && nelem > 0 && nelem != vec.size() ) {
    nelem = vec.size();
    st = -130;
  }
  return st;
}

//_____________________________________________________________________________
template<typename T> static inline
Int_t load_and_assign_matrix( FILE* f, const TDatime& date, const char* key,
                              void* dest, UInt_t nelem )
{
  vector<vector<T>>& mat = *reinterpret_cast<vector<vector<T>>*>(dest);
  Int_t st = LoadDBmatrix(f, date, key, mat, nelem);
  return st;
}

//_____________________________________________________________________________
Int_t LoadDatabase( FILE* f, const TDatime& date, const DBRequest* req,
                    const char* prefix, Int_t search, const char* here )
{
  // Load a list of parameters from the database file 'f' according to
  // the contents of the 'req' structure (see VarDef.h).

  if( !f ) {
    ::Error(::Here(here, prefix),
            "Bad argument FILE* = NULL. Probably file not found. "
            "Make sure to check for success after opening DB file.");
    return -255;
  }
  if( !req ) {
    ::Warning(::Here(here, prefix),
            "Database request is NULL. Nothing loaded.");
    return 0;
  }
  if( !prefix ) prefix = "";
  Int_t ret = 0;
  if( loaddb_depth++ == 0 )
    loaddb_prefix = prefix;

  const DBRequest* item = req;
  while( item->name ) {
    if( item->var ) {
      string keystr = prefix;
      keystr.append(item->name);
      UInt_t nelem = item->nelem;
      const char* key = keystr.c_str();
      switch( item->type ) {
      case kDouble:
        ret = load_and_assign<Double_t>(f, date, key, item->var, nelem);
        break;
      case kFloat:
        ret = load_and_assign<Float_t>(f, date, key, item->var, nelem);
        break;
      case kLong:
        ret = load_and_assign<Long64_t>(f, date, key, item->var, nelem);
        break;
      case kULong:
        ret = load_and_assign<ULong64_t>(f, date, key, item->var, nelem);
        break;
      case kInt:
        ret = load_and_assign<Int_t>(f, date, key, item->var, nelem);
        break;
      case kUInt:
        ret = load_and_assign<UInt_t>(f, date, key, item->var, nelem);
        break;
      case kShort:
        ret = load_and_assign<Short_t>(f, date, key, item->var, nelem);
        break;
      case kUShort:
        ret = load_and_assign<UShort_t>(f, date, key, item->var, nelem);
        break;
      case kChar:
        ret = load_and_assign<Char_t>(f, date, key, item->var, nelem);
        break;
      case kByte:
        ret = load_and_assign<Byte_t>(f, date, key, item->var, nelem);
        break;
      case kString:
        ret = LoadDBvalue(f, date, key, *((string*)item->var));
        break;
      case kTString:
        ret = LoadDBvalue(f, date, key, *((TString*)item->var));
        break;
      case kFloatV:
        ret = load_and_assign_vector<Float_t>(f, date, key, item->var, nelem);
        break;
      case kDoubleV:
        ret = load_and_assign_vector<Double_t>(f, date, key, item->var, nelem);
        break;
      case kIntV:
        ret = load_and_assign_vector<Int_t>(f, date, key, item->var, nelem);
        break;
      case kFloatM:
        ret = load_and_assign_matrix<Float_t>(f, date, key, item->var, nelem);
        break;
      case kDoubleM:
        ret = load_and_assign_matrix<Double_t>(f, date, key, item->var, nelem);
        break;
      case kIntM:
        ret = load_and_assign_matrix<Int_t>(f, date, key, item->var, nelem);
        break;
      default:
        ret = -2;
        break;
      }

      if( ret == 0 ) {  // Key found -> next item
        goto nextitem;
      } else if( ret > 0 ) {  // Key not found
        // If searching specified, either for this key or globally, retry
        // finding the key at the next level up along the name tree. Name
        // tree levels are defined by dots (".") in the prefix. The top
        // level is 1 (where prefix = "").
        // Example: key = "nw", prefix = "L.vdc.u1", search = 1, then
        // search for:  "L.vdc.u1.nw" -> "L.vdc.nw" -> "L.nw" -> "nw"
        //
        // Negative values of 'search' mean search up relative to the
        // current level by at most abs(search) steps, or up to top level.
        // Example: key = "nw", prefix = "L.vdc.u1", search = -1, then
        // search for:  "L.vdc.u1.nw" -> "L.vdc.nw"

        // per-item search level overrides global one
        Int_t newsearch = (item->search != 0) ? item->search : search;
        if( newsearch != 0 && *prefix ) {
          string newprefix(prefix);
          Int_t newlevel = ChopPrefix(newprefix) + 1;
          if( newsearch < 0 || newlevel >= newsearch ) {
            DBRequest newreq[2];
            newreq[0] = *item;
            memset(newreq + 1, 0, sizeof(DBRequest));
            newreq->search = 0;
            if( newsearch < 0 )
              newsearch++;
            ret = LoadDatabase(f, date, newreq, newprefix.c_str(), newsearch, here);
            // If error, quit here. Error message printed at lowest level.
            if( ret != 0 )
              break;
            goto nextitem;  // Key found and ok
          }
        }
        if( item->optional )
          ret = 0;
        else {
          if( item->descript ) {
            ::Error(::Here(here, loaddb_prefix.c_str()),
                    R"/(Required key "%s" (%s) missing in the database.)/",
                    key, item->descript);
          } else {
            ::Error(::Here(here, loaddb_prefix.c_str()),
                    R"(Required key "%s" missing in the database.)", key);
          }
          // For missing keys, the return code is the index into the request
          // array + 1. In this way the caller knows which key is missing.
          ret = 1 + static_cast<Int_t>(std::distance(req, item));
          break;
        }
      } else if( ret == -1 ) {  // errno != 0
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Key "%s": File read error in %s )",
                key, errtxt.c_str());
      } else if( ret == -2 ) {  // Unsupported type
        if( item->type >= kDouble && item->type <= kObject2P )
          ::Error(::Here(here, loaddb_prefix.c_str()),
                  R"(Key "%s": Reading of data type "%s" not implemented)",
                  key, Vars::GetEnumName(item->type));
        else
          ::Error(::Here(here, loaddb_prefix.c_str()),
                  R"/(Key "%s": Reading of data type "(#%d)" not implemented)/",
                  key, item->type);
        break;
      } else if( ret == -128 ) {  // Line too long
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Key "%s": Text line too long. Fix the database!\n"%s...")",
                key, errtxt.c_str());
        break;
      } else if( ret == -129 ) {  // Matrix ncols mismatch
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Key "%s": Number of matrix elements not evenly divisible)"
                R"(by requested number of columns. Fix the database!\n"%s...")",
                key, errtxt.c_str());
        break;
      } else if( ret == -130 ) {  // Vector/array size mismatch
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Key "%s": Incorrect number of array elements found. )"
                "%u requested, %u found. Fix database.", key,
                item->nelem, nelem);
        break;
      } else if( ret == -131 ) {  // Error converting string to numerical value
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Key "%s": Numerical conversion error: %s. )",
                key, errtxt.c_str());
        break;
      } else {  // other ret < 0: unexpected zero pointer etc.
        ::Error(::Here(here, loaddb_prefix.c_str()),
                R"(Program error %d when trying to read database key "%s". )"
                "CALL EXPERT!", ret, key);
        break;
      }
    }
nextitem:
    item++;
  }
  if( --loaddb_depth == 0 )
    loaddb_prefix.clear();

  return ret;
}

//_____________________________________________________________________________
Int_t SeekDBconfig( FILE* file, const char* tag, const char* label,
                    Bool_t end_on_tag )
{
  // Starting from the current position in 'file', look for the
  // configuration 'tag'. Position the file on the
  // line immediately following the tag. If no tag found, return to
  // the original position in the file.
  // Return zero if not found, 1 otherwise.
  //
  // Configuration tags have the form [ config=tag ].
  // If 'label' is given explicitly, it replaces 'config' in the tag string,
  // for example label="version" will search for [ version=tag ].
  // If 'label' is empty (""), search for just [ tag ].
  //
  // If 'end_on_tag' is true, quit if any non-matching tag found,
  // i.e. anything matching "*[*]*" except "[config=anything]".
  //
  // Useful for segmenting databases (esp. VDC) for different
  // experimental configurations.

  static const char* const here = "SeekDBconfig";

  if( !file || !tag || !*tag ) return 0;
  string _label("[");
  if( label && *label ) {
    _label.append(label);
    _label.append("=");
  }
  auto llen = _label.size();

  bool found = false;

  errno = 0;
  off_t pos = ftello(file);
  if( pos != -1 ) {
    bool quit = false;
    const int LEN = 256;
    char buf[LEN];
    while( !errno && !found && !quit && fgets(buf, LEN, file) ) {
      size_t len = strlen(buf);
      if( len < 2 || buf[0] == '#' ) continue;      //skip comments
      if( buf[len - 1] == '\n' ) buf[len - 1] = 0;     //delete trailing newline
      char* cbuf = ::Compress(buf);
      string line(cbuf);
      delete[] cbuf;
      auto lbrk = line.find(_label);
      if( lbrk != string::npos && lbrk + llen < line.size() ) {
        auto rbrk = line.find(']', lbrk + llen);
        if( rbrk == string::npos ) continue;
        if( line.substr(lbrk + llen, rbrk - lbrk - llen) == tag ) {
          found = true;
          break;
        }
      } else if( end_on_tag && IsTag(buf) )
        quit = true;
    }
  }
  if( errno ) {
    perror(here);
    found = false;
  }
  // If not found, rewind to previous position
  if( !found && pos >= 0 && fseeko(file, pos, SEEK_SET) )
    perror(here); // TODO: should throw exception

  return found;
}

//_____________________________________________________________________________
Int_t SeekDBdate( FILE* file, const TDatime& date, Bool_t end_on_tag )
{
  // Starting from the current position in file 'f', look for a
  // date tag matching time stamp 'date'. Position the file on the
  // line immediately following the tag. If no tag found, return to
  // the original position in the file.
  // Return zero if not found, 1 otherwise.
  // Date tags must be in SQL format: [ yyyy-mm-dd hh:mi:ss ].
  // If 'end_on_tag' is true, end the search at the next non-date tag;
  // otherwise, search through end of file.
  // Useful for sub-segmenting database files.

  static const char* const here = "SeekDBdateTag";

  if( !file ) return 0;
  const int LEN = 256;
  char buf[LEN];
  TDatime tagdate(950101, 0), prevdate(950101, 0);
  const bool kNoWarn = false;

  errno = 0;
  off_t pos = ftello(file);
  if( pos == -1 ) {
    if( errno )
      perror(here);
    return 0;
  }
  off_t foundpos = -1;
  bool found = false, quit = false;
  while( !errno && !quit && fgets(buf, LEN, file) ) {
    size_t len = strlen(buf);
    if( len < 2 || buf[0] == '#' ) continue;
    if( buf[len - 1] == '\n' ) buf[len - 1] = 0; //delete trailing newline
    string line(buf);
    if( IsDBdate(line, tagdate, kNoWarn)
        && tagdate <= date && tagdate >= prevdate ) {
      prevdate = tagdate;
      foundpos = ftello(file);
      found = true;
    } else if( end_on_tag && IsTag(buf) )
      quit = true;
  }
  if( errno ) {
    perror(here);
    found = false;
  }
  if( fseeko(file, (found ? foundpos : pos), SEEK_SET) ) {
    perror(here); // TODO: should throw exception
    found = false;
  }
  return found;
}

//_____________________________________________________________________________
Int_t SeekDBdate( istream& istr, const TDatime& date, Bool_t end_on_tag )
{
  // Starting from the current position in input stream 'istr', look for a
  // date tag matching time stamp 'date'. Position the stream on the
  // line immediately following the tag. If no tag found, return to
  // the original position in the stream.
  // Return zero if not found, 1 otherwise.
  // Date tags must be in SQL format: [ yyyy-mm-dd hh:mi:ss ].
  // If 'end_on_tag' is true, end the search at the next non-date tag;
  // otherwise, search through end of stream.
  // Useful for sub-segmenting database files.

  //static const char* const here = "SeekDBdate";

  if( !istr.good() )
    return 0;
  TDatime tagdate(950101, 0), prevdate(950101, 0);
  const bool kNoWarn = false;

  auto pos = istr.tellg();
  if( pos == -1 )
    return 0;

  string line;
  decltype(pos) foundpos{-1};
  bool found = false;
  while( getline(istr, line) ) {
    auto lpos = line.find_first_of("!#");
    if( lpos != string::npos )
      line.erase(lpos);
    if( line.find_first_not_of(" \t") == string::npos )
      continue;
    if( IsDBdate(line, tagdate, kNoWarn)
        && tagdate <= date && tagdate >= prevdate ) {
      prevdate = tagdate;
      foundpos = istr.tellg();
      found = true;
    } else if( end_on_tag && IsTag(line.c_str()) )
      break;
  }
  if( istr.fail() && !istr.eof() ) {
    cerr << "Error reading database" << endl;
    found = false;
  }
  if( !istr.seekg((found ? foundpos : pos), ios_base::beg) )
    found = false;
  return found;
}

//_____________________________________________________________________________
Bool_t IsDBtimestamp( const string& line, TDatime& date )
{
  return IsDBdate(line, date, true);
}

//_____________________________________________________________________________
#ifdef __clang__
// Clang appears to make implicitly instantiated template functions private
// symbols when linking optimized code. Prevent this with explicit instantiations.
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Double_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Float_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Long64_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, ULong64_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Int_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, UInt_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Short_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, UShort_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Char_t& );
template Int_t LoadDBvalue(FILE*, const TDatime&, const char*, Byte_t& );

template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Double_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Float_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Long64_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<ULong64_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Int_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<UInt_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Short_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<UShort_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Char_t>&);
template Int_t LoadDBarray( FILE*, const TDatime&, const char*, vector<Byte_t>&);

template Int_t LoadDBmatrix( FILE*, const TDatime&, const char*,
                             vector<vector<Double_t>>&, UInt_t);
template Int_t LoadDBmatrix( FILE*, const TDatime&, const char*,
                             vector<vector<Float_t>>&, UInt_t);
template Int_t LoadDBmatrix( FILE*, const TDatime&, const char*,
                             vector<vector<Int_t>>&, UInt_t);
#endif

} // namespace Podd
