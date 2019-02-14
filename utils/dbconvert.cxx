//*-- Author :    Ole Hansen  28-May-2015
//
// dbconvert.cxx
//
// Utility to convert Podd 1.5 and earlier database files to Podd 1.6
// and later format

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>    // for strdup
#include <ctime>
#include <cctype>     // for isdigit, tolower
#include <cerrno>
#include <cmath>
#include <stdexcept>
#include <cstddef>    // for offsetof
#include <cassert>
#include <map>
#include <set>
#include <limits>
#include <utility>
#include <iterator>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h> // for stat/lstat
#include <sys/ioctl.h> // for ioctl to get terminal windows size
#include <dirent.h>   // for opendir/readdir
#include <getopt.h>   // got getopt_long
#include <libgen.h>   // for POSIX basename()

#include "TString.h"
#include "TDatime.h"
#include "TMath.h"
#include "TVector3.h"
#include "TError.h"

#include "THaAnalysisObject.h"
#include "THaVDC.h"
#include "THaDetMap.h"
#include "THaString.h"  // for Split()
#include "Decoder.h"    // for MAXROC, MAXSLOT

#define kInitError THaAnalysisObject::kInitError
#define kOK        THaAnalysisObject::kOK
#define kBig       THaAnalysisObject::kBig

using namespace std;

#define ALL(c) (c).begin(), (c).end()

static bool IsDBdate( const string& line, time_t& date );
static bool IsDBcomment( const string& line );
static Int_t IsDBkey( const string& line, string& key, string& text );
static bool IsDBSubDir( const string& fname, time_t& date );

// Command line parameter defaults
static int do_debug = 0, verbose = 0, do_file_copy = 1, do_subdirs = 0;
static int do_clean = 1, do_verify = 1, do_dump = 0, purge_all_default_keys = 1;
static int format_fp = 1, format_fixed = 0;
static string srcdir;
static string destdir;
static string prgname;
static const char* mapfile = 0;
static const char* inp_tz_arg = 0, *outp_tz_arg = 0;
static string inp_tz, outp_tz, cur_tz;
static string current_filename;
static const char* c_out_subdirs = 0;
static vector<string> out_subdirs;

static const string prgargs("SRC_DIR DEST_DIR");
static const string whtspc = " \t";

static struct option longopts[] = {
  // Flags
  { "help",                 no_argument,       0,     'h' },
  { "verbose",              no_argument,       0,     'v' },
  { "debug",                no_argument,       0,     'd' },
  { "preserve-subdirs",     no_argument,       0,     'p' },
  { "no-preserve-subdirs",  no_argument, &do_subdirs,  0  },
  { "no-clean",             no_argument, &do_clean,    0  },
  { "no-verify",            no_argument, &do_verify,   0  },
  // Parameters
  { "mapfile",              required_argument, 0,     'm' },
  { "detlist",              required_argument, 0,     'l' },  // wildcard list detector names
  { "subdirs",              required_argument, 0,     's' },  // overrides preserve-subdirs
  { "input-timezone",       required_argument, 0,     'z' },
  { "output-timezone",      required_argument, 0,      1  },
  { 0, 0, 0, 0 }
};

static const char* const opthelp[] = {
  "show this help message",
  "increase verbosity",
  "print extensive debug info",
  "preserve subdirectory structure",
  "don't preserve subdirectory structure (negates -p)",
  "don't erase all existing files from DEST_DIR if it exists",
  "don't ask for confirmation before deleting any files",
  "read mapping from file names to detector types from <ARG>",
  "convert only detectors given in <ARG>. ARG is a comma-separated "
    "list of detector names which may contain wildcards * and ?.",
  "split output files into new set of sudirectories given in <ARG>. "
    "ARG is a comma-separated list of directory names which must be "
    "in YYYYMMDD format. Overrides -p.",
  "interpret all validity timestamps in the input that don't have a timezone "
    "indicator to be in timezone <ARG>. "
    "ARG may be the name of a system timezone file (e.g. 'US/Eastern'), "
    "which supports DST, or a fixed offset specified as [+|-]nnnn "
    "(e.g. +0100 for central European standard time). The default is "
    "to use the current timezone at run time.",
  "write all output timestamps relative to timezone ARG, specified in the "
    "same way as --input-timezone. The default is to use the current "
    "timezone at run time."
  /*
  //TODO...
  // cout << " -o <outfile>: Write output to <outfile>. Default: "
  //      << OUTFILE_DEFAULT << endl;
  // cout << " <infile>: Read input from <infile>. Default: "
  //      << INFILE_DEFAULT << endl;
  */
};

// Information for a single source database file
struct Filenames_t {
  Filenames_t( time_t _start, const string& _path = "" )
    : val_start(_start), path(_path) {}
  time_t    val_start;
  string    path;
  // Order by validity time
  bool operator<( const Filenames_t& rhs ) const {
    return val_start < rhs.val_start;
  }
};

typedef map<string, multiset<Filenames_t> > FilenameMap_t;
typedef multiset<Filenames_t>::iterator fiter_t;
typedef string::size_type ssiz_t;
typedef set<time_t>::iterator siter_t;

//-----------------------------------------------------------------------------
static size_t get_term_width()
{
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); //FIXME: should be the file no of the stream
  return w.ws_col;
}

//-----------------------------------------------------------------------------
// Helper class for usage()/help() printing
class wrapping_ostream {
public:
  wrapping_ostream( ostream& _os )
    : os(_os), pos(0), width(get_term_width()), first_line(true),
      first_item(true)
  {
    if( width == 0 )
      width = 80;
    set_indent(8);
  }
  void set_width( string::size_type n )  { width = n; }
  void reset() { pos = 0; first_line = first_item = true; }

  wrapping_ostream& set_indent( string::size_type n )
  {
    indent.assign(n, ' ');
    return *this;
  }
  wrapping_ostream& operator<<( const string& str ) { return write_item(str); }
  wrapping_ostream& advance( string::size_type tab )
  {
    // Move to tab position 'tab'. If already at or past 'tab', start a new line
    if( tab > width )
      tab = width;
    if( pos >= tab ) {
      os << endl;
      pos = 0;
    }
    for( ; pos < tab; ++pos )
      os << " ";
    return *this;
  }
  wrapping_ostream& end_line() { os << endl; reset(); return *this; }
  wrapping_ostream& write( const string& str )
  {
    os << str;
    pos += str.size();
    return *this;
  }
  wrapping_ostream& write_trimmed( string str )
  {
    // Trim leading whitespace before writing the string
    string::size_type pos = str.find_first_not_of(whtspc);
    if( pos == string::npos )
      return *this;
    str.erase( 0, pos );
    return write(str);
  }
  wrapping_ostream& write_item( const string& str )
  {
    // Write 'str' to output stream. If the output width is exceeded, start
    // a new line and indent it by 'indent'. The first item is always written to
    // the current line, even if it exceeds the width, and items are not broken.
    // This may give odd-looking results if width is very small or items are
    // very long.
    bool ind = false;
    if( first_item && !first_line ) {
      write(indent);
      ind = true;
    }
    if( pos + str.size() + (first_item ? 0 : 1) >= width ) {
      if( first_item ) {
	if( ind ) write_trimmed(str);
	else os << str;
      }
      os << endl;
      pos = 0;
      if( !first_item ) {
	if( str.find_first_not_of(whtspc) != string::npos ) {
	  write(indent);
	  write_trimmed(str);
	} else
	  // The item is all-whitespace and at the beginning of the line
	  first_item = true;
      }
      first_line = false;
    }
    else {
      write(str);
      first_item = false;
    }
    return *this;
  }
  wrapping_ostream& write_items( const string& str )
  {
    istringstream istr(str);
    string item;
    bool next = false;
    while( istr >> item ) {
      if( next)
	*this << " ";
      *this << item;
      next = true;
    }
    return *this;
  }

private:
  ostream& os;
  string::size_type pos, width;
  bool first_line, first_item;
  string indent;
};

//-----------------------------------------------------------------------------
static void print_usage( ostream& os, const string& program_name,
			 const string& nonoptions,
			 const struct option optarray[] )
{
  // Print usage info, including all defined short/long options

  wrapping_ostream ps(os);

  // Write "Usage: program_name "
  ps << "Usage: " << program_name;

  // Write all defined options in short and long form, as applicable.
  // This assumes that every short option has a corresponding long form,
  // however not all long options need to have a short form.
  const struct option* opt = optarray;
  while( opt && opt->name ) {
    ostringstream ostr;
    ostr << " [";
    if( (opt->val != 0) && isalnum(opt->val) )
      ostr << "-" << static_cast<char>(opt->val) << "|";
    ostr << "--" << opt->name;
    if( opt->has_arg )
      ostr << " ARG";
    ostr << "]";
    ps << ostr.str();
    ++opt;
  }

  // Write non-option arguments
  ps.write_items( nonoptions );
  ps.end_line();
}

//-----------------------------------------------------------------------------
static void print_help( ostream& os, const string& program_name,
			const string& nonoptions,
			const string& description,
			const struct option optarray[],
			const char* const helptxts[] )
{
  // Print help message with program and option descriptions

  const string::size_type col2 = 30;
  wrapping_ostream ps(os);
  const struct option* opt = optarray;

  ps << "Usage: " << program_name << " ";
  if( opt )
    ps << "[options] ";
  ps.write_items(nonoptions);
  ps.end_line().set_indent(1).advance(1);
  ps.write_items(description);
  ps.end_line();

  if( !opt )
    return;
  ps.write("Options:");
  ps.end_line();

  ps.set_indent(col2+2);
  while( opt->name ) {
    ostringstream ostr;
    ostr << " ";
    if( (opt->val != 0) && isalnum(opt->val) )
      ostr << "-" << static_cast<char>(opt->val) << ",";
    else
      ostr << "   ";
    ostr << " ";
    ostr << "--" << opt->name;
    if( opt->has_arg )
      ostr << " ARG";

    ps << ostr.str();
    ps.advance(col2);
    ps.write_items( *(helptxts+(opt-optarray)) );
    ps.end_line();
    ++opt;
  }
}

//-----------------------------------------------------------------------------
static void usage()
{
  // Print usage message and exit with error code

  print_usage( cerr, prgname, prgargs, longopts );
  exit(EXIT_FAILURE);
}

//-----------------------------------------------------------------------------
static void help()
{
  // Print help and exit

  print_help( cout, prgname, prgargs,
	      "Convert Podd 1.5 and earlier database files under SRC_DIR "
	      "to Podd 1.6 and later format, written to DEST_DIR",
	      longopts, opthelp );

  exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------------------
static inline string TZfromOffset( int off )
{
  // Convert integer timezone offset to string for TZ environment variable

  div_t d = div( std::abs(off), 3600 );
  if( d.quot > 24 )
    return string();
  ostringstream ostr;
  // Flip the sign. Input is east of UTC, $TZ uses west of UTC.
  ostr << "OFFS" << ((off >= 0) ? "-" : "+");
  ostr << d.quot;
  if( d.rem != 0 )
    ostr << ":" << d.rem;
  return ostr.str();
}

//-----------------------------------------------------------------------------
static int MkTimezone( string& zone )
{
  // Check command-line timezone spec 'zone' and convert it to something that
  // will work with tzset
  // Allowed syntax:
  // [+|-]nnnn:  Explicit timezone offset east of UTC (no DST) as HHMM. Must be
  //             parseable as an integer with 0 <= HH <= 24 and 0 <= MM < 60.
  //             Trailing characters are ignored.
  // characters: Timezone file name.
  if( zone.empty() )
    return -1;
  int off;
  istringstream istr(zone);
  if( istr >> off ) {
    // Convert HHMM to seconds
    div_t d = div( std::abs(off), 100 );
    if( d.quot > 24 || d.rem > 59 )
      return 1;
    off = ((off < 0 ) ? -1 : 1) * 60 * (60*d.quot + d.rem);
    zone = TZfromOffset( off );
    if( zone.empty() )
      return 1;
  } else {
    zone.insert( zone.begin(), ':' );
  }
  return 0;
}

//-----------------------------------------------------------------------------
static inline void set_tz( const string& tz )
{
  if( !tz.empty() ) {
    const char* z = getenv("TZ");
    if( z )
      cur_tz = z;
    else
      cur_tz.clear();
    setenv("TZ", tz.c_str(), 1);
    errno = 0;
    tzset();
  }
}

//-----------------------------------------------------------------------------
static int set_tz_errcheck( const string& tz, const string& which )
{
  errno = 0;
  set_tz( tz );
  if( errno && !tz.empty() && tz[0] == ':' ) {
    ostringstream ss;
    ss << "Error setting " << which << " time zone TZ = \"" << tz << "\"";
    perror(ss.str().c_str());
    return errno;
  }
  return 0;
}

//-----------------------------------------------------------------------------
static inline void reset_tz()
{
  if( !cur_tz.empty() )
    setenv("TZ", cur_tz.c_str(), 1);
  else
    unsetenv("TZ");
  tzset();
}

//-----------------------------------------------------------------------------
static void getargs( int argc, char* const argv[] )
{
  // Get command line parameters

  char* argv0 = strdup(argv[0]);
  prgname = basename(argv0);
  free(argv0);

  int opt;
  while( (opt = getopt_long(argc, argv, "hvdpm:s:z:", longopts, 0)) != -1) {
    switch( opt ) {
    case 'h':
      help();
      break;
    case 'v':
      verbose = 1;
      break;
    case 'd':
      do_debug = 1;
      break;
    case 'p':
      do_subdirs = 1;
      break;
    case 'm':
      mapfile = optarg;
      break;
    case 's':
      c_out_subdirs = optarg;
      break;
    case 'z':
      inp_tz_arg = optarg;
      break;
    case 0:
      break;
    case 1:
      outp_tz_arg = optarg;
      break;
    case ':':
    case '?':
      usage();
      break;
    default:
      cerr << "Unhandled option " << (char)opt << endl;
      usage();
    }
  }
  argc -= optind;
  argv += optind;

  // Retrieve non-option arguments (source and destination directories)
  // TODO: add option have several files as input
  if( argc < 1 ) {
    cerr << "Error: Must specify SRC_DIR and DEST_DIR" << endl;
    usage();
  }
  if( argc == 1 ) {
    cerr << "Error: Must specify DEST_DIR" << endl;
    usage();
  }
  if( argc > 2 ) {
    cerr << "Error: too many arguments" << endl;
    usage();
  }
  srcdir  = argv[0];
  destdir = argv[argc-1];

  // Parse timezone specs
  if( inp_tz_arg ) {
    inp_tz = inp_tz_arg;
    if( MkTimezone(inp_tz) ) {
      cerr << "Invalid input timezone: \"" << inp_tz << "\"" << endl;
      usage();
    }
  }
  if( outp_tz_arg ) {
    outp_tz = outp_tz_arg;
    if( MkTimezone(outp_tz) ) {
      cerr << "Invalid output timezone: \"" << outp_tz << "\"" << endl;
      usage();
    }
  }
  // If an input timezone is given and no explicit output timezone, set
  // output timezone to input timezone. Hopefully, this makes the most common
  // usage case, converting a JLab DB on a machine in a different timezone than
  // JLab, the most convenient
  if( !inp_tz.empty() && outp_tz.empty() )
    outp_tz = inp_tz;

  out_subdirs.clear();
  if( c_out_subdirs && *c_out_subdirs ) {
    istringstream istr(c_out_subdirs);
    string item;
    time_t date;
    while( getline(istr,item,',') ) {
      if( item != "DEFAULT" && IsDBSubDir(item,date) )
	out_subdirs.push_back(item);
    }
    do_subdirs = true;
  }

  //DEBUG
  if( mapfile )
    cout << "Mapfile name is \"" << mapfile << "\"" << endl;
  cout << "Converting from \"" << srcdir << "\" to \"" << destdir << "\"" << endl;

}

//-----------------------------------------------------------------------------
static inline time_t MkTime( struct tm& td, bool have_tz = false )
{
  string save_tz;
  if( have_tz ) {
    save_tz = cur_tz;
    set_tz( TZfromOffset(td.tm_gmtoff) );
  } else
    td.tm_isdst = -1;

  time_t ret = mktime( &td );

  if( have_tz ) {
    reset_tz();
    cur_tz = save_tz;
  }
  return ret;
}

//-----------------------------------------------------------------------------
static inline time_t MkTime( int yy, int mm, int dd, int hh, int mi, int ss )
{
  // Return Unix time representation (seconds since epoc in UTC) of given
  // broken-down time

  struct tm td;
  td.tm_sec   = ss;
  td.tm_min   = mi;
  td.tm_hour  = hh;
  td.tm_year  = yy-1900;
  td.tm_mon   = mm-1;
  td.tm_mday  = dd;
  td.tm_isdst = -1;

  return MkTime( td );
}

//-----------------------------------------------------------------------------
static inline string format_time( time_t t )
{
  if( t < 0 || t == numeric_limits<time_t>::max() )
    return "(inf)";
  char buf[32];
  ctime_r( &t, buf );
  // Translate date of the form "Wed Jun 30 21:49:08 1993\n" to
  // "Jun 30 1993 21:49:08"
  string ts( buf+4, 4 );
  if( buf[8] != ' ' ) ts += buf[8];
  ts += buf[9];
  ts.append( buf+19, 5 );
  ts.append( buf+10, 9 );
  return ts;
}

//-----------------------------------------------------------------------------
static inline string format_tstamp( time_t t )
{
  // Generate timestamp string for database files. Timestamps are in UTC
  // unless the user requests otherwise. The applicable timezone offset is
  // written along with the timestamp.

  struct tm tms;
  localtime_r( &t, &tms );

  stringstream ss;
  ss << "--------[ ";
  ss << tms.tm_year+1900 << "-";
  ss << setw(2) << setfill('0') << tms.tm_mon+1 << "-";
  ss << setw(2) << setfill('0') << tms.tm_mday  << " ";
  ss << setw(2) << setfill('0') << tms.tm_hour  << ":";
  ss << setw(2) << setfill('0') << tms.tm_min   << ":";
  ss << setw(2) << setfill('0') << tms.tm_sec   << " ";
  // Timezone offset
  ldiv_t offs = ldiv( std::abs(tms.tm_gmtoff), 3600 );
  ss << (( tms.tm_gmtoff >= 0 ) ? "+" : "-");
  ss << setw(2) << setfill('0') << offs.quot;
  ss << setw(2) << setfill('0') << offs.rem/60;
  ss << " ]";

  return ss.str();
}

//-----------------------------------------------------------------------------
static inline string MakePath( const string& dir, const string& fname )
{
  bool need_slash = (*dir.rbegin() != '/');
  string fpath(dir);
  if( need_slash ) fpath += '/';
  fpath.append(fname);
  return fpath;
}

//-----------------------------------------------------------------------------
static inline bool IsDBFileName( const string& fname )
{
  return (fname.size() > 7 && fname.substr(0,3) == "db_" &&
	  fname.substr(fname.size()-4,4) == ".dat" );
}

//-----------------------------------------------------------------------------
static bool IsDBSubDir( const string& fname, time_t& date )
{
  // Check if the given file name corresponds to a database subdirectory.
  // Subdirectories have filenames of the form "YYYYMMDD" and "DEFAULT".
  // If so, extract its encoded time stamp to 'date'

  const ssiz_t LEN = 8;

  if( fname == "DEFAULT" ) {
    date = 0;
    return true;
  }
  if( fname.size() != LEN )
    return false;

  ssiz_t pos = 0;
  while( pos != LEN )
    if( !isdigit(fname[pos++]) )
      return false;

  // Convert date encoded in directory name to time_t
  int year  = atoi( fname.substr(0,4).c_str() );
  int month = atoi( fname.substr(4,2).c_str() );
  int day   = atoi( fname.substr(6,2).c_str() );
  if( year < 1900 || month == 0 || month > 12 || day > 31 || day < 1 )
    return false;
  date = MkTime( year, month, day, 0, 0, 0 );
  return ( date != static_cast<time_t>(-1) );
}

//-----------------------------------------------------------------------------
static inline size_t Order( Int_t n )
{
  // Return number of digits of 32-bit integer argument

  // This is much faster than computing log10
  UInt_t u = std::abs(n), d = 1;
  size_t c = 0;
  while (1) {
    if( u < d ) return c;
    d *= 10;
    ++c;
    assert( c < 11 );
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T> static inline
size_t GetSignificantDigits( T x, T round_level )
{
  // Get number of significant *decimal* digits of x, rounded at given level

  T xx;
  if( x >= 0.0 )
    xx = x - std::floor(x);
  else
    xx = std::ceil(x) - x;
  if( xx < 0.5*round_level )
    return 0;
  // Now 0 <= xx < 1. Deal with leading zeros
  if( xx < 0.1 )
    xx += 0.1;
  Int_t ix = xx/round_level + 0.5;
  size_t nn = Order(ix), n = nn;
  for( size_t j = 1; j < nn; ++j ) {
    // Remove trailing zeros until we find a non-zero digit
    Int_t ix1 = ix/10;
    if( 10*ix1 != ix )
      break;
    --n;
    ix = ix1;
  }
  return n;
}

//-----------------------------------------------------------------------------
template <class T> static inline
void PrepareStreamImpl( ostringstream& str, const vector<T>& arr, T round_level )
{
  // Determine formatting for floating point data

  // Formatting only matters for groups of numbers; we want them to line up
  if( arr.size() < 2 )
    return;

  size_t ndigits = 0;
  bool sci = false;
  const T zero = 0.L, one = 1.L, ten = 10.L;
  typename vector<T>::size_type i;
  for( i = 0; i < arr.size(); ++i ) {
    T x = std::abs( arr[i] );
    if( x != zero && (x < 1e-3L || x >= 1e6L) ) {
      sci = true;
      break;
    }
  }
  if( sci ) {
    // Scientific format
    for( i = 0; i < arr.size(); ++i ) {
      T x = std::abs(arr[i]);
      if( x == zero ) continue;
      x *= std::pow( ten, -std::ceil(std::log10(x)) );
      assert( x < ten );
      if( x >= one ) x /= ten;
      ndigits = max( ndigits, GetSignificantDigits(x,round_level) );
    }
    if( ndigits > 0 )
      str << scientific << setprecision(ndigits-1);
    return;
  }
  else if( format_fixed ) {
    // Fixed format
    for( i = 0; i < arr.size(); ++i ) {
      T x = std::abs(arr[i]);
      ndigits = max( ndigits, GetSignificantDigits(x,round_level) );
    }
    if( ndigits > 0 )
      str << fixed << setprecision(ndigits);
  }
}

//-----------------------------------------------------------------------------
template <class T> static inline
void PrepareStream( ostringstream&, const T*, int )
{
  // Set formatting of stream 'str' based on data in array.
  // By default, do nothing.
}

//-----------------------------------------------------------------------------
template <> inline
void PrepareStream( ostringstream& str, const double* array, int size )
{
  vector<double> arr( array, array+size );
  PrepareStreamImpl<double>( str, arr, 1e-6 );
}

//-----------------------------------------------------------------------------
template <> inline
void PrepareStream( ostringstream& str, const float* array, int size )
{
  vector<float> arr( array, array+size );
  PrepareStreamImpl<float>( str, arr, 1e-5 );
}

//-----------------------------------------------------------------------------
// Structure returned by MakeValue functions, containing a database value
// (possibly an array) as a string and metadata: number of array elements,
// printing width needed to accommodate all values.
struct Value_t {
  Value_t( const string& v, int n, ssiz_t w ) : value(v), nelem(n), width(w) {}
  string value;
  int    nelem;
  ssiz_t width;
};

//-----------------------------------------------------------------------------
template <class T> static inline
Value_t MakeValue( const T* array, int size = 1 )
{
  ostringstream ostr;
  ssiz_t w = 0;
  if( size <= 0 ) size = 1;
  if( format_fp )
    PrepareStream( ostr, array, size );
  for( int i = 0; i < size; ++i ) {
    ssiz_t len = ostr.str().size();
    ostr << array[i];
    w = max(w, ostr.str().size()-len);
    if( i+1 < size ) ostr << "  ";
  }
  return Value_t(ostr.str(), size, w);
}

//-----------------------------------------------------------------------------
static inline
Value_t MakeDetmapElemValue( const THaDetMap* detmap, int n, int extras )
{
  ostringstream ostr;
  THaDetMap::Module* d = detmap->GetModule(n);
  ssiz_t len, w = 0;
  ostr << d->crate; w = ostr.str().size();
  ostr << " ";
  len = ostr.str().size();
  ostr << d->slot; w = max(ostr.str().size()-len,w);
  ostr << " ";
  len = ostr.str().size();
  ostr << d->lo;   w = max(ostr.str().size()-len,w);
  ostr << " ";
  len = ostr.str().size();
  ostr << d->hi;   w = max(ostr.str().size()-len,w);
  if( extras >= 1 ) {
    ostr << " ";
    len = ostr.str().size();
    ostr << d->first; w = max(ostr.str().size()-len,w);
  }
  if( extras >= 2 ) {
    ostr << " ";
    len = ostr.str().size();
    ostr << d->GetModel(); w = max(ostr.str().size()-len,w);
  }
  return Value_t(ostr.str(), 4+extras, w);
}

//-----------------------------------------------------------------------------
template<> inline
Value_t MakeValue( const THaDetMap* detmap, int extras )
{
  ostringstream ostr;
  ssiz_t w = 0;
  int nelem = 0;
  for( Int_t i = 0; i < detmap->GetSize(); ++i ) {
    Value_t v = MakeDetmapElemValue( detmap, i, extras );
    ostr << v.value;
    w = max(w,v.width);
    nelem += v.nelem;
    if( i+1 != detmap->GetSize() ) ostr << " ";
  }
  return Value_t(ostr.str(), nelem, w);
}

//-----------------------------------------------------------------------------
template<> inline
Value_t MakeValue( const TVector3* vec3, int )
{
  Double_t darr[3];
  vec3->GetXYZ( darr );
  return MakeValue( darr, 3 );
}

//-----------------------------------------------------------------------------
// Define data structures for local caching of database parameters

struct DBvalue {
  DBvalue( const string& valstr, time_t start, const string& ver = string(),
	   int maxv = 0 )
    : value(valstr), validity_start(start), version(ver), nelem(0),
      width(0), max_per_line(maxv)
  {
    istringstream istr(value); string item;
    while( istr >> item ) {
      width = max( width, item.length() );
      ++nelem;
    }
  }
  DBvalue( const Value_t& valobj, time_t start, const string& ver = string(),
	   int max = 0 )
    : value(valobj.value), validity_start(start), version(ver),
      nelem(valobj.nelem), width(valobj.width), max_per_line(max) {}
  DBvalue( time_t start, const string& ver = string() )
    : validity_start(start), version(ver), nelem(0), width(0), max_per_line(0) {}
  string value;
  time_t validity_start;
  string version;
  int    nelem;
  ssiz_t width;
  int    max_per_line;    // Number of values per line (for formatting text db)
  // Order values by validity start time, then version
  bool operator<( const DBvalue& rhs ) const {
    if( validity_start < rhs.validity_start ) return true;
    if( validity_start > rhs.validity_start ) return false;
    return version < rhs.version;
  }
  bool operator==( const DBvalue& rhs ) const {
    return validity_start == rhs.validity_start && version == rhs.version;
  }
};

typedef set<DBvalue> ValSet_t;

struct KeyAttr_t {
  KeyAttr_t() : isCopy(false) {}
  bool isCopy;
  string comment;
  ValSet_t values;
};

typedef map<string, KeyAttr_t > DB;
typedef map<string, string> StrMap_t;
typedef multimap<string, string> MStrMap_t;
typedef MStrMap_t::iterator iter_t;

static DB gDB;
static StrMap_t gKeyToDet;
static MStrMap_t gDetToKey;

//-----------------------------------------------------------------------------
static void DumpMap( ostream& os = std::cout )
{
  // Dump contents of the in-memory database to given output

  os << "------ Dump of keys in gDB:" << endl;
  for( DB::const_iterator it = gDB.begin(); it != gDB.end(); ++it ) {
    DB::value_type item = *it;
    for( ValSet_t::const_iterator jt = item.second.values.begin();
	 jt != item.second.values.end(); ++jt ) {
      const DBvalue& val = *jt;
      os << item.first << " (" << format_time(val.validity_start);
      if( !val.version.empty() )
	os << ", \"" << val.version << "\"";
      os << ") = ";
      os << val.value << endl;
    }
  }
  os << "------ Dump of dets in gDetToKey:" << endl;
  for( MStrMap_t::const_iterator it = gDetToKey.begin(); it != gDetToKey.end();
       ++it ) {
    os << it->first << ": " << it->second << endl;
  }
}

//-----------------------------------------------------------------------------
static int PruneMap()
{
  // Remove duplicate entries for the same key and consecutive timestamps

  for( DB::iterator it = gDB.begin(); it != gDB.end(); ++it ) {
    ValSet_t& vals = it->second.values;
    ValSet_t::iterator jt = vals.begin();
    while( jt != vals.end() ) {
      ValSet_t::iterator kt = jt;
      ++kt;
      while( kt != vals.end() && kt->value == jt->value )
	vals.erase( kt++ );
      jt = kt;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
static int WriteAllKeysForTime( ofstream& ofs,
				const iter_t& first, const iter_t& last,
				time_t tstamp, bool find_first = false )
{
  // Write all keys in the range [first,last) to 'ofs' whose timestamp is exactly
  // 'tstamp'. If 'find_first' is true, modify the logic: If the key has a time-
  // stamp equal to 'tstamp', use it (as before), but if such a timestamp does
  // not exist, use the largest timestamp preceding 'tstamp' (validity started
  // before this time, but extends into it).

  int nwritten = 0;
  bool header_done = false;
  DBvalue tstamp_val(tstamp);
  for( iter_t dt = first; dt != last; ++dt ) {
    const string& key = dt->second;
    DB::const_iterator jt = gDB.find( key );
    assert( jt != gDB.end() );
    const KeyAttr_t& attr = jt->second;
    if( attr.isCopy )
      continue;
    const ValSet_t& vals = attr.values;

    ValSet_t::const_iterator vt;
    if( find_first ) {
      // Find the last element that is not greater than tstamp
      vt = vals.upper_bound(tstamp_val);
      if( vt == vals.begin() )
	continue;
      --vt;
    } else {
      // Find the exact tstamp
      vt = vals.find(tstamp_val);
      if( vt == vals.end() )
	continue;
    }

    // Write timestamp header for this group of keys
    if( !header_done ) {
      if( tstamp > 0 )
	ofs << format_tstamp(tstamp) << endl << endl;
      header_done = true;
    }

    // Write the key, pretty-printing if requested
    ofs << key << " =";
    int ncol = vt->max_per_line;
    if( ncol == 0 )
      ofs << " " << vt->value << endl;
    else {
      // Tokenize on whitespace
      bool warn = (ncol > 0);
      ncol = std::abs(ncol);
      stringstream istr( vt->value.c_str() );
      string val;
      int num_to_do = ncol, nelem = 0;
      ofs << endl;
      while( istr >> val ) {
	if( num_to_do == ncol )
	  ofs << " "; // New line indentation
	assert( vt->width > 0 );
	ofs << setw(vt->width + 2);
	ofs << val;
	++nelem;
	if( --num_to_do == 0 ) {
	  ofs << endl;
	  num_to_do = ncol;
	}
	// else {
	//   ofs << " ";
	// }
      }
      if( num_to_do != ncol ) {
	if( warn ) {
	  cerr << "Warning: number of elements of key " << key
	       << " does not divide evenly by requested number of columns,"
	       << " nelem/ncol = " << nelem << "/" << ncol << endl;
	}
	ofs << endl;
      }
    }
    ++nwritten;
  }
  if( nwritten > 0 )
    ofs << endl;

  return nwritten;
}

//-----------------------------------------------------------------------------
static int WriteFileDB( const string& target_dir, const vector<string>& subdirs )
{
  // Write all accumulated database keys in gDB to files in 'target_dir'.
  // If --preserve-subdirs was specified, split the information over
  // the date-coded directories in 'subdirs'.

  set<time_t> dir_times;
  map<time_t,string> dir_names;
  if( !do_subdirs || subdirs.empty() ) {
    dir_times.insert(0);
    dir_names.insert(make_pair(0,target_dir));
  } else {
    for( vector<string>::size_type i = 0; i < subdirs.size(); ++i ) {
      time_t date;
      if( IsDBSubDir(subdirs[i],date) ) {
	dir_times.insert(date);
	dir_names.insert( make_pair(date, MakePath(target_dir,subdirs[i])) );
      }
    }
  }
  siter_t lastdt = dir_times.insert( numeric_limits<time_t>::max() ).first;

  // Consider each detector in turn
  for( iter_t it = gDetToKey.begin(); it != gDetToKey.end(); ) {
    const string& det = it->first;

    // Find all keys for this detector
    pair<iter_t,iter_t> range = gDetToKey.equal_range( det );

    // Accumulate all timestamps for this detector
    set<time_t> tstamps;
    for( iter_t kt = range.first; kt != range.second; ++kt ) {
      const string& key = kt->second;
      DB::const_iterator jt = gDB.find( key );
      assert( jt != gDB.end() );
      const KeyAttr_t& attr = jt->second;
      if( attr.isCopy )
	continue;
      const ValSet_t& vals = attr.values;
      for( ValSet_t::const_iterator vt = vals.begin(); vt != vals.end();
	   ++vt ) {
	tstamps.insert( vt->validity_start );
      }
    }
    if( tstamps.empty() ) {
      it = range.second;
      continue;
    }

    // For each subdirectory, write keys within that directory's time range
    for( siter_t dt = dir_times.begin(); dt != lastdt; ) {
      time_t dir_from = *dt, dir_until = *(++dt);

      if( *tstamps.begin() >= dir_until )
	continue;   // No values for this diretcory time range

      // Build the output file name and open the file
      map<time_t,string>::iterator nt = dir_names.find(dir_from);
      assert( nt != dir_names.end() );
      const string& subdir = nt->second;
      string fname = subdir + "/db_" + det + ".dat";
      ofstream ofs( fname.c_str() );
      if( !ofs ) {
	stringstream ss("Error opening ",ios::out|ios::app);
	ss << fname;
	perror(ss.str().c_str());
	return 1;
      }

      // Because of the search logic for subdirectories, we have to carry keys
      // whose validity started before this directory's time and extends
      // into its range. To do this, we need a different key finding logic,
      // enabled with the find_first parameter of WriteAllKeysForTime.
      int nw = WriteAllKeysForTime( ofs, range.first, range.second,
				    dir_from, true );

      // All following keys are found and written using their exact time stamp
      for( siter_t tt = tstamps.upper_bound(dir_from); tt != tstamps.end() &&
	     *tt < dir_until; ++tt ) {
	nw += WriteAllKeysForTime( ofs, range.first, range.second, *tt );
      }
      ofs.close();
      // Don't create empty files (should never happen)
      assert( nw > 0 );
      // if( nw == 0 ) {
      //	unlink( fname.c_str() );
      // }
    }
    it = range.second;
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Common detector data
class Detector {
public:
  Detector( const string& name )
    : fName(name), fDBName(name), fDetMap(new THaDetMap),
      fDetMapHasLogicalChan(true), fDetMapHasModel(false),
      fNelem(0), fAngle(0) /*, fXax(1.,0,0), fYax(0,1.,0), fZax(0,0,1.) */{
    fSize[0] = fSize[1] = fSize[2] = 0.;
    string::size_type pos = fName.find('.');
    if( pos == string::npos )
      pos = 0;
    else
      ++pos;
    fRealName = fName.substr(pos);
  }
  Detector( const Detector& rhs )
    : fName(rhs.fName), fDBName(rhs.fDBName), fRealName(rhs.fRealName),
      fConfig(rhs.fConfig), fDetMapHasLogicalChan(rhs.fDetMapHasLogicalChan),
      fDetMapHasModel(rhs.fDetMapHasModel),
      fNelem(rhs.fNelem), fAngle(rhs.fAngle), fOrigin(rhs.fOrigin),
      fDefaults(rhs.fDefaults)
  {
    memcpy( fSize, rhs.fSize, 3*sizeof(fSize[0]) );
    fDetMap = new THaDetMap(*rhs.fDetMap);
  }
  Detector& operator=( const Detector& rhs ) {
    if( this != &rhs ) {
      fName = rhs.fName; fDBName = rhs.fDBName; fRealName = rhs.fRealName;
      fConfig = rhs.fConfig; fDetMapHasLogicalChan = rhs.fDetMapHasLogicalChan,
      fDetMapHasModel = rhs.fDetMapHasModel; fNelem = rhs.fNelem;
      fAngle = rhs.fAngle; fOrigin = rhs.fOrigin; fDefaults = rhs.fDefaults;
      memcpy( fSize, rhs.fSize, 3*sizeof(fSize[0]) );
      delete fDetMap;
      fDetMap = new THaDetMap(*rhs.fDetMap);
      fErrmsg.Clear();
    }
    return *this;
  }
  virtual ~Detector() { delete fDetMap; fDetMap = 0; }

  virtual void Clear() {
    fConfig = ""; fDetMap->Clear();
    fDetMapHasModel = false;
    fSize[0] = fSize[1] = fSize[2] = 0.;
    fAngle = 0; fOrigin.SetXYZ(0,0,0);
  }
  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until ) = 0;
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const = 0;
  virtual bool SupportsTimestamps()  const { return false; }
  virtual bool SupportsVariations()  const { return false; }
  virtual void RegisterDefaults();
  virtual int  PurgeAllDefaultKeys();

  void SetDBName( const string& s ) { fDBName = s; }
  const string& GetDBName() const { return fDBName; }

protected:
  virtual int AddToMap( const string& key, const Value_t& value, time_t start,
			const string& version = string(), int max = 0 ) const;
  // void DefineAxes( Double_t rot ) {
  //   fXax.SetXYZ( TMath::Cos(rot), 0.0, TMath::Sin(rot) );
  //   fYax.SetXYZ( 0.0, 1.0, 0.0 );
  //   fZax = fXax.Cross(fYax);
  // }
  const char* Here( const char* here ) {
    if( !here ) return "";
    fErrmsg  = here;
    fErrmsg += "(file=\"";
    fErrmsg += current_filename.c_str();
    fErrmsg += "\")";
    return fErrmsg.Data();
  }
  int ErrPrint( FILE* fi, const char* here )
  {
    ostringstream msg;
    msg << "Unexpected data at file position " << ftello(fi);
    Error( Here(here), "%s", msg.str().c_str() );
    return kInitError;
  }
  Bool_t CheckDetMapInp( Int_t crate, Int_t slot, Int_t lo, Int_t hi,
      const char* buff, const char* here )
  {
    Bool_t is_err = ( crate <= 0 || crate > Decoder::MAXROC  ||
        slot  <= 0 || slot  > Decoder::MAXSLOT ||
        lo < 0 || lo > std::numeric_limits<UShort_t>::max() ||
        hi < 0 || lo > std::numeric_limits<UShort_t>::max() );
    if( is_err )
      Error( Here(here), "Illegal detector map parameter, line: %s", buff );
    return is_err;
  }

  // Bits for flags parameter of ReadBlock()
  enum EReadBlockFlags {
    kQuietOnErrors      = BIT(0),
    kQuietOnTooMany     = BIT(1),
    kQuietOnTooFew      = BIT(2),
    kNoNegativeValues   = BIT(3),
    kRequireGreaterZero = BIT(4),
    kDisallowDataGuess  = BIT(5),
    kWarnOnDataGuess    = BIT(6),
    kErrOnTooManyValues = BIT(7),
    kStopAtNval         = BIT(8),
    kStopAtSection      = BIT(9)
  };
  enum EReadBlockRetval {
    kSuccess = 0, kNoValues = -1, kTooFewValues = -2, kTooManyValues = -3,
    kNegativeFound = -4, kLessEqualZeroFound = -5, kBadInput = -6,
    kFileError = -7 };
  template <class T>
  EReadBlockRetval ReadBlock( FILE* fi, T* data, int nval, const char* here,
      int flags = 0 );

  string      fName;    // Detector "name", actually the prefix without trailing dot
  string      fDBName;  // Database file name for this detector
  string      fRealName;// Actual detector name (top level dropped)
  TString     fConfig;  // TString for compatibility with old API
  THaDetMap*  fDetMap;
  bool        fDetMapHasLogicalChan;
  bool        fDetMapHasModel;
  Int_t       fNelem;
  Double_t    fSize[3];
  Double_t    fAngle;
  TVector3    fOrigin;//, fXax, fYax, fZax;

  StrMap_t    fDefaults;

  mutable string fNelemName;

  static bool TestBit(UInt_t flags, UInt_t bit) { return (flags & bit) != 0; }

private:
  TString     fErrmsg;
};

//-----------------------------------------------------------------------------
// CopyFile - pseudo-detector for run database and database files that
// are already in the new format. Extracts all defined keys and copies
// file essentially verbatim.

class CopyFile : public Detector {
public:
  CopyFile( const string& name, bool doingFileCopy = true )
    : Detector(name), fDoingFileCopy(doingFileCopy) {}

  virtual void Clear();
  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "CopyFile"; }

protected:
  virtual int AddToMap( const string& key, const Value_t& value, time_t start,
			const string& version = string(), int max = 0 ) const;

  bool fDoingFileCopy; // If true, mark DB values with isCopy = true
  DB   fDB;
};

//-----------------------------------------------------------------------------
// Cherenkov
class Cherenkov : public Detector {
public:
  Cherenkov( const string& name )
    : Detector(name), fOff(0), fPed(0), fGain(0) {}
  virtual ~Cherenkov() { DeleteArrays(); }

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "Cherenkov"; }
  virtual void RegisterDefaults();

private:
  // Calibrations
  Float_t *fOff, *fPed, *fGain;

  void DeleteArrays() { delete [] fOff; delete [] fPed; delete [] fGain; }
};

//-----------------------------------------------------------------------------
// Scintillator
class Scintillator : public Detector {
public:
  Scintillator( const string& name )
    : Detector(name), fTdc2T(0), fCn(3e8), fAdcMIP(0), fAttenuation(0),
      fResolution(5e-10), fNTWalkPar(0),
      fLOff(0), fROff(0), fLPed(0), fRPed(0), fLGain(0),
      fRGain(0), fTWalkPar(0), fTrigOff(0), fHaveExtras(false) {}
  virtual ~Scintillator() { DeleteArrays(); }

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "Scintillator"; }
  virtual bool SupportsTimestamps()  const { return true; }
  virtual bool SupportsVariations()  const { return true; }
  virtual void RegisterDefaults();

private:
  // Configuration
  Double_t  fTdc2T, fCn, fAdcMIP, fAttenuation, fResolution;
  Int_t     fNTWalkPar;
  // Calibrations
  Double_t  *fLOff, *fROff, *fLPed, *fRPed, *fLGain, *fRGain;
  Double_t  *fTWalkPar, *fTrigOff;
  bool      fHaveExtras;

  void DeleteArrays() {
    delete [] fLOff; delete [] fROff; delete [] fLPed; delete [] fRPed;
    delete [] fLGain; delete [] fRGain; delete [] fTWalkPar; delete [] fTrigOff;
  }
};

//-----------------------------------------------------------------------------
// Shower
class Shower : public Detector {
public:
  Shower( const string& name )
    : Detector(name), fNcols(0), fNrows(0), fEmin(0),fMaxCl(kMaxInt),
      fPed(0), fGain(0) { fDetMapHasLogicalChan = false; }
  virtual ~Shower() { DeleteArrays(); }

  virtual void Clear() { Detector::Clear(); fMaxCl = -1; }
  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "Shower"; }
  virtual bool SupportsTimestamps()  const { return true; }
  virtual bool SupportsVariations()  const { return true; }
  virtual void RegisterDefaults();

private:
  // Mapping
  vector<unsigned int> fChanMap;

  // Geometry, configuration
  Int_t      fNcols;
  Int_t      fNrows;
  Float_t    fXY[2], fDXY[2];
  Float_t    fEmin;
  Int_t      fMaxCl;   // Maximum number of clusters, used by BigBite shower code

  // Calibrations
  Float_t   *fPed, *fGain;

  void DeleteArrays() { delete [] fPed; delete [] fGain; }
};

//-----------------------------------------------------------------------------
// TotalShower
class TotalShower : public Detector {
public:
  TotalShower( const string& name ) : Detector(name) {}

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "TotalShower"; }

private:
  // Geometry, configuration
  Float_t    fMaxDXY[2];
};

//-----------------------------------------------------------------------------
// BPM
class BPM : public Detector {
public:
  BPM( const string& name ) : Detector(name), fCalibRot(0) {}

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "BPM"; }
  virtual void RegisterDefaults();

private:
  Double_t fCalibRot;
  Double_t fPed[4];
  Double_t fRot[4];
};

//-----------------------------------------------------------------------------
// Raster
class Raster : public Detector {
public:
  Raster( const string& name ) : Detector(name) {}

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "Raster"; }
  virtual bool SupportsTimestamps()  const { return true; }
  virtual void RegisterDefaults();

private:
  Double_t fFreq[2], fRPed[2], fSPed[2];
  Double_t fZpos[3];
  Double_t fCalibA[6], fCalibB[6], fCalibT[6];
};

//-----------------------------------------------------------------------------
// CoincTime
class CoincTime : public Detector {
public:
  CoincTime( const string& name ) : Detector(name) {
    fTdcOff[0] = fTdcOff[1] = 0;
  }
  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "CoincTime"; }
  virtual void RegisterDefaults();

private:
  Double_t fTdcRes[2], fTdcOff[2];
  TString fTdcLabels[2];
};

//-----------------------------------------------------------------------------
// TriggerTime
class TriggerTime : public Detector {
public:
  TriggerTime( const string& name )
    : Detector(name), fGlOffset(0), fTDCRes(-0.5e-9) {}

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "TriggerTime"; }

private:
  Double_t fGlOffset, fTDCRes;
  vector<string> fTrgDef;
};

//-----------------------------------------------------------------------------
// VDC
class VDC : public Detector {
public:
  VDC( const string& name )
    : Detector(name), fErrorCutoff(kBig), fNumIter(0), fCommon(0) {}

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "VDC"; }
  virtual void RegisterDefaults();

  UInt_t GetCommon() const { return fCommon; }

private:
  typedef THaVDC::THaMatrixElement THaMatrixElement;
  enum { kPORDER = 7 };
  enum ECoordType { kTransport, kRotatingTransport };
  enum EFPMatrixElemTag { T000 = 0, Y000, P000 };

  // initial matrix elements
  vector<THaMatrixElement> fTMatrixElems;
  vector<THaMatrixElement> fDMatrixElems;
  vector<THaMatrixElement> fPMatrixElems;
  vector<THaMatrixElement> fPTAMatrixElems; // involves abs(theta_fp)
  vector<THaMatrixElement> fYMatrixElems;
  vector<THaMatrixElement> fYTAMatrixElems; // involves abs(theta_fp)
  vector<THaMatrixElement> fFPMatrixElems;  // matrix elements used in
					    // focal plane transformations { T, Y, P }
  vector<THaMatrixElement> fLMatrixElems;   // Path-length corrections (meters)

  Double_t fErrorCutoff;
  Int_t    fNumIter;
  string   fCoordType;

  UInt_t fCommon;

  class Plane : public Detector {
  public:
    friend class VDC;
    Plane( const string& name, VDC* vdc )
      : Detector(name), fZ(0), fWBeg(0), fWSpac(0), fWAngle(0),
        fDriftVel(0), fTDCRes(0), fT0Resolution(0),
        fMinTdiff(0), fMaxTdiff(kBig), fMinClustSize(0),
        fMaxClustSpan(kMaxInt), fNMaxGap(0), fMinTime(0),
        fMaxTime(kMaxInt), fVDC(vdc)
    { fTTDPar.resize(9); }

    virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
    virtual int Save( time_t start, const string& version = string() ) const;
    virtual const char* GetClassName() const { return "VDC::Plane"; }

  private:
    Double_t fZ, fWBeg, fWSpac, fWAngle, fDriftVel;
    vector<Double_t> fTDCOffsets;
    vector<Int_t> fBadWires;
    Double_t fTDCRes, fT0Resolution, fMinTdiff, fMaxTdiff;
    vector<Double_t> fTTDPar;
    Int_t fMinClustSize, fMaxClustSpan, fNMaxGap, fMinTime, fMaxTime;
    string fTTDConv, fDescription;

    VDC* fVDC;
  };
  vector<Plane> fPlanes;
};

//-----------------------------------------------------------------------------
// Flags for common VDC::Plane parameters, set in bitmask VDC::fCommon

enum ECommonFlag { kNelem        = BIT(0),  kWSpac        = BIT(1),
		   kDriftVel     = BIT(2),  kMinTime      = BIT(3),
		   kMaxTime      = BIT(4),  kTDCRes       = BIT(5),
		   kTTDConv      = BIT(6),  kTTDPar       = BIT(7),
		   kT0Resolution = BIT(8),  kMinClustSize = BIT(9),
		   kMaxClustSpan = BIT(10), kNMaxGap      = BIT(11),
		   kMinTdiff     = BIT(12), kMaxTdiff     = BIT(13),
		   kFlagsEnd     = 0 };
inline ECommonFlag& operator++( ECommonFlag& e )
{
  switch (e) {
  case kNelem:        return e=kWSpac;
  case kWSpac:        return e=kDriftVel;
  case kDriftVel:     return e=kMinTime;
  case kMinTime:      return e=kMaxTime;
  case kMaxTime:      return e=kTDCRes;
  case kTDCRes:       return e=kTTDConv;
  case kTTDConv:      return e=kTTDPar;
  case kTTDPar:       return e=kT0Resolution;
  case kT0Resolution: return e=kMinClustSize;
  case kMinClustSize: return e=kMaxClustSpan;
  case kMaxClustSpan: return e=kNMaxGap;
  case kNMaxGap:      return e=kMinTdiff;
  case kMinTdiff:     return e=kMaxTdiff;
  case kMaxTdiff:     return e=kFlagsEnd;
  case kFlagsEnd: default:
    throw std::range_error("Increment past end of range");
  }
}

//-----------------------------------------------------------------------------
// Global maps for detector types and names
enum EDetectorType { kNone = 0, kKeep, kCopyFile, kCherenkov, kScintillator,
		     kShower, kTotalShower, kBPM, kRaster, kCoincTime,
		     kTriggerTime, kVDC, kDecData };
typedef map<string,EDetectorType> NameTypeMap_t;
static NameTypeMap_t detname_map;
static NameTypeMap_t dettype_map;

struct StringToType_t {
  const char*   name;
  EDetectorType type;
};

//-----------------------------------------------------------------------------
static Detector* MakeDetector( EDetectorType type, const string& name )
{
  Detector* det = 0;
  switch( type ) {
  case kNone:
  case kKeep:
    return 0;
  case kCopyFile:
    return new CopyFile(name,do_file_copy);
  case kCherenkov:
    return new Cherenkov(name);
  case kScintillator:
    return new Scintillator(name);
  case kShower:
    return new Shower(name);
  case kTotalShower:
    return new TotalShower(name);
  case kBPM:
    return new BPM(name);
  case kRaster:
    return new Raster(name);
  case kCoincTime:
    return new CoincTime(name);
  case kTriggerTime:
    return new TriggerTime(name);
  case kVDC:
    return new VDC(name);
  case kDecData:
    return 0; //TODO
  }
  return det;
}

//-----------------------------------------------------------------------------
static void DefineTypes()
{
  // Set up mapping of detector type names to EDetectorType constants

  StringToType_t deftypes[] = {
    // Names must be all lowercase for case-insensitive find() later
    { "scintillator",   kScintillator },
    { "cherenkov",      kCherenkov },
    { "cerenkov",       kCherenkov },   // common misspelling
    { 0,                kNone }
  };
  for( StringToType_t* item = deftypes; item->name; ++item ) {
#ifndef NDEBUG
    pair<NameTypeMap_t::iterator, bool> ins =
#endif
      dettype_map.insert( make_pair(string(item->name),item->type) );
    assert( ins.second ); // else typo in definition of deftypes[]
  }
}

//-----------------------------------------------------------------------------
static int ReadMapfile( const char* filename )
{
  ifstream ifs(filename);
  if( !ifs ) {
    cerr << "Error opening mapfile \"" << filename << "\""  << endl;
    return -1;
  }
  DefineTypes();
  detname_map.clear();
  for( string line; getline(ifs,line); ) {
    ssiz_t pos = line.find_first_not_of(" \t");
    if( pos == string::npos || line[pos] == '#' )
      continue;
    istringstream istr(line);
    string name, type;
    istr >> name;
    istr >> type;
    if( type.empty() ) {
      cerr << "Mapfile: Missing detector type for name = \""
	   << name << "\"" << endl;
      return 1;
    }
    string ltype(type);
    transform( ALL(type), ltype.begin(), (int(*)(int))tolower );
    NameTypeMap_t::iterator it = dettype_map.find(ltype);
    if( it == dettype_map.end() ) {
      cerr << "Mapfile: undefined detector type = \"" << type << "\""
	   << "for name \"" << name << "\"" << endl;
      return 2;
    }
    // Add new name/type to map
    pair<NameTypeMap_t::iterator, bool> ins =
      detname_map.insert( make_pair(name,it->second) );
    // If name already exists, generate an error, but only if the duplicate
    // line has an inconsistent type. This allows trivial duplicates (repeated
    // lines) in the mapfile.
    if( !ins.second && ins.first->second != it->second ) {
      cerr << "Mapfile: duplicate, inconsistent definition of detector name "
	   << "= \"" << name << "\"" << endl;
      return 3;
    }
  }
  ifs.close();
  dettype_map.clear(); // no longer needed
  return 0;
}

//-----------------------------------------------------------------------------
static void DefaultMap()
{
  // Set up common detector names as defaults

  StringToType_t defaults[] = {
    //TODO
    { "R.cer",      kCherenkov },
    { "R.aero",     kCherenkov },
    { "R.aero1",    kCherenkov },
    { "R.aero2",    kCherenkov },
    { "L.cer",      kCherenkov },
    { "L.aero1",    kCherenkov },
    { "L.aero2",    kCherenkov },
    { "L.a1",       kCherenkov },
    { "R.s0",       kScintillator },
    { "R.s1",       kScintillator },
    { "R.s2",       kScintillator },
    { "L.s0",       kScintillator },
    { "L.s1",       kScintillator },
    { "L.s2",       kScintillator },
    { "TA.E",       kScintillator },
    { "TA.dE",      kScintillator },
    { "R.sh",       kShower },
    { "R.ps",       kShower },
    { "L.sh",       kShower },
    { "L.ps",       kShower },
    { "L.prl1",     kShower },
    { "L.prl2",     kShower },
    { "L.CSR",      kShower },  // NaI detector CSR experiment 2007
    { "BB.ts.ps",   kShower },
    { "BB.ts.sh",   kShower },
    { "R.ts",       kTotalShower },
    { "L.ts",       kTotalShower },
    { "BB.ts",      kTotalShower },
    { "CT",         kCoincTime },
    { "R.vdc",      kVDC },
    { "L.vdc",      kVDC },
    // Ignore top-level Beam apparatus db files - these are actually never read.
    // Instead, the apparatus's detector read db_urb.BPMA.dat etc.
    { "beam",       kNone },
    { "R_beam",     kNone },
    { "L_beam",     kNone },
    { "rb",         kNone },
    { "Rrb",        kNone },
    { "Lrb",        kNone },
    { "urb",        kNone },
    { "Rurb",       kNone },
    { "Lurb",       kNone },
    // Apparently Bodo's database scheme was so confusing that people started
    // defining BPM databases for rastered beams, which only use the Raster
    // detector, and Raster databases for unrastered beams, which only use
    // BPM detectors. Ignore all these since they are never used.
    { "rb.BPMA",    kNone },
    { "rb.BPMB",    kNone },
    { "Lrb.BPMA",   kNone },
    { "Lrb.BPMB",   kNone },
    { "Rrb.BPMA",   kNone },
    { "Rrb.BPMB",   kNone },
    { "urb.Raster", kNone },
    { "Lurb.Raster",kNone },
    { "Rurb.Raster",kNone },
    // Actually used beam detectors follow here
    { "urb.BPMA",   kBPM },
    { "urb.BPMB",   kBPM },
    { "Rurb.BPMA",  kBPM },
    { "Rurb.BPMB",  kBPM },
    { "Lurb.BPMA",  kBPM },
    { "Lurb.BPMB",  kBPM },
    { "BBurb.BPMA", kBPM },
    { "BBurb.BPMB", kBPM },
    { "rb.Raster",  kRaster },
    { "Rrb.Raster", kRaster },
    { "Lrb.Raster", kRaster },
    { "BBrb.Raster",kRaster },
    // Databases already in new format
    { "run",        kCopyFile },
    { "hel",        kCopyFile },
    { "he3",        kCopyFile },
    { "he3_R",      kCopyFile },
    { "vdceff",     kCopyFile },
    { "L.fpp",      kCopyFile },
    { "R.fpp",      kCopyFile },
    { "BB.mwdc",    kCopyFile },
    { "L.gem",      kCopyFile },
    { "L.rich",     kCopyFile },
    { 0,            kNone }
  };
  for( StringToType_t* item = defaults; item->name; ++item ) {
#ifndef NDEBUG
    pair<NameTypeMap_t::iterator, bool> ins =
#endif
      detname_map.insert( make_pair(string(item->name),item->type) );
    assert( ins.second ); // else typo in definition of defaults[]
  }
}

//-----------------------------------------------------------------------------
static inline string GetDetName( const string& fname )
{
  assert( fname.size() > 7 );
  ssiz_t pos = fname.rfind('/');
  assert( pos == string::npos || pos+7 < fname.size() );
  if( pos == string::npos )
    pos = 0;
  else
    pos++;
  return fname.substr(pos+3,fname.size()-pos-7);
}

//-----------------------------------------------------------------------------
template<typename Action>
static int ForAllFilesInDir( const string& sdir, Action action, int depth = 0 )
{
  int n_add = 0;
  errno = 0;

  if( sdir.empty() )
    return 0;

  // Open given directory
  const char* cdir = sdir.c_str();
  DIR* dir = opendir(cdir);
  if( !dir || errno ) {
    stringstream ss("Error opening ",ios::out|ios::app);
    //    ss << action.GetDescription();
    ss << sdir;
    perror(ss.str().c_str());
    return -1;
  }

  // Loop over all directory entries and pass their names to action
  struct dirent *ent = 0;
  int err = 0;
  bool unfinished;
  errno = 0;
  do {
    unfinished = false;
    while( (ent = readdir(dir)) ) {
      // Skip trivial file names
      string fname(ent->d_name);
      if( fname.empty() || fname == "." || fname == ".." )
	continue;
      // Operate on the name
      if( (err = action(sdir, fname, depth)) < 0 )
	break;
      n_add += err;
      err = 0;
    }
    if( errno ) {
      stringstream ss("Error reading directory ",ios::out|ios::app);
      ss << sdir;
      perror(ss.str().c_str());
      closedir(dir);
      return -1;
    }
    // In case 'action' caused the directory contents to change, rewind the
    // directory and scan it again (needed for MacOS HFS file systems, perhaps
    // elsewhere too)
    if( action.MustRewind() ) {
      rewinddir(dir);
      unfinished = true;
    }
  } while( unfinished );

  closedir(dir);
  if( err )
    return err;
  return n_add;
}

//-----------------------------------------------------------------------------
class CopyDBFileName {
public:
  CopyDBFileName( FilenameMap_t& filenames, vector<string>& subdirs,
                  const time_t start_time )
    : fFilenames(filenames), fSubdirs(subdirs), fStart(start_time) {}

  int operator() ( const string& dir, const string& fname, int depth )
  {
    int n_add = 0;
    string fpath = MakePath( dir, fname );
    const char* cpath = fpath.c_str();
    struct stat sb;
    if( stat(cpath, &sb) || errno ) {
      perror(cpath);
      return -1;
    }

    // Record regular files whose names match db_*.dat
    if( S_ISREG(sb.st_mode) && IsDBFileName(fname) ) {
      fFilenames[GetDetName(fpath)].insert( Filenames_t(fStart,fpath) );
      n_add = 1;
    }

    // Recurse down one level into valid subdirectories ("YYYYMMDD" and "DEFAULT")
    else if( S_ISDIR(sb.st_mode) && depth == 0 ) {
      time_t date;
      if( IsDBSubDir(fname,date) ) {
        int ret =
          ForAllFilesInDir(fpath, CopyDBFileName(fFilenames,fSubdirs,date),
              depth+1);
        if( ret < 0 )
          return ret;
        n_add = ret;
        // if( n_add > 0 )  // Must keep the directory, even if empty!
        fSubdirs.push_back(fname);
      }
    }
    return n_add;
  }
  bool MustRewind() { return false; }
private:
  FilenameMap_t&    fFilenames;
  vector<string>&   fSubdirs;
  time_t            fStart;
};

//-----------------------------------------------------------------------------
class DeleteDBFile {
public:
  DeleteDBFile( bool do_all = false ) : fDoAll(do_all), fMustRewind(false) {}

  int operator() ( const string& dir, const string& fname, int depth )
  {
    time_t date;
    struct stat sb;

    string fpath = MakePath(dir, fname);
    const char* cpath = fpath.c_str();
    bool is_db_subdir = IsDBSubDir(fname, date);

    if( lstat(cpath, &sb) ) { // Use lstat here - we don't want to follow links!
      perror(cpath);
      return -1;
    }
    // Recurse down into valid subdirectories ("YYYYMMDD" and "DEFAULT")
    if( S_ISDIR(sb.st_mode) ) {
      if( depth > 0 || is_db_subdir ) {
        // Wipe everything in date-coded subdirectories, except "DEFAULT",
        // where we only delete database files
        bool delete_all = (depth > 0 || fname != "DEFAULT");
        if( ForAllFilesInDir(fpath, DeleteDBFile(delete_all), depth+1) )
          return -2;
        int err;
        if( (err = rmdir(cpath)) && errno != ENOTEMPTY ) {
          perror(cpath);
          return -2;
        }
        errno = 0;
        if( !err )
          fMustRewind = true;
      }
    }
    // Non-directory (regular, links) files matching db_*.dat or a subdirectory name
    else if( fDoAll || is_db_subdir || IsDBFileName(fname) ) {
      if( unlink(cpath) ) {
        perror(cpath);
        return -2;
      }
      fMustRewind = true;
    }
    return 0;
  }
  bool MustRewind()
  { if( fMustRewind ) { fMustRewind = false; return true; } else return false; }
private:
  bool fDoAll;
  bool fMustRewind;
};

//-----------------------------------------------------------------------------
static int GetFilenames( const string& srcdir, const time_t start_time,
			 FilenameMap_t& filenames, vector<string>& subdirs )
{
  // Get a list of all database files, based on Podd's search order rules.
  // Keep timestamps info with each file. Reading files from the current
  // directory must be explicitly requested, though.

  assert( !srcdir.empty() );

  return ForAllFilesInDir( srcdir, CopyDBFileName(filenames, subdirs, start_time) );
}

//-----------------------------------------------------------------------------
static int CheckDir( string path, bool writable )
{
  // Check if 'dir' exists and is readable. If 'writable' is true, also
  // check if it is writable. Print error if any test fails.

  errno = 0;

  if( path.empty() ) {
    cerr << "Empty directory name" << endl;
    return -1;
  }

  // Chop trailing '/' from the directory name
  while( !path.empty() && *path.rbegin() == '/' ) {
    path.erase( path.size()-1 );
  }

  struct stat sb;
  const char* cpath = path.c_str();
  int mode = R_OK|X_OK;
  if( writable )  mode |= W_OK;

  if( stat(cpath,&sb) ) {
    return 1;
  }
  if( !S_ISDIR(sb.st_mode) ) {
    return 2;
  }
  if( access(cpath,mode) ) {
    return 3;
  }
  return 0;
}

//-----------------------------------------------------------------------------
static int PrepareOutputDir( const string& topdir, const vector<string>& subdirs )
{
  // Create subdirectories 'subdirs' in 'topdir'.
  // Unless --no-clean is given, remove all database files and subdirectories
  // from 'topdir' first.

  assert( !topdir.empty() );

  const mode_t mode = 0775; // Default mode for directories
  const char* ctop = topdir.c_str();

  // Check if target directory exists and can be written to
  int err = CheckDir(topdir, true);
  switch( err ) {
  case 0:
    break;
  case 1:
    if( mkdir(ctop,mode) ) {
      perror(ctop);
      return 2;
    }
    break;
  case 2:
    cerr << "Output destination " << topdir << " exists but is not a directory. "
	 << "Move it out of the way." << endl;
    return 1;
  default:
    perror(ctop);
    return 1;
  }
  bool top_existed = ( err == 0 );

  // If requested, remove any existing database files and time-stamp directories
  if( do_clean && top_existed ) {
    bool do_delete = true;
    if( do_verify ) {
      cout << "Really delete all database files in \'" << topdir << "\'? " << flush;
      char c;
      cin >> c;
      if( c != 'y' && c != 'Y' )
	do_delete = false;
    }
    if( do_delete ) {
      if( ForAllFilesInDir(topdir, DeleteDBFile()) )
	return 3;
    }
  }

  // Create requested subdirectories
  if( do_subdirs ) {
    for( vector<string>::size_type i = 0; i < subdirs.size(); ++i ) {
      string path = MakePath( topdir, subdirs[i] );
      const char* cpath = path.c_str();
      if( mkdir(cpath,mode) && errno != EEXIST ) {
	perror(cpath);
	return 4;
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
static Int_t GetLine( FILE* file, char* buf, size_t bufsiz, string& line )
{
  // Get a line (possibly longer than 'bufsiz') from 'file' using
  // using the provided buffer 'buf'. Put result into string 'line'.
  // This is similar to std::getline, except that C-style I/O is used.
  // Also, convert all tabs to spaces.
  // Returns 0 on success, or EOF if no more data (or error).

  char* r = buf;
  line.clear();
  line.reserve(bufsiz);
  while( (r = fgets(buf, bufsiz, file)) ) {
    char* c = strchr(buf, '\n');
    if( c )
      *c = '\0';
    // Convert all tabs to spaces
    char *p = buf;
    while( (p = strchr(p,'\t')) ) *(p++) = ' ';
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

//-----------------------------------------------------------------------------
static bool ParseTimestamps( FILE* fi, set<time_t>& timestamps )
{
  // Put all timestamps from file 'fi' in given vector 'timestamps'.
  // If any data lines are present before the first timestamp,
  // return 1, else return 0;

  const size_t LEN = 256;
  char buf[LEN];
  string line;
  bool got_tstamp = false, have_unstamped = false;

  rewind(fi);
  while( GetLine(fi,buf,LEN,line) == 0 ) {
    time_t date;
    if( IsDBdate(line, date) ) {
      timestamps.insert(date);
      got_tstamp = true;
    } else if( !got_tstamp && !IsDBcomment(line) ) {
      have_unstamped = true;
    }
  }

  return have_unstamped;
}

//-----------------------------------------------------------------------------
static int ParseVariations( FILE* fi, vector<string>& variations )
{
  rewind(fi);
  variations.clear();

  return 0;
}

//-----------------------------------------------------------------------------
class MatchesOneOf : public unary_function<string,bool>
{
public:
  MatchesOneOf( multiset<Filenames_t>& filenames ) : fFnames(filenames) {}
  bool operator() ( const string& subdir ) {
    if( subdir == "DEFAULT" )
      return true;
    string chunk = "/" + subdir + "/db_";
    for( fiter_t it = fFnames.begin(); it != fFnames.end(); ++it ) {
      if( it->path.find(chunk) != string::npos )
	return true;
    }
    return false;
  }
private:
  multiset<Filenames_t>& fFnames;
};

//-----------------------------------------------------------------------------
static int InsertDefaultFiles( const vector<string>& subdirs,
			       multiset<Filenames_t>& filenames )
{
  // If there are default files, select the one that the analyzer would pick,
  // i.e. give preference to topdir/DEFAULT/ over topdir/
  Filenames_t defval(0);
  pair<fiter_t,fiter_t> defs = filenames.equal_range(defval);
  int ndef = distance( defs.first, defs.second );
  string deffile;
  switch( ndef ) {
  case 0:
    break;
  case 1:
    deffile = defs.first->path;
    break;
  case 2:
    --defs.second;
    if( defs.first->path.find("/DEFAULT/db_") != string::npos ) {
      deffile = defs.first->path;
      filenames.erase( defs.second );
    } else if( defs.second->path.find("/DEFAULT/db_") != string::npos ) {
      deffile = defs.second->path;
      filenames.erase( defs.first );
    } else {
      cerr << "ERROR: Unrecognized default file location: " << endl
	   << defs.first->path << ", " << defs.second->path << endl
	   << "This is a bug. Call expert." << endl;
      return -1;
    }
    break;
  default:
    cerr << "More than 2 default files " << defs.first->path << "... ??? "
	 << "This is a bug. Call expert." << endl;
    return -1;
    break;
  }
  // If there is a default file and any date-coded subdirectories, we must
  // check if this detector has a file in each such subdirectory. For any
  // subdirectory where there is no file, the default file applies.
  bool have_dated_subdirs = !subdirs.empty() &&
    (subdirs.size() > 1 || subdirs[0] != "DEFAULT");
  if( !deffile.empty() && have_dated_subdirs ) {
    vector<string> missing;
    MatchesOneOf match(filenames);
    remove_copy_if( ALL(subdirs), back_inserter(missing), match );
    if( !missing.empty() ) {
      for( vector<string>::iterator mt = missing.begin(); mt != missing.end();
	   ++mt ) {
	const string& subdir = *mt;
	time_t date;
	bool good = IsDBSubDir(subdir,date);
	assert(good);
	if( !good ) {
	  cerr << "ERROR: subdir = " << subdir << " is not a DB subdir? "
	       << "Should never happen. Call expert." << endl;
	  return -1;
	}
#ifdef NDEBUG
	filenames.insert( Filenames_t(date,deffile) );
#else
	Filenames_t ins(date,deffile);
	assert( filenames.find(ins) == filenames.end() );
	filenames.insert(ins);
#endif
      }
    }
  }
  // Eliminate consecutive identical paths - these would cause the same file
  // to be read multiple times with artificial validity start times at
  // directory boundaries, which would then be deleted again in PruneMap
  fiter_t it = filenames.begin();
  while( it != filenames.end() ) {
    fiter_t jt = it;
    ++jt;
    while( jt != filenames.end() && it->path == jt->path )
      filenames.erase( jt++ );
    it = jt;
  }
  return 0;
}

//-----------------------------------------------------------------------------
static int ExtractKeys( Detector* det, const multiset<Filenames_t>& filenames )
{
  // Extract keys for given detector from the database files in 'filenames'

  fiter_t lastf = filenames.end();
  if( lastf != filenames.begin() )
    --lastf;
  for( fiter_t st = filenames.begin(); st != lastf; ) {
    const string& path = st->path;
    time_t val_from = st->val_start, val_until = (++st)->val_start;

    FILE* fi = fopen( path.c_str(), "r" );
    if( !fi ) {
      stringstream ss("Error opening database file ",ios::out|ios::app);
      ss << path;
      perror(ss.str().c_str());
      continue;
    }
    current_filename = path;

    // Parse the file for any timestamps and "configurations" (=variations)
    set<time_t> timestamps;
    vector<string> variations;
    timestamps.insert(val_from);
    if( det->SupportsTimestamps() ) {
      errno = 0;
      ParseTimestamps(fi, timestamps);
      if( errno ) {
	stringstream ss("Error reading database file ",ios::out|ios::app);
	ss << path;
	perror(ss.str().c_str());
	goto next;
      }
    }
    if( det->SupportsVariations() ) {
      ParseVariations(fi, variations);
    }

    timestamps.insert( numeric_limits<time_t>::max() );
    {
      siter_t it = timestamps.lower_bound( val_from );
      siter_t lastt  = timestamps.lower_bound( val_until );
      for( ; it != lastt; ) {
	time_t date_from = *it, date_until = *(++it);
	if( date_from  < val_from  )  date_from  = val_from;
	if( date_until > val_until )  date_until = val_until;
	rewind(fi);
	det->Clear();
	if( det->ReadDB(fi,date_from,date_until) == 0 ) {
	  // } else {
	  //   cout << "Read " << path << endl;
	  //TODO: support variations
	  det->Save( date_from );
	}
	else {
	  cerr << "Failed to read " << path << " as " << det->GetClassName()
	       << endl;
	}
      }
    }

  next:
    fclose(fi);
  } // end for st = filenames

  return 0;
}

typedef map<time_t,pair<size_t,string> > SectionMap_t;
typedef SectionMap_t::iterator SMiter_t;

//-----------------------------------------------------------------------------
static void AddSection( SectionMap_t& sections, time_t cur_date, size_t ndata,
			string& chunk )
{
  pair<size_t,string>& secdata = sections[cur_date];
  secdata.first = ndata;
  if( secdata.second.empty() )
    secdata.second.swap( chunk );
  else {
    if( *secdata.second.rbegin() != '\n' )
      secdata.second.append( "\n" );
    secdata.second.append( chunk );
    chunk.clear();
  }
}

//-----------------------------------------------------------------------------
static int CopyFiles( const string& target_dir, const vector<string>& subdirs,
		      FilenameMap_t::const_iterator ft )
{
  // Copy source files given in the map 'ft' to 'target_dir' and given set
  // of subdirs. If 'subdirs' is empty or no output subdirs are requested,
  // write all output to 'target_dir'. If files already exist, intelligently
  // merge new keys. Remove any non-reachable time ranges.
  // Should be called with current system timezone set.

  const string& detname = ft->first;
  const multiset<Filenames_t>& filenames = ft->second;
  if( filenames.empty() )
    return 0;  // nothing to do

  fiter_t lastf = filenames.end();
  assert( lastf != filenames.begin() );
  --lastf;

  set<time_t> dir_times;
  map<time_t,string> dir_names;
  if( !do_subdirs || subdirs.empty() ) {
    dir_times.insert(0);
    dir_names.insert(make_pair(0,target_dir));
  } else {
    for( vector<string>::size_type i = 0; i < subdirs.size(); ++i ) {
      time_t date;
      if( IsDBSubDir(subdirs[i],date) ) {
	dir_times.insert(date);
	dir_names.insert( make_pair(date, MakePath(target_dir,subdirs[i])) );
      }
    }
  }
  siter_t lastdt = dir_times.insert( numeric_limits<time_t>::max() ).first;

  // For each output subdirectory, find the files that map into it
  for( siter_t dt = dir_times.begin(); dt != lastdt; ) {
    time_t dir_from = *dt, dir_until = *(++dt);

    multiset<Filenames_t> these_files;
    for( fiter_t st = filenames.begin(); st != lastf; ) {
      fiter_t this_file = st;
      time_t val_from = st->val_start, val_until = (++st)->val_start;
      if( val_from < dir_until && dir_from < val_until )
	these_files.insert(*this_file);
    }
    if( these_files.empty() )
      continue;

    fiter_t lasttf = these_files.end();
    lasttf = these_files.insert( Filenames_t(numeric_limits<time_t>::max()) );

    // Build the output file name and open the file
    map<time_t,string>::iterator nt = dir_names.find(dir_from);
    assert( nt != dir_names.end() );
    const string& subdir = nt->second;
    string fname = subdir + "/db_" + detname + ".dat";
    ofstream ofs( fname.c_str() );
    if( !ofs ) {
      stringstream ss("Error opening ",ios::out|ios::app);
      ss << fname;
      perror(ss.str().c_str());
      return 1;
    }
    //    size_t nlines = 0; // Number of non-comment lines written to output

    for( fiter_t st = these_files.begin(); st != lasttf; ) {
      const size_t LEN = 256;
      char buf[LEN];
      string line;
      const string& path = st->path;
      //      fiter_t this_st = st;
      //      bool first_file = (st == these_files.begin());
      time_t val_from = st->val_start, val_until = (++st)->val_start;
      if( val_from  < dir_from  ) val_from = dir_from;
      if( val_until > dir_until ) val_until = dir_until;
      current_filename = path;

      //DEBUG
      cout << "Copy " << path << " val_from = " << format_time(val_from)
	   << " val_until = " << format_time(val_until) << endl;

      FILE* fi = fopen( path.c_str(), "r" );
      assert(fi);   // already succeeded previously
      if( !fi )
	continue;

      // Now copy this file. Processing depends on a number of factors:
      //
      // (1) If there is exactly one time range in the file (i.e. either
      //     no timestamps at all or exactly one timestamp and no preceding
      //     unstamped data), copy it exactly as it is, except:
      //     - if there is no timestamp, insert a val_from timestamp
      //       (converted to the output timezone) before the first data
      //     - if there is a timestamp, and it is < val_from, change it to
      //       val_from.
      //
      // (2) If there is more than one time range, and there is exactly one
      //     time range starting before or at val_from, copy the file time
      //     range by time range in order of ascending time. The first
      //     timestamp is advanced
      //     to val_from if necessary. (For the first file of several to be
      //     merged, this is just cosmetic, but it's required for subsequent
      //     files.) Any "unstamped" data at the beginning of the file are
      //     considered to be in a separate time range (starting at 0).
      //
      //     Any time ranges starting on or after val_until of the current file
      //     are not copied at all since they would never be reached when
      //     reading the original database.
      //
      // (3) If there is more than one time range starting before or at
      //     val_from, these "introductory" time ranges need to be collapsed
      //     into a single one (starting at val_from). If identical keys are
      //     present in several time ranges, only the latest one needs to be
      //     kept. Although it is technically acceptable to have multiple
      //     instances of a key in a single time range as long as they are
      //     written in the correct order (ascending with time), users could
      //     be confused by this, so multiple keys are eliminated.

      set_tz( inp_tz );
      bool got_initial_timestamp = false;
      time_t cur_date = 0; // Timestamp of currect section
      size_t ndata = 0;    // Non-comment lines in currect section
      SectionMap_t sections;
      string chunk;
      bool skip = false;
      while( GetLine(fi,buf,LEN,line) == 0 ) {
	time_t date;
	if( IsDBdate(line,date) ) {
	  // Save the previous section's data
	  if( !skip && !chunk.empty() && (cur_date == 0 || ndata > 0) ) {
	    AddSection( sections, cur_date, ndata, chunk );
	    //	    nlines += ndata;
	  }
	  cur_date = date;
	  ndata = 0;
	  // Skip sections whose time range is not applicable to this directory
	  skip = ( date >= val_until );
	}
	else if( !skip ) {
	  bool is_data = !IsDBcomment(line);
	  chunk.append(line).append("\n");
	  if( is_data )
	    ++ndata;
	}
      }
      if( !skip && !chunk.empty() && (cur_date == 0 || ndata > 0) ) {
	AddSection( sections, cur_date, ndata, chunk );
	//	nlines += ndata;
      }

      if( !sections.empty() ) {
	// Process sections with time <= val_from
	SMiter_t tt = sections.upper_bound(val_from), tf = sections.begin();
	assert( tf != sections.end() );
	iterator_traits<SMiter_t>::difference_type d = distance(tf,tt);
	if( tf->first /*date*/ == 0 && tf->second.first /*ndata*/ == 0 ) --d;
	// Now d holds the number of sections <= val_from that have data
	if( d > 1 ) {
	  // If more than one introductory section, keep only the most recent
	  // key definitions
	  assert( tt != sections.begin() );
	  --tt;
	  const size_t  tt_ndata = tt->second.first;
	  const string& tt_chunk = tt->second.second;
	  set<string> keys;
	  if( tt_ndata > 0 ) {
	    istringstream istr(tt_chunk);
	    string line;
	    while( getline(istr,line) ) {
	      string key, value;
	      if( IsDBkey(line,key,value) )
		keys.insert(key);
	    }
	  }
	  for( SectionMap_t::reverse_iterator rt(tt); rt != sections.rend(); ++rt ) {
	    size_t& ndata = rt->second.first, ncomments = 0;
	    if( ndata == 0 )
	      continue;
	    string& chunk = rt->second.second;
	    istringstream istr(chunk);
	    string new_chunk, line;
	    bool copying = true;
	    new_chunk.reserve(chunk.size());
	    ndata = 0;
	    while( getline(istr,line) ) {
	      string key, value;
	      if( IsDBkey(line,key,value) ) {
		copying = (keys.find(key) == keys.end());
		if( copying ) {
		  new_chunk.append(line).append("\n");
		  ++ndata;
		  keys.insert(key);
		}
	      } else if( IsDBcomment(line) ) {
		new_chunk.append(line).append("\n");
		if( line.find_first_not_of(" \t") == string::npos )
		  copying = true;
		else
		  ++ncomments;
	      } else if( copying ) {
		new_chunk.append(line).append("\n");
		++ndata;
	      }
	    }
	    if( ndata > 0 || ncomments > 0 ) {
	      // Collapse 3 or more consecutive newlines to 2
	      string::size_type pos;
	      while( (pos = new_chunk.find("\n\n\n")) != string::npos )
		new_chunk.replace(pos,3,2,'\n');
	      // Save the pruned new text block
	      chunk.swap( new_chunk );
	    } else
	      // Clear any blocks that are only whitespace
	      chunk.clear();
	  }
	}

	// Write processed sections to file
	reset_tz();
	set_tz( outp_tz );
	for( SMiter_t it = sections.begin(); it != sections.end(); ++it ) {
	  time_t date = it->first;
	  size_t ndata = it->second.first;
	  string& chunk = it->second.second;
	  if( chunk.empty() )
	    continue;
	  if( date == 0 ) {
	    if( ndata == 0 )
	      ofs << chunk;
	    else {
	      istringstream istr(chunk);
	      string line;
	      while( getline(istr,line) ) {
		if( !got_initial_timestamp && !IsDBcomment(line) ) {
		  ofs << format_tstamp(val_from) << endl << endl;
		  got_initial_timestamp = true;
		}
		ofs << line << endl;
	      }
	    }
	  } else {
	    //TODO: temporary
	    if( date < val_from || (date == val_from && got_initial_timestamp) ) {
	      ofs << "#";
	      ofs << format_tstamp(date) << endl;
	    }
	    if( !got_initial_timestamp ) {
	      if( date < val_from )
		date = val_from;
	      ofs << format_tstamp(date) << endl;
	      got_initial_timestamp = true;
	    } else if( date > val_from )
	      ofs << format_tstamp(date) << endl;

	    ofs << chunk;
	  }
	}
	if( these_files.size() > 2 && st != lasttf )
	  ofs << endl;
      }
      reset_tz();
      fclose(fi);
    }
    ofs.close();
    // TODO: check if file is empty (may happen because of in-file timestamps)
  }
  return 0;
}

//-----------------------------------------------------------------------------
int main( int argc, char* const argv[] )
{
  // Parse command line
  getargs(argc,argv);

  if( set_tz_errcheck( inp_tz, "input" ) )
    exit(EXIT_FAILURE);
  reset_tz();
  if( set_tz_errcheck( outp_tz, "output" ) )
    exit(EXIT_FAILURE);;
  reset_tz();

  // Read the detector name mapping file. If unavailable, set up defaults.
  if( mapfile ) {
    if( ReadMapfile(mapfile) )
      exit(5);  // Error message already printed
  } else {
    DefaultMap();
  }

  // Interpret timestamps according to the user-specified timezone for
  // input files. If no timezone was given, local time will be used.
  set_tz( inp_tz );

  // Get list of all database files to convert
  FilenameMap_t filemap;
  vector<string> subdirs;
  set<string> copy_dets;
  if( GetFilenames(srcdir, 0, filemap, subdirs) < 0 )
    exit(4);  // Error message already printed

  // Assign a parser to each database file, based on the name mapping info.
  // Let the parsers translate each file to database keys.
  // If the original parser supported in-file timestamps, pre-parse the
  // corresponding files to find any timestamps in them.
  // Keep all found keys/values along with timestamps in a central map.
  for( FilenameMap_t::iterator ft = filemap.begin(); ft != filemap.end();
       ++ft ) {
    const string& detname = ft->first;
    multiset<Filenames_t>& filenames = ft->second;
    assert( !filenames.empty() ); // else bug in GetFilenames

    // Check if we know what to do with this detector name
    NameTypeMap_t::iterator it = detname_map.find(detname);
    if( it == detname_map.end() ) {
      //TODO: make behavior configurable
      cerr << "===WARNING: unknown detector name \"" << detname
	   << "\", corresponding files will not be converted" << endl;
      continue;
    }
    EDetectorType type = it->second;
    Detector* det = MakeDetector( type, detname );
    if( !det )
      continue;

    if( InsertDefaultFiles(subdirs, filenames) )
      exit(8);

    filenames.insert( Filenames_t(numeric_limits<time_t>::max()) );

    ExtractKeys( det, filenames );

    // If requested, remove keys for this detector that only have default values
    if( purge_all_default_keys )
      det->PurgeAllDefaultKeys();

    // Save detector names whose database files are to be copied
    if( do_file_copy && type == kCopyFile )
      copy_dets.insert( detname );

    // Done with this detector
    delete det;
  } // end for ft = filemap

  reset_tz();

  // Prune the key/value map to remove entries that have the exact
  // same keys/values and only differ by /consecutive/ timestamps.
  // Keep only the earliest timestamp.

  PruneMap();

  // Write out keys/values to database files in target directory.
  // All file names will be preserved; a file that existed anywhere
  // in the source will also appear at least once in the target.
  // User may request that original directory structure be preserved,
  // otherwise just write one file per detector name.

  set_tz( outp_tz );

  if( do_dump )
    DumpMap();

  if( do_subdirs && out_subdirs.empty() )
    out_subdirs = subdirs;

  int err = 0;
  if( PrepareOutputDir(destdir,out_subdirs) )
    err = 6;

  if( !err && WriteFileDB(destdir,out_subdirs) )
    err = 7;

  reset_tz();

  if( do_file_copy ) {
    for( set<string>::const_iterator it = copy_dets.begin();
	 !err && it != copy_dets.end(); ++it ) {
      const string& detname = *it;
      FilenameMap_t::const_iterator ft = filemap.find( detname );
      assert( ft != filemap.end() );
      if( CopyFiles(destdir,out_subdirs,ft) )
	err = 8;
    }
  }

  return err;
}

//-----------------------------------------------------------------------------
static char* ReadComment( FILE* fp, char *buf, const int len )
{
  // Read fixed-format database comment lines. Data (non-comment) lines
  // start with a space (' ') or a number (the latter is a modification of
  // the original code).
  // The return value is a pointer to the comment line.
  // If the line is data, then nothing is done and NULL is returned,
  // so one can search for the next data line with:
  //   while ( ReadComment(fp, buf, len) );

  off_t pos = ftello(fp);
  if( pos == -1 )
    return 0;  // TODO: throw exception

  char* s = fgets(buf,len,fp);
  if( !s )
    return 0;

  int ch = *s;

  if( ch == '#' )
    goto comment;
  if( ch == ' ' || strchr("0123456789",ch) )
    goto data;
  // Might be a negative number
  if( ch == '-' && strlen(s) > 1 && strchr("0123456789",*(s+1)) )
    goto data;

 comment:
  return s;

 data:
  // Data: rewind to start position
  if( fseeko(fp, pos, SEEK_SET) ) {
    perror("ReadComment");  // TODO: throw exception
  }
  return 0;
}

//_____________________________________________________________________________
static bool IsDBdate( const string& line, time_t& date )
{
  // Check if 'line' contains a valid database time stamp. If so,
  // parse the line, set 'date' to the extracted time stamp, and return 1.
  // Else return 0;
  // Time stamps must be in SQL format: [ yyyy-mm-dd hh:mi:ss ]

  ssiz_t lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size()-12 ) return false;
  ssiz_t rbrk = line.find(']',lbrk);
  if( rbrk == string::npos || rbrk <= lbrk+11 ) return false;
  string ts = line.substr(lbrk+1,rbrk-lbrk-1);

  struct tm tm;
  bool have_tz = true;
  memset(&tm, 0, sizeof(tm));
  const char* c = strptime(ts.c_str(), "%Y-%m-%d %H:%M:%S %z", &tm );
  if( !c ) {
    have_tz = false;
    c = strptime(ts.c_str(), "%Y-%m-%d %H:%M:%S", &tm );
    if( !c )
      return false;
  }
  date = MkTime( tm, have_tz );
  return (date != static_cast<time_t>(-1));
}

//_____________________________________________________________________________
static Int_t IsDBkey( const string& line, string& key, string& text )
{
  // Check if 'line' is of the form "key = value"
  // - If there is no '=', then return 0.
  // - If key found, parse the line, set 'text' to the whitespace-trimmed
  //   text after the "=" and return +1.

  // Search for "="
  const char* ln = line.c_str();
  const char* eq = strchr(ln, '=');
  if( !eq ) return 0;
  // Extract the key
  while( *ln == ' ' || *ln == '\t' ) ++ln; // find_first_not_of(" \t")
  assert( ln <= eq );
  if( ln == eq ) return -1;
  const char* p = eq-1;
  assert( p >= ln );
  while( *p == ' ' || *p == '\t' ) --p; // find_last_not_of(" \t")
  key = string(ln,p-ln+1);
  // Extract the value, trimming leading and trailing whitespace
  ln = eq+1;
  while( *ln == ' ' || *ln == '\t' ) ++ln;
  text = ln;
  if( !text.empty() ) {
    string::size_type pos = text.find_last_not_of(" \t");
    text.erase(pos+1);
  }
  return 1;
}

//_____________________________________________________________________________
static bool IsDBcomment( const string& line )
{
  // Return true if 'line' is entirely a comment line in a new-format database
  // file. Comments start with '#', possibly preceded by whitespace, or are
  // completely empty lines.

  ssiz_t pos = line.find_first_not_of(" \t");
  return ( pos == string::npos || line[pos] == '#' );
}

//-----------------------------------------------------------------------------
template <class T>
Detector::EReadBlockRetval Detector::ReadBlock( FILE* fi, T* data, int nval,
    const char* here, int flags )
{
  // Read exactly 'nval' values of type T from file 'fi' into array 'data'.
  // Skip initial comment lines and comments at the end of each line.
  // Comment lines are lines that start with non-whitespace.
  // Data values may be spread over multiple lines. Each data line should
  // start with whitespace. A comment line after data lines ends parsing.
  // Error conditions: Not enough or too many data values in block
  // Optionally: value < 0 or <= 0 encountered
  // Warning if: comment line actually appears to be data (starts with
  // a digit or "-" followed by a digit.

  const char* const digits = "01234567890";
  const int LEN = 128;
  char buf[LEN];
  string line;
  int nread = 0;
  bool got_data = false, maybe_data = false, found_section = false;
  off_t pos_on_entry = ftello(fi), pos = pos_on_entry;
  std::less<T> std_less;  // to suppress compiler warnings "< 0 is always false"

  if( pos_on_entry < 0 )
    goto file_err;
  errno = 0;
  while( GetLine(fi,buf,LEN,line) == 0 ) {
    assert( !line.empty() );  // Should never happen by construction in GetLine
    if( line.empty() ) continue;
    if( line.length() > 4096 ) {  // Arbitrary limit. No single database line
                                  // should ever be even close to this
      Error( Here(here), "Runaway database line:\n \"%s ...\"\n",
          line.substr(0,48).c_str() );
      return kBadInput;
    }
    int c = line[0];
    maybe_data = (c && strchr(digits,c)) ||
      (c == '-' && line.length()>1 && strchr(digits,line[1]));
    if( maybe_data && TestBit(flags,kWarnOnDataGuess) )
      Warning( Here(here), "Suspicious data line:\n \"%s\"\n"
	       " Should start with whitespace. Check input file.",
	       line.c_str() );

    if( c == ' ' || c == '\t' ||
	(maybe_data && !TestBit(flags,kDisallowDataGuess)) ) {
      // Data
      if( nval <= 0 ) {
        if( fseeko(fi,pos,SEEK_SET) )
          goto file_err;
	return kSuccess;
      }
      got_data = true;
      istringstream is(line);
      if( is.bad() )
        goto file_err;
      while( is && nread < nval ) {
	is >> data[nread];
	if( !is.fail() ) {
	  if( TestBit(flags,kNoNegativeValues) && std_less(data[nread],T(0)) ) {
	    if( !TestBit(flags,kQuietOnErrors) )
	      Error( Here(here), "Negative values not allowed here:\n \"%s\"",
		     line.c_str() );
	    if( fseeko(fi,pos_on_entry,SEEK_SET) )
	      goto file_err;
	    return kNegativeFound;
	  }
	  if( TestBit(flags,kRequireGreaterZero) && data[nread] <= 0 ) {
	    if( !TestBit(flags,kQuietOnErrors) )
	      Error( Here(here), "Values must be greater then zero:\n \"%s\"",
		     line.c_str() );
	    if( fseeko(fi,pos_on_entry,SEEK_SET) )
	      goto file_err;
	    return kLessEqualZeroFound;
	  }
	  ++nread;
	}
	if( is.eof() )
	  break;
      }
      if( nread == nval ) {
	T x;
	is >> x;
	if( !is.fail() ) {
	  if( !TestBit(flags,kQuietOnErrors) && !TestBit(flags,kQuietOnTooMany) ) {
	    ostringstream msg;
	    msg << "Too many values (expected " << nval << ") at file position "
		<< ftello(fi) << endl
		<< " Line: \"" << line << "\"";
	    if( TestBit(flags,kErrOnTooManyValues) )
	      Error( Here(here), "%s", msg.str().c_str() );
	    else
	      Warning( Here(here), "%s", msg.str().c_str() );
	  }
	  if( fseeko(fi,pos_on_entry,SEEK_SET) )
	    goto file_err;
	  return kTooManyValues;
	}
	if( TestBit(flags,kStopAtNval) ) {
	  // If requested, stop here, regardless of whether there is another data line
	  return kSuccess;
	}
      }
    } else {
      // Comment
      found_section = ( TestBit(flags,kStopAtSection)
          && (line.find('[') != string::npos) );
      if( got_data || found_section ) {
	if( nread < nval )
	  goto toofew;
	// Success
	// Rewind to start of terminating line
	if( fseeko(fi,pos,SEEK_SET) )
	  goto file_err;
	return kSuccess;
      }
    }
    pos = ftello(fi);
    if( pos < 0 )
      break;
  }
  if( errno ) {
  file_err:
    perror( Here(here) );
    return kFileError;
  }
  if( nread < nval ) {
  toofew:
    if( !TestBit(flags,kQuietOnErrors) && !TestBit(flags,kQuietOnTooFew) ) {
      ostringstream msg;
      msg << "Too few values (expected " << nval << ", got " << nread << ") "
	  << "at file position " << ftello(fi) << endl
	  << " Line: \"" << line << "\"";
      Error( Here(here), "%s", msg.str().c_str() );
    }
    if( fseeko(fi,pos_on_entry,SEEK_SET) )
      goto file_err;
    if( nread == 0 )
      return kNoValues;
    return kTooFewValues;
  }
  return kSuccess;
}

//-----------------------------------------------------------------------------
int Detector::PurgeAllDefaultKeys()
{
  // Remove all keys of this detector that have only default values

  if( fDefaults.empty() )
    RegisterDefaults();

  for( StrMap_t::iterator it = fDefaults.begin(); it != fDefaults.end(); ++it ) {
    string key = fName + "." + it->first;
    DB::iterator jt = gDB.find( key );
    if( jt == gDB.end() )
      continue;
    const KeyAttr_t& attr = jt->second;
    if( attr.isCopy )
      continue;
    // Got a registered key. Compare all its values to the default.
    // Since the "values" are actually a string, we do a string comparison
    // for simplicity. This means that default values need to be registered
    // exactly in the form in which MakeValue would return them as a string.
    const ValSet_t& vals = attr.values;
    const string& defval = it->second;
    bool all_defaults = true;
    for( ValSet_t::const_iterator vt = vals.begin();
	 vt != vals.end() && all_defaults; ++vt ) {
      istringstream istr( vt->value );
      string val;
      while( istr >> val ) {
	if( val != defval ) {
	  all_defaults = false;
	  break;
	}
      }
    }
    if( all_defaults ) {
      gDB.erase( jt );
      pair<iter_t,iter_t> range = gDetToKey.equal_range( fName );
      for( iter_t kt = range.first; kt != range.second; ++kt ) {
	if( kt->second == key ) {
	  gDetToKey.erase( kt );
	  break;
	}
      }
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
void CopyFile::Clear()
{
  Detector::Clear();
  fDB.clear();
}

//-----------------------------------------------------------------------------
int CopyFile::ReadDB( FILE* fi, time_t date, time_t date_until )
{
  // Read keys/values from a file that is already in the new format.
  // This routine is similar to THaAnalysisObject::LoadDBvalue, but it
  // detects the key names and saves all key/value pairs.

  assert( date < date_until );

  const size_t bufsiz = 256;
  char* buf = new char[bufsiz];
  string line, version;
  time_t curdate = date;
  bool ignore = false;

  // Extract and save the keys
  while( THaAnalysisObject::ReadDBline(fi, buf, bufsiz, line) != EOF ) {
    if( line.empty() ) continue;
    string key, value;
    if( !ignore && IsDBkey(line, key, value) ) {
      // cout << "CopyFile date/key/value:"
      //	   << format_time(curdate) << ", " << key << " = " << value << endl;

      // TODO: add support for "text variables"?

      // Add this key/value pair to our local database
      DBvalue val( value, curdate, version );
      KeyAttr_t& attr = fDB[key];
      ValSet_t& vals = attr.values;
      ValSet_t::iterator pos = vals.find(val);
      // If key already exists for this time & version, overwrite its value
      // (this is the behavior in THaAnalysisObject::LoadDBvalue)
      if( pos != vals.end() ) {
	const_cast<DBvalue&>(*pos).value = value;
      } else {
	vals.insert(val);
      }
    }
    else if( IsDBdate(line, curdate) ) {
      // Ignore timestamp sections past the requested validity range
      ignore = ( curdate >= date_until );
    }
    // TODO: parse version tags
  }

  delete [] buf;
  return 0;
}

//-----------------------------------------------------------------------------
int CopyFile::Save( time_t start, const string& /*version*/ ) const
{
  // Copy the keys found to the global database. If multiple keys with
  // timestamps earlier than 'start' are present, use only the latest.

  for( DB::const_iterator dt = fDB.begin(); dt != fDB.end(); ++dt ) {
    const DB::value_type& keyval = *dt;
    const KeyAttr_t& attr = keyval.second;
    const ValSet_t& vals = attr.values;
    DBvalue startval(start);
    ValSet_t::const_iterator vt = vals.upper_bound(startval);
    if( vt != vals.begin() )
      --vt;
#ifndef NDEBUG
    bool forwarded = false;
#endif
    for( ; vt != vals.end(); ++vt ) {
      time_t date = vt->validity_start;
      if( date < start ) {
	date = start;
#ifndef NDEBUG
	// This should only ever happen at most once in this loop,
	// else the upper_bound logic above is buggy
	assert( !forwarded );
	forwarded = true;
#endif
      }
      AddToMap( keyval.first, Value_t(vt->value,vt->nelem,vt->width),
		date, vt->version, vt->max_per_line );
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
int Cherenkov::ReadDB( FILE* fi, time_t /* date */, time_t /* date_until */ )
{
  // Read legacy Cherenkov database

  const char* const here = "Cherenkov::ReadDB";

  const int LEN = 256;
  char buf[LEN];
  Int_t nelem;
  Int_t flags = kErrOnTooManyValues|kWarnOnDataGuess;

  if( ReadBlock(fi,&nelem,1,here,flags|kNoNegativeValues) )
    return kInitError;

  // Read detector map.  Assumes that the first half of the entries
  // is for ADCs, and the second half, for TDCs
  while( ReadComment(fi,buf,LEN) );
  fDetMap->Clear();
  fDetMapHasModel = false;
  while (1) {
    Int_t crate, slot, first, last, first_chan,model;
    int pos;
    if( fgets(buf,LEN,fi) == 0 ) return ErrPrint(fi,here);
    int n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		    &crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;
    if( CheckDetMapInp(crate,slot,first,last,buf,here) )
      return kInitError;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != 2*nelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d mirrors with 1 ADC and 1 TDC each)",
	   fDetMap->GetTotNumChan(), 2*nelem, nelem );
    return kInitError;
  }

  // If the detector map did not specify models, make it so. The Cherenkov
  // detector need models numbers in the data base
  if( !fDetMapHasModel ) {
    // If there are no model numbers, we must have an even number of modules
    // where the first half are ADCs, the second, TDCs
    if( (fDetMap->GetSize() % 2) != 0 ) {
      Error( Here(here), "Detector map without model numbers must have an "
	     "even number of modules." );
      return kInitError;
    }
    for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
      THaDetMap::Module* d = fDetMap->GetModule( i );
      if( i < fDetMap->GetSize()/2 )
	d->model = 1881; // Standard ADC
      else
	d->model = 1877; // Standard TDC
    }
    fDetMapHasModel = true;
  }

  // Read geometry
  Double_t dvals[3];
  if( ReadBlock(fi,dvals,3,here,flags) )                   // Detector's X,Y,Z coord
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size of det in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&fAngle,1,here,flags) )                 // Rotation angle of det
    return kInitError;

  // Calibration data
  if( nelem != fNelem ) {
    DeleteArrays();
    fNelem = nelem;
    fOff = new Float_t[ fNelem ];
    fPed = new Float_t[ fNelem ];
    fGain = new Float_t[ fNelem ];
  }

  // Read calibrations
  if( ReadBlock(fi,fOff,fNelem,here,flags) )
    return kInitError;

  if( ReadBlock(fi,fPed,fNelem,here,flags|kNoNegativeValues) )
    return kInitError;

  if( ReadBlock(fi,fGain,fNelem,here,flags|kNoNegativeValues) )
    return kInitError;

  return kOK;
}

//-----------------------------------------------------------------------------
int Scintillator::ReadDB( FILE* fi, time_t date, time_t /* date_until */ )
{
  // Read legacy Scintillator database

  const char* const here = "Scintillator::ReadDB";
  const int LEN = 256;
  char buf[LEN];
  Int_t nelem;
  Int_t flags = kErrOnTooManyValues|kWarnOnDataGuess;
  int n;

  fDetMapHasModel = fHaveExtras = false;

  // Read data from database

  if( ReadBlock(fi,&nelem,1,here,flags|kNoNegativeValues) )
    return kInitError;

  // Read detector map. Unless a model-number is given
  // for the detector type, this assumes that the first half of the entries
  // are for ADCs and the second half, for TDCs.
  while( ReadComment(fi, buf, LEN) );
  fDetMap->Clear();
  while (1) {
    int pos;
    Int_t first_chan, model;
    Int_t crate, slot, first, last;
    if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
    n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		&crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;
    if( CheckDetMapInp(crate,slot,first,last,buf,here) )
      return kInitError;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != 4*nelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d paddles with 2 ADCs and 2 TDCs each)",
	   fDetMap->GetTotNumChan(), 4*nelem, nelem );
    return kInitError;
  }

  // Read geometry
  Double_t dvals[3];
  if( ReadBlock(fi,dvals,3,here,flags) )                   // Detector's X,Y,Z coord
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size of det in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&fAngle,1,here,flags) )                 // Rotation angle of det
    return kInitError;

  // Calibration data
  if( nelem != fNelem ) {
    DeleteArrays();
    fNelem = nelem;
    fLOff  = new Double_t[ fNelem ];
    fROff  = new Double_t[ fNelem ];
    fLPed  = new Double_t[ fNelem ];
    fRPed  = new Double_t[ fNelem ];
    fLGain = new Double_t[ fNelem ];
    fRGain = new Double_t[ fNelem ];
    fTrigOff = new Double_t[ fNelem ];

    fNTWalkPar = 2*fNelem; // 1 paramter per PMT
    fTWalkPar = new Double_t[ fNTWalkPar ];
  }
  memset(fTrigOff,0,fNelem*sizeof(fTrigOff[0]));

  // Set DEFAULT values here
  // TDC resolution (s/channel)
  fTdc2T = 0.1e-9;      // seconds/channel
  fResolution = fTdc2T; // actual timing resolution
  // Speed of light in the scintillator material
  fCn = 1.7e+8;    // meters/second
  // Attenuation length
  fAttenuation = 0.7; // inverse meters
  // Time-walk correction parameters
  fAdcMIP = 1.e10;    // large number for offset, so reference is effectively disabled
  // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
  for (int i=0; i<fNTWalkPar; i++) fTWalkPar[i]=0;
  // trigger-timing offsets (s)
  for (int i=0; i<fNelem; i++) fTrigOff[i]=0;

  // Read in the timing/adc calibration constants
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // #comment line goes here
  // <left TDC offsets>
  // <right TDC offsets>
  // <left ADC peds>
  // <rigth ADC peds>
  // <left ADC coeff>
  // <right ADC coeff>
  //
  // if below aren't present, 'default' values are used
  // <TDC resolution: seconds/channel>
  // <speed-of-light in medium m/s>
  // <attenuation length m^-1>
  // <ADC of MIP>
  // <number of timewalk parameters>
  // <timewalk paramters>
  //
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // ...etc.
  //
  TDatime datime(date);
  if( THaAnalysisObject::SeekDBdate( fi, datime ) == 0 && fConfig.Length() > 0 &&
      THaAnalysisObject::SeekDBconfig( fi, fConfig.Data() )) {}

  // Read calibration data

  // Most scintillator databases have two data lines in each section, one for left,
  // one for right paddles, without a separator comment. Hence we must read the first
  // line without the check for "too many values".

  // Left pads TDC offsets
  Int_t err = ReadBlock(fi,fLOff,fNelem,here,flags|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    // Attempt to recover S0 databases with nelem == 1 that have L/R values
    // on a single line
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags) )
      return kInitError;
    fLOff[0] = dval[0];
    fROff[0] = dval[1];
  } else {
    // Right pads TDC offsets
    if( ReadBlock(fi,fROff,fNelem,here,flags) )
      return kInitError;
  }

  // Left pads ADC pedestals
  err = ReadBlock(fi,fLPed,fNelem,here,flags|kNoNegativeValues|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags|kNoNegativeValues) )
      return kInitError;
    fLPed[0] = dval[0];
    fRPed[0] = dval[1];
  } else {
    // Right pads ADC pedestals
    if( ReadBlock(fi,fRPed,fNelem,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  // Left pads ADC gains
  err = ReadBlock(fi,fLGain,fNelem,here,flags|kNoNegativeValues|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags|kNoNegativeValues) )
      return kInitError;
    fLGain[0] = dval[0];
    fRGain[0] = dval[1];
  } else {
    // Right pads ADC gains
    if( ReadBlock(fi,fRGain,fNelem,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  // Here on down, look ahead line-by-line
  // stop reading if a '[' is found on a line (corresponding to the next date-tag)

  flags = kErrOnTooManyValues|kQuietOnTooFew|kStopAtNval|kStopAtSection;
  // TDC resolution (s/channel)
  if( (err = ReadBlock(fi,&fTdc2T,1,here,flags)) )
    goto exit;
  fHaveExtras = true; // FIXME: should always be true?
  // Speed of light in the scintillator material (m/s)
  if( (err = ReadBlock(fi,&fCn,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Attenuation length (inverse meters)
  if( (err = ReadBlock(fi,&fAttenuation,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Typical ADC value for minimum ionizing particle (MIP)
  if( (err = ReadBlock(fi,&fAdcMIP,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Timewalk coefficients (1 for each PMT) -- seconds*1/sqrt(ADC)
  if( (err = ReadBlock(fi,fTWalkPar,fNTWalkPar,here,flags)) )
    goto exit;
  // Trigger-timing offsets -- seconds offset per paddle
  err = ReadBlock(fi,fTrigOff,fNelem,here,flags);

 exit:
  if( err && err != kNoValues )
    return kInitError;
  return kOK;
}

//-----------------------------------------------------------------------------
int Shower::ReadDB( FILE* fi, time_t date, time_t /* date_until */ )
{
  // Read legacy shower database

  const char* const here = "Shower::ReadDB";
  const int LEN = 100;
  char buf[LEN];
  Int_t flags = kErrOnTooManyValues; // TODO: make configurable
  //  Int_t nclbl; //TODO: use this
  bool old_format = false;

  // Read data from database

  // Blocks, rows, max blocks per cluster
  Int_t ivals[3];
  Int_t err = ReadBlock(fi,ivals,2,here,kRequireGreaterZero|kQuietOnTooMany);
  if( err ) {
    if( err != kTooManyValues )
      return kInitError;
    // > 2 values - may be an old-style DB with 3 numbers on first line
    if( ReadBlock(fi,ivals,3,here,kRequireGreaterZero) )
      return kInitError;
    if( ivals[0] % ivals[1] ) {
      Error( Here(here), "Total number of blocks = %d does not divide evenly into "
	     "number of columns = %d", ivals[0], ivals[1] );
      return kInitError;
    }
    fNcols = ivals[0] / ivals[1];
    fNrows = ivals[1];
    //    nclbl  = ivals[2];
    old_format = true;
  } else {
    fNcols = ivals[0];
    fNrows = ivals[1];
    //    nclbl  = TMath::Min( 3, fNrows ) * TMath::Min( 3, fNcols );
  }
  Int_t nelem = fNcols * fNrows;

  // Read detector map
  fDetMap->Clear();
  while( ReadComment(fi,buf,LEN) );
  while (1) {
    Int_t crate, slot, first, last;
    if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
    int n = sscanf( buf, "%6d %6d %6d %6d", &crate, &slot, &first, &last );
    if( crate < 0 ) break;
    if( n < 4 ) return ErrPrint(fi,here);
    if( CheckDetMapInp(crate,slot,first,last,buf,here) )
      return kInitError;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != nelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d blocks with 1 ADC each)",
	   fDetMap->GetTotNumChan(), nelem, nelem );
    return kInitError;
  }

  // Read channel map
  fChanMap.resize(nelem);
  if( ReadBlock(fi,&fChanMap[0],nelem,here,flags|kRequireGreaterZero) )
    return kInitError;

  bool trivial = true;
  for( Int_t i = 0; i < nelem; ++i ) {
    if( (Int_t)fChanMap[i] > nelem ) {
      Error( Here(here), "Illegal logical channel number %u. "
	     "Must be between 1 and %d.", fChanMap[i], nelem );
      return kInitError;
    }
    if( i>0 && fChanMap[i-1]+1 != fChanMap[i] ) {
      trivial = false;
      break;
    }
  }
  if( trivial )
    fChanMap.clear();

  // Arrays for per-block geometry and calibration data
  if( nelem != fNelem ) {
    DeleteArrays();
    fNelem  = nelem;
    fPed    = new Float_t[ fNelem ];
    fGain   = new Float_t[ fNelem ];
  }

  // Read geometry
  Double_t dvals[3];
  if( ReadBlock(fi,dvals,3,here,flags) )                   // Detector's X,Y,Z coord
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&fAngle,1,here,flags) )                 // Rotation angle of det
    return kInitError;

  // Try new format: two values here
  err = ReadBlock(fi,fXY,2,here,flags|kQuietOnTooMany);    // Block 1 center position
  if( err ) {
    if( err != kTooManyValues )
      return kInitError;
    old_format = true;
  }

  if( old_format ) {
    Double_t u = 1e-2; // Old database values are in cm
    Double_t dx = 0, dy = 0, dx1, dy1, eps = 1e-4;
    Double_t* xpos = new Double_t[ fNelem ];
    Double_t* ypos = new Double_t[ fNelem ];

    fOrigin *= u;
    for( Int_t i = 0; i < 3; ++i )
      fSize[i] *= u;

    if( (err = ReadBlock(fi,xpos,fNelem,here,flags)) )     // Block x positions
      goto err;
    if( (err = ReadBlock(fi,ypos,fNelem,here,flags)) )     // Block y positions
      goto err;

    fXY[0] = u*xpos[0];
    fXY[1] = u*ypos[0];

    // Determine the block spacings. Irregular spacings are not supported.
    for( Int_t i = 1; i < fNelem; ++i ) {
      dx1 = xpos[i] - xpos[i-1];
      dy1 = ypos[i] - ypos[i-1];
      if( dx == 0 ) {
	if( dx1 != 0 )
	  dx = dx1;
      } else if( dx1 != 0 && dx*dx1 > 0 && TMath::Abs(dx1-dx) > eps ) {
	Error( Here(here), "Irregular x block positions not supported, "
	       "dx = %lf, dx1 = %lf", dx, dx1 );
	err = -1;
	goto err;
      }
      if( dy == 0 ) {
	if( dy1 != 0 )
	  dy = dy1;
      } else if( dy1 != 0 && dy*dy1 > 0 && TMath::Abs(dy1-dy) > eps ) {
	Error( Here(here), "Irregular y block positions not supported, "
	       "dy = %lf, dy1 = %lf", dy, dy1 );
	err = -1;
	goto err;
      }
    }
    fDXY[0] = u*dx;
    fDXY[1] = u*dy;
  err:
    delete [] xpos;
    delete [] ypos;
    if( err )
      return kInitError;
  }
  else {
    // New format
    if( ReadBlock(fi,fDXY,2,here,flags) )                    // Block spacings in x and y
      return kInitError;

    if( ReadBlock(fi,&fEmin,1,here,flags|kNoNegativeValues) ) // Emin thresh for center
      return kInitError;

    // The BigBite shower has one extra parameter here: the max number of clusters
    // Attempt to read it
    if( fNelem > 1 ) {
      // Read as double to detect "too many values" from the pedestals block reliably
      Double_t maxcl;
      err = ReadBlock(fi,&maxcl,1,here,flags|kNoNegativeValues|kQuietOnTooMany );
      if( err == kSuccess )
	fMaxCl = TMath::Nint(maxcl);
    }

    // Search for optional time stamp or configuration section
    TDatime datime(date);
    if( THaAnalysisObject::SeekDBdate( fi, datime ) == 0 && fConfig.Length() > 0 &&
	THaAnalysisObject::SeekDBconfig( fi, fConfig.Data() )) {}
  }

  // Read calibrations
  // ADC pedestals (in order of logical channel number)
  if( ReadBlock(fi,fPed,fNelem,here,flags) )
    return kInitError;
  // ADC gains
  if( ReadBlock(fi,fGain,fNelem,here,flags|kNoNegativeValues) )
    return kInitError;

  if( old_format ) {
    // Emin thresh for center
    if( ReadBlock(fi,&fEmin,1,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  return kOK;
}

//-----------------------------------------------------------------------------
int TotalShower::ReadDB( FILE* fi, time_t, time_t )
{
  // Read legacy THaTotalShower database

  const char* const here = "TotalShower::ReadDB";
  int flags = kErrOnTooManyValues|kRequireGreaterZero|kWarnOnDataGuess;

  if( ReadBlock(fi, fMaxDXY, 2, here, flags) )
    return kInitError;

  return kOK;
}

//-----------------------------------------------------------------------------
int BPM::ReadDB( FILE* fi, time_t, time_t )
{
  // Read legacy THaBPM database

  const char* const here = "BPM::ReadDB";

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];

  sprintf(keyword,"[%s_detmap]",fRealName.c_str());
  Int_t n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here(here), "Unexpected end of BPM configuration file");
    return kInitError;
  }

  fDetMap->Clear();
  int first_chan, crate, dummy, slot, first, last, modulid = 0;
  do {
    if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
    n = sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d",
	       &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if( n < 1 ) return ErrPrint(fi,here);
    if( n == 7 )
      fDetMapHasModel = true;
    if( CheckDetMapInp(crate,slot,first,last,buf,here) )
      return kInitError;
    if (first_chan>=0 && n >= 6 ) {
      if ( fDetMap->AddModule(crate, slot, first, last, first_chan, modulid) <0 ) {
	Error( Here(here), "Couldn't add BPM to DetMap");
	return kInitError;
      }
    }
  } while (first_chan>=0);
  if( fDetMap->GetTotNumChan() != 4 ) {
    Error( Here(here), "Invalid BPM detector map.\n Needs to define exactly 4 "
	   "channels. Has %d.", fDetMap->GetTotNumChan() );
    return kInitError;
  }

  sprintf(keyword,"[%s]",fRealName.c_str());
  n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here(here), "Unexpected end of BPM configuration file");
    return kInitError;
  }

  double dummy1,dummy2,dummy3,dummy4,dummy5,dummy6;
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf",&dummy1,&dummy2,&dummy3,&dummy4);
  if( n < 2 ) return ErrPrint(fi,here);

  // dummy 1 is the z position of the bpm. Used below to set fOrigin.

  // calibration constant, historical
  fCalibRot = dummy2;

  // dummy3 and dummy4 are not used in this apparatus

  // Pedestals
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf",fPed,fPed+1,fPed+2,fPed+3);
  if( n != 4 ) return ErrPrint(fi,here);

  // 2x2 transformation matrix and x/y offset
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	     fRot,fRot+1,fRot+2,fRot+3,&dummy5,&dummy6);
  if( n != 6 ) return ErrPrint(fi,here);

  fOrigin.SetXYZ(dummy5,dummy6,dummy1);

  return kOK;
}

//-----------------------------------------------------------------------------
int Raster::ReadDB( FILE* fi, time_t date, time_t )
{
  // Read legacy THaRaster database

  const char* const here = "Raster::ReadDB";

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];

  // Seek our detmap section (e.g. "Raster_detmap")
  sprintf(keyword,"[%s_detmap]",fRealName.c_str());
  Int_t n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here(here), "Unexpected end of raster configuration file");
    return kInitError;
  }

  fDetMap->Clear();
  int first_chan, crate, dummy, slot, first, last, modulid = 0;
  do {
    if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
    n = sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d",
	       &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if( n < 1 ) return ErrPrint(fi,here);
    if( n == 7 )
      fDetMapHasModel = true;
    if( CheckDetMapInp(crate,slot,first,last,buf,here) )
      return kInitError;
    if (first_chan>=0 && n >= 6 ) {
      if ( fDetMap->AddModule(crate, slot, first, last, first_chan, modulid) <0 ) {
	Error( Here(here), "Couldn't add Raster to DetMap");
	return kInitError;
      }
    }
  } while (first_chan>=0);
  if( fDetMap->GetTotNumChan() != 4 ) {
    Error( Here(here), "Invalid raster detector map.\n Needs to define exactly 4 "
	   "channels. Has %d.", fDetMap->GetTotNumChan() );
    return kInitError;
  }

  // Seek our database section
  sprintf(keyword,"[%s]",fRealName.c_str());
  n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here(here), "Unexpected end of raster configuration file");
    return kInitError;
  }

  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  // NB: dummy1 is never used. Comment in old db files says it is "z-pos",
  // which are read from the following lines - probably dummy1 is leftover junk
  double dummy1;
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf %15lf",
	     &dummy1,fFreq,fFreq+1,fRPed,fRPed+1,fSPed,fSPed+1);
  if( n != 7 ) return ErrPrint(fi,here);

  // z positions of BPMA, BPMB, and target reference point (usually 0)
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf",fZpos);
  if( n != 1 ) return ErrPrint(fi,here);
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf",fZpos+1);
  if( n != 1 ) return ErrPrint(fi,here);
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf",fZpos+2);
  if( n != 1 ) return ErrPrint(fi,here);

  // Find timestamp, if any, for the raster constants
  TDatime datime(date);
  THaAnalysisObject::SeekDBdate( fi, datime, true );

  // Raster constants: offx/y,amplx/y,slopex/y for BPMA, BPMB, target
  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	     fCalibA,fCalibA+1,fCalibA+2,fCalibA+3,fCalibA+4,fCalibA+5);
  if( n != 6 ) return ErrPrint(fi,here);

  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 fCalibB,fCalibB+1,fCalibB+2,fCalibB+3,fCalibB+4,fCalibB+5);
  if( n != 6 ) return ErrPrint(fi,here);

  if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 fCalibT,fCalibT+1,fCalibT+2,fCalibT+3,fCalibT+4,fCalibT+5);
  if( n != 6 ) return ErrPrint(fi,here);

  return kOK;
}

//-----------------------------------------------------------------------------
int CoincTime::ReadDB( FILE* fi, time_t, time_t )
{
  // Read legacy THaCoincTime database

  const char* const here = "CoincTime::ReadDB";

  const int LEN = 200;
  char buf[LEN];

  fDetMap->Clear();
  fDetMapHasModel = true;

  Int_t k = 0;
  while ( 1 ) {
    Int_t crate, slot, first, model, nread;
    Double_t tres;       // TDC resolution
    Double_t toff = 0.;  // TDC offset (in seconds)
    char label[21];      // string label for TDC = Stop_by_Start

    while( ReadComment(fi, buf, LEN) );
    if( fgets(buf,LEN,fi ) == 0 ) return ErrPrint(fi,here);
    nread = sscanf( buf, "%6d %6d %6d %6d %15lf %20s %15lf",
		    &crate, &slot, &first, &model, &tres, label, &toff );
    if( crate < 0 ) break;
    if( CheckDetMapInp(crate,slot,first,1,buf,here) )
      return kInitError;
    if( k > 1 ) {
      Warning( Here(here), "Too many detector map entries. Must be exactly 2.");
      break;
    }
    // Heuristic check for old-format database (before ca. 2003)
    if( tres > 1. ) {
      // Old format (no TDC offset, but redundant "last" channel)
      Int_t last;
      nread = sscanf( buf, "%6d %6d %6d %6d %6d %15lf %20s",
		      &crate, &slot, &first, &last, &model, &tres, label );
      // Turn labels like "L_by_R" into "LbyR"
      char* p = strstr(label, "_by_");
      if( p && p != label && *(p+4) ) {
	char *q = p+1, *r = p;
	while( *q ) { if( q == p+3 ) ++q; *(r++) = *(q++); } *r = 0;
      }
    } else {
      // New format (already read above)
      // Turn labels like "ct_LbyR" into "LbyR"
      char *q = label+3;
      while( *q ) { *(q-3) = *q; ++q; } *(q-3) = 0;
    }
    if( nread < 6 ) {
      Error( Here(here), "Invalid detector map! Need at least 6 columns." );
      return kInitError;
    }
    if( CheckDetMapInp(crate,slot,first,1,buf,here) )
      return kInitError;
    if( fDetMap->AddModule( crate, slot, first, first, k, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
    fTdcRes[k] = tres;
    fTdcOff[k] = toff;
    fTdcLabels[k] = label;
    k++;
  }

  return kOK;
}

//-----------------------------------------------------------------------------
int TriggerTime::ReadDB( FILE* fi, time_t, time_t )
{
  // Legacy THaTriggerTime database reader

  const char* const here = "TriggerTime::ReadDB";

  const int LEN = 200;
  char buf[LEN];

  // Read in the time offsets, in the format below, to be subtracted from
  // the times measured in other detectors.
  //
  // TrgType 0 is special, in that it is a global offset that is applied
  //  to all triggers. This gives us a simple single value for adjustments.
  //
  // Trigger types NOT listed are assumed to have a zero offset.
  //
  // <TrgType>   <time offset in seconds>
  // eg:
  //   0              10   -0.5e-9  # global-offset shared by all triggers and s/TDC
  //   1               0       crate slot chan
  //   2              10.e-9
  Double_t toff, ch2t=-0.5e-9; // assume 1872 TDC's.
  Int_t trg,crate,slot,chan;
  fTrgDef.clear();
  fTDCRes = ch2t;

  while( ReadComment(fi,buf,LEN) );

  while ( fgets(buf,LEN,fi) ) {
    int fnd = sscanf( buf,"%8d %16lf %16lf",&trg,&toff,&ch2t);
    if( fnd < 2 ) goto err;
    if( trg == 0 ) {
      fGlOffset = toff;
      fTDCRes = ch2t;
    } else {
      fnd = sscanf( buf,"%8d %16lf %8d %8d %8d",&trg,&toff,&crate,&slot,&chan);
      if( fnd != 5 ) {
      err:
	Error( Here(here), "Cannot parse line:\n \"%s\"", buf );
	return kInitError;
      }
      ostringstream ostr;
      ostr << trg << " " << toff << " " << crate << " " << slot << " " << chan;
      fTrgDef.push_back(ostr.str());
    }
  }

  return kOK;
}

//-----------------------------------------------------------------------------
int VDC::ReadDB( FILE* file, time_t date, time_t date_until )
{
  // Legacy VDC database reader (reads data for VDC and VDCPlane)

  const char* const here = "VDC::ReadDB";

  const int LEN = 200;
  char buff[LEN];

  //----- VDC -----

  // Look for the section [<prefix>.global] in the file, e.g. [ R.global ]
  TString tag(fName);
  Ssiz_t pos = tag.Index(".");
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]");

  TString line;
  bool found = false;
  while (!found && fgets(buff, LEN, file) != 0) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section %s not found!", tag.Data() );
    return kInitError;
  }

  // We found the section, now read the data

  // read in some basic constants first
  //  fscanf(file, "%lf", &fSpacing);
  // fSpacing is now calculated from the actual z-positions in Init(),
  // so skip the first line after [ global ] completely:
  if( fgets(buff,LEN,file ) == 0 ) return kInitError;  // Skip rest of line

  // Read in the focal plane transfer elements
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // comment line goes here
  // t 0 0 0  ...
  // y 0 0 0  ... etc.
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // t 0 0 0  ... etc.
  //
  TDatime datime(date);
  if( (found = THaAnalysisObject::SeekDBdate(file,date)) == 0
      && !fConfig.IsNull()
      && (found = THaAnalysisObject::SeekDBconfig(file,fConfig.Data())) == 0 ) {
    // Print warning if a requested (non-empty) config not found
    Warning( Here(here), "Requested configuration section \"%s\" not "
	     "found in database. Using default (first) section.",
	     fConfig.Data() );
  }

  // Second line after [ global ] or first line after a found tag.
  // After a found tag, it must be the comment line. If not found, then it
  // can be either the comment or a non-found tag before the comment...
  if( fgets(buff,LEN,file) == 0 ) return kInitError;
  if( !found && THaAnalysisObject::IsTag(buff) )
    // Skip one more line if this one was a non-found tag
    if( fgets(buff,LEN,file) == 0 ) return kInitError;

  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fPTAMatrixElems.clear();
  fYMatrixElems.clear();
  fYTAMatrixElems.clear();
  fLMatrixElems.clear();

  fFPMatrixElems.clear();
  fFPMatrixElems.resize(3);

  typedef vector<string>::size_type vsiz_t;
  map<string,vsiz_t> power;
  power["t"] = 3;  // transport to focal-plane tensors
  power["y"] = 3;
  power["p"] = 3;
  power["D"] = 3;  // focal-plane to target tensors
  power["T"] = 3;
  power["Y"] = 3;
  power["YTA"] = 4;
  power["P"] = 3;
  power["PTA"] = 4;
  power["L"] = 4;  // pathlength from z=0 (target) to focal plane (meters)
  power["XF"] = 5; // forward: target to focal-plane (I think)
  power["TF"] = 5;
  power["PF"] = 5;
  power["YF"] = 5;

  map<string,vector<THaMatrixElement>*> matrix_map;
  matrix_map["t"] = &fFPMatrixElems;
  matrix_map["y"] = &fFPMatrixElems;
  matrix_map["p"] = &fFPMatrixElems;
  matrix_map["D"] = &fDMatrixElems;
  matrix_map["T"] = &fTMatrixElems;
  matrix_map["Y"] = &fYMatrixElems;
  matrix_map["YTA"] = &fYTAMatrixElems;
  matrix_map["P"] = &fPMatrixElems;
  matrix_map["PTA"] = &fPTAMatrixElems;
  matrix_map["L"] = &fLMatrixElems;

  map <string,int> fp_map;
  fp_map["t"] = 0;
  fp_map["y"] = 1;
  fp_map["p"] = 2;

  // Read in as many of the matrix elements as there are.
  // Read in line-by-line, so as to be able to handle tensors of
  // different orders.
  while( fgets(buff, LEN, file) ) {
    string line(buff);
    // Erase trailing newline
    if( line.size() > 0 && line[line.size()-1] == '\n' ) {
      line.erase(line.size()-1,1);
    }
    // Split the line into whitespace-separated fields
    vector<string> line_spl = THaString::Split(line);

    // Stop if the line does not start with a string referring to
    // a known type of matrix element. In particular, this will
    // stop on a subsequent timestamp or configuration tag starting with "["
    if(line_spl.empty())
      continue; //ignore empty lines
    const char* w = line_spl[0].c_str();
    vsiz_t npow = power[w];
    if( npow == 0 )
      break;
    if( line_spl.size() < npow+2 ) {
      ostringstream ostr;
      ostr << "Line = \"" << line << "\"" << endl
	   << " Too few values for matrix element"
	   << " (found " << line_spl.size() << ", need >= " << npow+2 << ")";
      Error( Here(here), "%s", ostr.str().c_str() );
      return kInitError;
    }
    // Looks like a good line, go parse it.
    THaMatrixElement ME;
    ME.pw.resize(npow);
    ME.poly.resize(kPORDER);
    vsiz_t pos;
    for (pos=1; pos<npow+1; pos++) {
      assert(pos < line_spl.size());
      ME.pw[pos-1] = atoi(line_spl[pos].c_str());
    }
    vsiz_t p_cnt;
    for ( p_cnt=0; pos<line_spl.size() && p_cnt<kPORDER && pos<npow+kPORDER+1;
	  pos++,p_cnt++ ) {
      ME.poly[p_cnt] = atof(line_spl[pos].c_str());
      if (ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    assert( p_cnt >= 1 );
    // if (p_cnt < 1) {
    //   Error(Here(here), "Could not read in Matrix Element %s%d%d%d!",
    //	    w, ME.pw[0], ME.pw[1], ME.pw[2]);
    //   Error(Here(here), "Line = \"%s\"",line.c_str());
    //   return kInitError;
    // }
    // Don't bother with all-zero matrix elements
    if( ME.iszero )
      continue;

    // Add this matrix element to the appropriate array
    vector<THaMatrixElement> *mat = matrix_map[w];
    if (mat) {
      // Special checks for focal plane matrix elements
      if( mat == &fFPMatrixElems ) {
	if( ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
	  THaMatrixElement& m = (*mat)[fp_map[w]];
	  if( m.order > 0 ) {
	    Warning(Here(here), "Duplicate definition of focal plane "
		    "matrix element: %s. Using first definition.",
		    line.c_str());
	  } else
	    m = ME;
	} else
	  Warning(Here(here), "Bad coefficients of focal plane matrix "
		  "element %s", line.c_str() );
      }
      else {
	// All other matrix elements are just appended to the respective array
	// but ensure that they are defined only once!
	bool match = false;
	for( vector<THaMatrixElement>::iterator it = mat->begin();
	     it != mat->end() && !(match = it->match(ME)); ++it ) {}
	if( match ) {
	  Warning(Here(here), "Duplicate definition of "
		  "matrix element: %s. Using first definition.",
		  line.c_str() );
	} else
	  mat->push_back(ME);
      }
    }
  } //while(fgets)

  //----- VDCPlane -----
  fPlanes.clear();
  const char* plane_name[] = { "u1", "v1", "u2", "v2" };
  for( int i = 0; i < 4; ++i ) {
    fPlanes.push_back( Plane(fName+"."+plane_name[i],this) );
    fPlanes.back().SetDBName(fName);
    rewind(file);
    Int_t err = fPlanes.back().ReadDB( file, date, date_until );
    if( err )
      return err;
  }

  // Determine parameters common to all planes
  fCommon = 0;
  for( ECommonFlag e = kNelem; e != kFlagsEnd; ++e ) {
    bool is_common = true;
    for( int i = 0; i < 3; ++i ) {
      switch (e) {
      case kNelem:
	is_common = ( fPlanes[i].fNelem == fPlanes[i+1].fNelem );
	break;
      case kWSpac:
	is_common = ( fPlanes[i].fWSpac == fPlanes[i+1].fWSpac );
	break;
      case kDriftVel:
	is_common = ( fPlanes[i].fDriftVel == fPlanes[i+1].fDriftVel );
	break;
      case kMinTime:
	is_common = ( fPlanes[i].fMinTime == fPlanes[i+1].fMinTime );
	break;
      case kMaxTime:
	is_common = ( fPlanes[i].fMaxTime == fPlanes[i+1].fMaxTime );
	break;
      case kTDCRes:
	is_common = ( fPlanes[i].fTDCRes == fPlanes[i+1].fTDCRes );
	break;
      case kTTDConv:
	is_common = ( fPlanes[i].fTTDConv == fPlanes[i+1].fTTDConv );
	break;
      case kTTDPar:
	is_common = ( fPlanes[i].fTTDPar == fPlanes[i+1].fTTDPar );
	break;
      case kT0Resolution:
	is_common = ( fPlanes[i].fT0Resolution == fPlanes[i+1].fT0Resolution );
	break;
      case kMinClustSize:
	is_common = ( fPlanes[i].fMinClustSize == fPlanes[i+1].fMinClustSize );
	break;
      case kMaxClustSpan:
	is_common = ( fPlanes[i].fMaxClustSpan == fPlanes[i+1].fMaxClustSpan );
	break;
      case kNMaxGap:
	is_common = ( fPlanes[i].fNMaxGap == fPlanes[i+1].fNMaxGap );
	break;
      case kMinTdiff:
	is_common = ( fPlanes[i].fMinTdiff == fPlanes[i+1].fMinTdiff );
	break;
      case kMaxTdiff:
	is_common = ( fPlanes[i].fMaxTdiff == fPlanes[i+1].fMaxTdiff );
	break;
      default:
	assert(false); // not reached
	break;
      }
      if( !is_common )
	break;
    }
    if( is_common )
      fCommon |= e;
  }

  fErrorCutoff = 1e9;
  fNumIter = 1;

  return kOK;
}

//-----------------------------------------------------------------------------
int VDC::Plane::ReadDB( FILE* file, time_t /* date */, time_t )
{
  // Legacy VDCPlane database reader

  const char* const here = "VDC::Plane::ReadDB";

  const int LEN = 200;
  char buff[LEN];

  TString tag(fName);

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  tag.Prepend("["); tag.Append("]");
  TString line;
  bool found = false;
  while (!found && fgets(buff, LEN, file) != 0) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section \"%s\" not found! "
	  "Initialization failed", tag.Data() );
    return kInitError;
  }

  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  do {
    if( fgets(buff,LEN,file) == 0 ) {
      Error( Here(here), "Error reading detector map" );
      return kInitError;
    }
    // bad kludge to allow a variable number of detector map lines
    if( strchr(buff, '.') ) // any floating point number indicates end of map
      break;
    // Get crate, slot, low channel and high channel from file
    Int_t crate, slot, lo, hi;
    if( sscanf( buff, "%6d %6d %6d %6d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      return kInitError;
    }
    if( CheckDetMapInp(crate,slot,lo,hi,buff,here) )
      return kInitError;
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (TMath::Abs(hi - lo) + 1);
    nWires += prev_nwires;
  } while( *buff );  // sanity escape

  static const Int_t max_nWires = std::numeric_limits<Short_t>::max();
  if( nWires > max_nWires ) {
    Error( Here(here), "Illegal number of wires %d in VDC plane %s. "
        "Should be 368, but at most %d. Fix database.",
        nWires, fName.c_str(), max_nWires );
    return kInitError;
  }
  fNelem = nWires;

  // Load z, wire beginning postion, wire spacing, and wire angle
  sscanf( buff, "%15lf %15lf %15lf %15lf", &fZ, &fWBeg, &fWSpac, &fWAngle );
  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  // Load drift velocity (will be used to initialize crude Time to Distance
  // converter)
  if( fscanf(file, "%15lf", &fDriftVel) != 1 ) {
    Error( Here(here), "Error reading drift velocity, line: %s", buff );
    return kInitError;
  }
  if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Read to end of line
  if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Skip line

  // first read in the time offsets for the wires
  fTDCOffsets.clear();
  fBadWires.clear();
  fTDCOffsets.resize(nWires);
  for (int i = 0; i < nWires; i++) {
    Int_t wnum;
    Double_t offset;
    if( fscanf(file, " %6d %15lf", &wnum, &offset) != 2 ) {
      Error( Here(here), "Error reading TDC offsets, line: %s", buff );
      return kInitError;
    }
    fTDCOffsets[wnum-1] = offset;
    if( wnum < 0 )
      fBadWires.push_back(wnum-1); // Wire numbers in file start at 1
  }

  // now read in the time-to-drift-distance lookup table
  // data (if it exists)
  //   fgets(buff, LEN, file); // read to the end of line
  //   fgets(buff, LEN, file);
  //   if(strncmp(buff, "TTD Lookup Table", 16) == 0) {
  //     // if it exists, read the data in
  //     fscanf(file, "%le", &fT0);
  //     fscanf(file, "%d", &fNumBins);

  //     // this object is responsible for the memory management
  //     // of the lookup table
  //     delete [] fTable;
  //     fTable = new Float_t[fNumBins];
  //     for(int i=0; i<fNumBins; i++) {
  //       fscanf(file, "%e", &(fTable[i]));
  //     }
  //   } else {
  //     // if not, set some reasonable defaults and rewind the file
  //     fT0 = 0.0;
  //     fNumBins = 0;
  //     fTable = NULL;
  //     cout<<"Could not find lookup table header: "<<buff<<endl;
  //     fseek(file, -strlen(buff), SEEK_CUR);
  //   }


  // Read additional parameters (not in original database)
  // For backward compatibility with database version 1, these parameters
  // are in an optional section, labelled [ <prefix>.extra_param ]
  // (e.g. [ R.vdc.u1.extra_param ]) or [ R.extra_param ].  If this tag is
  // not found, a warning is printed and defaults are used.

  tag = "["; tag.Append(fName); tag.Append(".extra_param]");
  TString tag2(tag);
  found = false;
  rewind(file);
  while (!found && fgets(buff,LEN,file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    // Poor man's default key search
    rewind(file);
    tag2 = fName;
    Ssiz_t pos = tag2.Index(".");
    if( pos != kNPOS )
      tag2 = tag2(0,pos+1);
    else
      tag2.Append(".");
    tag2.Prepend("[");
    tag2.Append("extra_param]");
    while (!found && fgets(buff,LEN,file) != NULL) {
      char* buf = ::Compress(buff);  //strip blanks
      line = buf;
      delete [] buf;
      if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
      if ( tag2 == line )
	found = true;
    }
  }
  if( found ) {
    if( fscanf(file, "%lf %lf", &fTDCRes, &fT0Resolution) != 2) {
      Error( Here(here), "Error reading TDC scale and T0 resolution\n"
	     "Line = %s\nFix database.", buff );
      return kInitError;
    }
    if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Read to end of line
    if( fscanf( file, "%d %d %d %d %d %lf %lf", &fMinClustSize, &fMaxClustSpan,
		&fNMaxGap, &fMinTime, &fMaxTime, &fMinTdiff, &fMaxTdiff ) != 7 ) {
      Error( Here(here), "Error reading min_clust_size, max_clust_span, "
	     "max_gap, min_time, max_time, min_tdiff, max_tdiff.\n"
	     "Line = %s\nFix database.", buff );
      return kInitError;
    }
    if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Read to end of line
    // Time-to-distance converter parameters
    // THaVDCAnalyticTTDConv
    // Format: 4*A1 4*A2 dtime(s)  (9 floats)
    Double_t par[9];
    if( fscanf(file, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
	       par,   par+1, par+2, par+3,
	       par+4, par+5, par+6, par+7, par+8 ) != 9) {
      Error( Here(here), "Error reading THaVDCAnalyticTTDConv parameters\n"
	     "Line = %s\nExpect 9 floating point numbers. Fix database.",
	     buff );
      return kInitError;
    }
    fTTDPar.assign( par, par+9 );
    if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Read to end of line

    Double_t h, w;

    if( fscanf(file, "%lf %lf", &h, &w) != 2) {
      Error( Here(here), "Error reading height/width parameters\n"
	     "Line = %s\nExpect 2 floating point numbers. Fix database.",
	     buff );
      return kInitError;
    } else {
      fSize[0] = h/2.0;
      fSize[1] = w/2.0;
    }

    if( fgets(buff,LEN,file) == 0 ) return kInitError;  // Read to end of line
  } else {
    // Warning( Here(here), "No database section \"%s\" or \"%s\" found. "
    //	     "Using defaults.", tag.Data(), tag2.Data() );
    fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan
    fT0Resolution = 6e-8; // 60 ns --- crude guess
    fMinClustSize = 3;
    fMaxClustSpan = 7;
    fNMaxGap = 1;
    fMinTime = 800;
    fMaxTime = 2200;
    fMinTdiff = 3e-8;   // 30ns  -> ~20 deg track angle
    fMaxTdiff = 2e-7;   // 200ns -> ~67 deg track angle

    fTTDPar[0] = 2.12e-3;
    fTTDPar[1] = fTTDPar[2] = fTTDPar[3] = 0.0;
    fTTDPar[4] = -4.2e-4;
    fTTDPar[5] = 1.3e-3;
    fTTDPar[6] = 1.06e-4;
    fTTDPar[7] = 0.0;
    fTTDPar[8] = 4e-9;
  }

  return kOK;
}

//-----------------------------------------------------------------------------
int Detector::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";
  int flags = 0;
  if( fDetMapHasLogicalChan ) {
    flags++;
    if( fDetMapHasModel )
      flags++;
  }
  AddToMap( prefix+"detmap",   MakeValue(fDetMap,flags), start, version, 4+flags );
  if( !fNelemName.empty() )
    AddToMap( prefix+fNelemName, MakeValue(&fNelem),     start, version );
  AddToMap( prefix+"angle",    MakeValue(&fAngle),       start, version );
  AddToMap( prefix+"position", MakeValue(&fOrigin),      start, version );
  // Old databases define the x and y size as half the actual size, but z as the
  // full detector thickness. New databases specify full sizes for all coordinates.
  // Internally, fSize[0] and fSize[1] continue to be half-sizes.
  Double_t fullSize[3];
  fullSize[0] = 2.0*fSize[0]; fullSize[1] = 2.0*fSize[1]; fullSize[2] = fSize[2];
  AddToMap( prefix+"size",     MakeValue(fullSize,3),    start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Cherenkov::Save( time_t start, const string& version ) const
{
  // Create database keys for current Cherenkov configuration data

  string prefix = fName + ".";

  fNelemName = "npmt";
  Detector::Save( start, version );

  AddToMap( prefix+"tdc.offsets",   MakeValue(fOff,fNelem),  start, version );
  AddToMap( prefix+"adc.pedestals", MakeValue(fPed,fNelem),  start, version );
  AddToMap( prefix+"adc.gains",     MakeValue(fGain,fNelem), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Scintillator::Save( time_t start, const string& version ) const
{
  // Create database keys for current Scintillator configuration data

  string prefix = fName + ".";

  fNelemName = "npaddles";
  Detector::Save( start, version );

  AddToMap( prefix+"L.off",    MakeValue(fLOff,fNelem),  start, version );
  AddToMap( prefix+"R.off",    MakeValue(fROff,fNelem),  start, version );
  AddToMap( prefix+"L.ped",    MakeValue(fLPed,fNelem),  start, version );
  AddToMap( prefix+"R.ped",    MakeValue(fRPed,fNelem),  start, version );
  AddToMap( prefix+"L.gain",   MakeValue(fLGain,fNelem), start, version );
  AddToMap( prefix+"R.gain",   MakeValue(fRGain,fNelem), start, version );

  if( fHaveExtras ) {
    AddToMap( prefix+"avgres",   MakeValue(&fResolution), start, version );
    AddToMap( prefix+"atten",    MakeValue(&fAttenuation), start, version );
    AddToMap( prefix+"Cn",       MakeValue(&fCn),start, version );
    AddToMap( prefix+"MIP",      MakeValue(&fAdcMIP), start, version );
    AddToMap( prefix+"tdc.res",  MakeValue(&fTdc2T), start, version );

    AddToMap( prefix+"timewalk_params",  MakeValue(fTWalkPar,fNTWalkPar),
	      start, version, fNelem );
    AddToMap( prefix+"retiming_offsets", MakeValue(fTrigOff,fNelem), start, version );
  }

  return 0;
}

//-----------------------------------------------------------------------------
int Shower::Save( time_t start, const string& version ) const
{
  // Create database keys for current Shower configuration data

  string prefix = fName + ".";

  fNelemName = ""; // nelem is redundant here because it equals fNcols*fNrows
  Detector::Save( start, version );

  AddToMap( prefix+"ncols",     MakeValue(&fNcols), start, version );
  AddToMap( prefix+"nrows",     MakeValue(&fNrows), start, version );

  if( !fChanMap.empty() )
    AddToMap( prefix+"chanmap", MakeValue(&fChanMap[0], fChanMap.size()),
	      start, version, fNcols );

  AddToMap( prefix+"xy",        MakeValue(fXY,2),   start, version );
  AddToMap( prefix+"dxdy",      MakeValue(fDXY,2),  start, version );
  AddToMap( prefix+"emin",      MakeValue(&fEmin),  start, version );
  if( fMaxCl != -1 )
    AddToMap( prefix+"maxcl",   MakeValue(&fMaxCl), start, version );

  AddToMap( prefix+"pedestals", MakeValue(fPed,fNelem),  start, version, fNcols );
  AddToMap( prefix+"gains",     MakeValue(fGain,fNelem), start, version, fNcols );

  return 0;
}

//-----------------------------------------------------------------------------
int TotalShower::Save( time_t start, const string& version ) const
{
  // Create database keys for current TotalShower configuration data

  string prefix = fName + ".";

  //  Detector::Save( start, version );

  AddToMap( prefix+"max_dxdy",  MakeValue(fMaxDXY,2), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int BPM::Save( time_t start, const string& version ) const
{
  // Create database keys for current TotalShower configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  AddToMap( prefix+"calib_rot",  MakeValue(&fCalibRot), start, version );
  AddToMap( prefix+"pedestals",  MakeValue(fPed,4),     start, version );
  AddToMap( prefix+"rotmatrix",  MakeValue(fRot,4),     start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Raster::Save( time_t start, const string& version ) const
{
  // Create database keys for current TotalShower configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  AddToMap( prefix+"zpos",        MakeValue(fZpos,3),   start, version );
  AddToMap( prefix+"freqs",       MakeValue(fFreq,2),   start, version );
  AddToMap( prefix+"rast_peds",   MakeValue(fRPed,2),   start, version );
  AddToMap( prefix+"slope_peds",  MakeValue(fSPed,2),   start, version );
  AddToMap( prefix+"raw2posA",    MakeValue(fCalibA,6), start, version );
  AddToMap( prefix+"raw2posB",    MakeValue(fCalibB,6), start, version );
  AddToMap( prefix+"raw2posT",    MakeValue(fCalibT,6), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int CoincTime::Save( time_t start, const string& version ) const
{
  // Create database keys for current TotalShower configuration data

  string prefix0 = fName + "." + fTdcLabels[0].Data() + ".";
  string prefix1 = fName + "." + fTdcLabels[1].Data() + ".";

  AddToMap( prefix0+"detmap",     MakeDetmapElemValue(fDetMap,0,2), start, version );
  AddToMap( prefix0+"tdc_res",    MakeValue(fTdcRes), start, version );
  AddToMap( prefix0+"tdc_offset", MakeValue(fTdcOff), start, version );
  AddToMap( prefix1+"detmap",     MakeDetmapElemValue(fDetMap,1,2), start, version );
  AddToMap( prefix1+"tdc_res",    MakeValue(fTdcRes+1), start, version );
  AddToMap( prefix1+"tdc_offset", MakeValue(fTdcOff+1), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int TriggerTime::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";

  AddToMap( prefix+"tdc_res",  MakeValue(&fTDCRes),   start, version );
  AddToMap( prefix+"glob_off", MakeValue(&fGlOffset), start, version );
  AddToMap( prefix+"trigdef",  MakeValue(&fTrgDef[0],fTrgDef.size()), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
static void WriteME( ostream& s, const THaVDC::THaMatrixElement& ME,
		     const string& MEname, ssiz_t w )
{
  if( ME.iszero )
    return;
  s << endl << "  " << MEname;
  for( vector<int>::size_type k = 0; k < ME.pw.size(); ++k ) {
    s << " " << ME.pw[k];
  }
  assert( ME.order <= (int)ME.poly.size() );
  ios_base::fmtflags fmt = s.flags();
  streamsize prec = s.precision();
  for( int k = 0; k < ME.order; ++k ) {
    s << scientific << setw(w) << setprecision(6) << ME.poly[k];
  }
  s.flags(fmt);
  s.precision(prec);
}

//-----------------------------------------------------------------------------
int VDC::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";

  // Build the matrix element string
  ostringstream s;
  int nME = 0;
  ssiz_t w = 14;
  const vector<THaMatrixElement>* allME[] = { &fFPMatrixElems, &fDMatrixElems,
					      &fTMatrixElems,  &fPMatrixElems,
					      &fYMatrixElems,  &fLMatrixElems,
					      &fYTAMatrixElems, &fPTAMatrixElems,
					      0 };
  const char* all_names[] = { "FP", "D", "T", "P", "Y", "L", "YTA", "PTA" };
  const vector<THaMatrixElement>** mat = allME;
  int i = 0;
  while( *mat ) {
    if( *mat == &fFPMatrixElems ) {
      const char* fp_names[] = { "t", "y", "p" };
      for( int i = 0; i < 3; ++i ) {
	WriteME( s, fFPMatrixElems[i], fp_names[i], w );
	++nME;
      }
    } else {
      for( vector<THaMatrixElement>::const_iterator mt = (*mat)->begin();
	   mt != (*mat)->end(); ++mt ) {
	WriteME( s, *mt, all_names[i], w );
	++nME;
      }
    }
    ++mat;
    ++i;
  }
  s << endl;

  AddToMap( prefix+"max_matcherr", MakeValue(&fErrorCutoff), start, version );
  AddToMap( prefix+"num_iter",     MakeValue(&fNumIter),     start, version );

  const VDC::Plane& pl = fPlanes[0];
  if( TestBit(fCommon,kNelem) )
    AddToMap( prefix+"nwires",        MakeValue(&pl.fNelem),         start, version );
  if( TestBit(fCommon,kWSpac) )
    AddToMap( prefix+"wire.spacing",  MakeValue(&pl.fWSpac),         start, version );
  if( TestBit(fCommon,kDriftVel) )
    AddToMap( prefix+"driftvel",      MakeValue(&pl.fDriftVel),      start, version );
  if( TestBit(fCommon,kMinTime) )
    AddToMap( prefix+"tdc.min",       MakeValue(&pl.fMinTime),       start, version );
  if( TestBit(fCommon,kMaxTime) )
    AddToMap( prefix+"tdc.max",       MakeValue(&pl.fMaxTime),       start, version );
  if( TestBit(fCommon,kTDCRes) )
    AddToMap( prefix+"tdc.res",       MakeValue(&pl.fTDCRes),        start, version );
  if( TestBit(fCommon,kTTDConv) )
    AddToMap( prefix+"ttd.converter", MakeValue(&pl.fTTDConv),       start, version );
  if( TestBit(fCommon,kTTDPar) )
    AddToMap( prefix+"ttd.param",     MakeValue(&pl.fTTDPar[0],9),   start, version, -4 );
  if( TestBit(fCommon,kT0Resolution) )
    AddToMap( prefix+"t0.res",        MakeValue(&pl.fT0Resolution),  start, version );
  if( TestBit(fCommon,kMinClustSize) )
    AddToMap( prefix+"clust.minsize", MakeValue(&pl.fMinClustSize),  start, version );
  if( TestBit(fCommon,kMaxClustSpan) )
    AddToMap( prefix+"clust.maxspan", MakeValue(&pl.fMaxClustSpan),  start, version );
  if( TestBit(fCommon,kNMaxGap) )
    AddToMap( prefix+"maxgap",        MakeValue(&pl.fNMaxGap),       start, version );
  if( TestBit(fCommon,kMinTdiff) )
    AddToMap( prefix+"tdiff.min",     MakeValue(&pl.fMinTdiff),      start, version );
  if( TestBit(fCommon,kMaxTdiff) )
    AddToMap( prefix+"tdiff.max",     MakeValue(&pl.fMaxTdiff),      start, version );

  // Per-plane data
  for( int i = 0; i < 4; ++i )
    fPlanes[i].Save( start, version );

  AddToMap( prefix+"coord_type",   MakeValue(&fCoordType),   start, version );
  if( nME > 0 )
    AddToMap( prefix+"matrixelem", Value_t(s.str(),nME,w), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int VDC::Plane::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";

  int flags = 1;
  AddToMap( prefix+"detmap", MakeValue(fDetMap,flags), start, version, 4+flags );
  AddToMap( prefix+"position", MakeValue(&fOrigin),      start, version );

  UInt_t cbits = fVDC->GetCommon();
  if( !TestBit(cbits,kNelem) )
    AddToMap( prefix+"nwires",        MakeValue(&fNelem),         start, version );
  AddToMap( prefix+"wire.start",      MakeValue(&fWBeg),          start, version );
  if( !TestBit(cbits,kWSpac) )
    AddToMap( prefix+"wire.spacing",  MakeValue(&fWSpac),         start, version );
  AddToMap( prefix+"wire.angle",      MakeValue(&fWAngle),        start, version );
  if( !fBadWires.empty() )
    AddToMap( prefix+"wire.badlist",    MakeValue(&fBadWires[0],fBadWires.size()),
	      start, version );
  if( !TestBit(cbits,kDriftVel) )
    AddToMap( prefix+"driftvel",      MakeValue(&fDriftVel),      start, version );
  if( !TestBit(cbits,kMinTime) )
    AddToMap( prefix+"tdc.min",       MakeValue(&fMinTime),       start, version );
  if( !TestBit(cbits,kMaxTime) )
    AddToMap( prefix+"tdc.max",       MakeValue(&fMaxTime),       start, version );
  if( !TestBit(cbits,kTDCRes) )
    AddToMap( prefix+"tdc.res",       MakeValue(&fTDCRes),        start, version );
  AddToMap( prefix+"tdc.offsets",     MakeValue(&fTDCOffsets[0],fNelem),
	    start, version, 8 );
  if( !TestBit(cbits,kTTDConv) )
    AddToMap( prefix+"ttd.converter", MakeValue(&fTTDConv),       start, version );
  if( !TestBit(cbits,kTTDPar) )
    AddToMap( prefix+"ttd.param",     MakeValue(&fTTDPar[0],9),   start, version, -4 );
  if( !TestBit(cbits,kT0Resolution) )
    AddToMap( prefix+"t0.res",        MakeValue(&fT0Resolution),  start, version );
  if( !TestBit(cbits,kMinClustSize) )
    AddToMap( prefix+"clust.minsize", MakeValue(&fMinClustSize),  start, version );
  if( !TestBit(cbits,kMaxClustSpan) )
    AddToMap( prefix+"clust.maxspan", MakeValue(&fMaxClustSpan),  start, version );
  if( !TestBit(cbits,kNMaxGap) )
    AddToMap( prefix+"maxgap",        MakeValue(&fNMaxGap),       start, version );
  if( !TestBit(cbits,kMinTdiff) )
    AddToMap( prefix+"tdiff.min",     MakeValue(&fMinTdiff),      start, version );
  if( !TestBit(cbits,kMaxTdiff) )
    AddToMap( prefix+"tdiff.max",     MakeValue(&fMaxTdiff),      start, version );
  AddToMap( prefix+"description",     MakeValue(&fDescription),   start, version );

  return 0;
}

//-----------------------------------------------------------------------------
void Detector::RegisterDefaults()
{
  // Register default values for certain keys

  fDefaults["nelem"] = "0";
  fDefaults["angle"] = "0";
  fDefaults["position"] = "0";
  fDefaults["size"] = "0";
}

//-----------------------------------------------------------------------------
void Cherenkov::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["offsets"] = "0";
  fDefaults["pedestals"] = "0";
  fDefaults["gains"] = "1";
}

//-----------------------------------------------------------------------------
void Scintillator::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["L.off"] = "0";
  fDefaults["L.ped"] = "0";
  fDefaults["L.gain"] = "1";
  fDefaults["R.off"] = "0";
  fDefaults["R.ped"] = "0";
  fDefaults["R.gain"] = "1";
}

//-----------------------------------------------------------------------------
void Shower::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["pedestals"] = "0";
  fDefaults["gains"] = "1";
}

//-----------------------------------------------------------------------------
void BPM::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["pedestals"] = "0";
  fDefaults["rotmatrix"] = "0"; // FIXME: correct?
}

//-----------------------------------------------------------------------------
void Raster::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["rast_peds"] = "0";
  fDefaults["slope_peds"] = "0";
}

//-----------------------------------------------------------------------------
void CoincTime::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["tdc_offset"] = "0";
}

//-----------------------------------------------------------------------------
void VDC::RegisterDefaults()
{
  // Register default values for certain keys

  Detector::RegisterDefaults();
  fDefaults["max_matcherr"] = "1e+09";
  fDefaults["num_iter"] = "1";
}

//-----------------------------------------------------------------------------
int Detector::AddToMap( const string& key, const Value_t& v, time_t start,
			const string& version, int maxv ) const
{
  // Add given key and value with given validity start time and optional
  // "version" (secondary index) to the in-memory database.
  // If value is empty, do nothing (i.e. bail if MakeValue fails).

  if( v.value.empty() )
    return 0;
  assert( v.nelem > 0 );
  assert( v.width > 0 );

  // Ensure that each key can only be associated with one detector name
  StrMap_t::iterator itn = gKeyToDet.find( key );
  if( itn == gKeyToDet.end() ) {
    gKeyToDet.insert( make_pair(key,fDBName) );
    gDetToKey.insert( make_pair(fDBName,key) );
  }
  else if( itn->second != fDBName ) {
    cerr << "Error: key " << key << " already previously found for "
	 << "detector " << itn->second << ", now for " << fDBName << endl;
    return 1;
  }

  DBvalue val( v, start, version, maxv );
  KeyAttr_t& attr = gDB[key];
  ValSet_t& vals = attr.values;
  // Find existing values with the exact timestamp of 'val' (='start')
  ValSet_t::iterator pos = vals.find(val);
  if( pos != vals.end() ) {
    if( pos->value != val.value ) {
      // User database inconsistent (FIXME: can this ever happen now?)
      // (This case is silently ignored in THaAnalysisObject::LoadDBvalue,
      // which simply takes the last value encountered.)
      cerr << "WARNING: key " << key << " already exists for time "
	   << format_time(start);
      if( !version.empty() )
	cerr << " and version \"" << version << "\"";
      cerr << ", but with a different value:" << endl;
      cerr << " old = " << pos->value << endl;
      cerr << " new = " << v.value << endl;
      const_cast<DBvalue&>(*pos).value = v.value;
    }
    return 0;
  }
#ifdef NDEBUG
  vals.insert(val);
#else
  assert( vals.insert(val).second );
#endif
  return 0;
}

//-----------------------------------------------------------------------------
int CopyFile::AddToMap( const string& key, const Value_t& value, time_t start,
			const string& version, int maxv ) const
{
  int err = Detector::AddToMap(key, value, start, version, maxv);
  if( err )
    return err;

  if( fDoingFileCopy ) {
    KeyAttr_t& attr = gDB[key];
    attr.isCopy = true;
  }
  return 0;
}

//-----------------------------------------------------------------------------
