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
#include <cstring>    // for GNU basename()
#include <unistd.h>   // for getopt
#include <popt.h>
#include <ctime>
#include <map>
#include <set>
#include <limits>
#include <utility>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h> // for stat/lstat
#include <dirent.h>   // for opendir/readdir
#include <cctype>     // for isdigit, tolower
#include <cerrno>

#include "TString.h"
#include "TDatime.h"
#include "TMath.h"
#include "TVector3.h"
#include "TError.h"

#include "THaAnalysisObject.h"
#include "THaDetMap.h"

#define kInitError THaAnalysisObject::kInitError
#define kOK        THaAnalysisObject::kOK
#define kBig       THaAnalysisObject::kBig

using namespace std;

#define ALL(c) (c).begin(), (c).end()

static bool IsDBdate( const string& line, time_t& date );

// Command line parameter defaults
static int do_debug = 0, verbose = 0, do_file_copy = 1, do_subdirs = 0;
static int do_clean = 1, do_verify = 1, do_dump = 0, purge_all_default_keys = 1;
static string srcdir;
static string destdir;
static const char* prgname = 0;
static char* mapfile = 0;
static string current_filename;

static struct poptOption options[] = {
  //  POPT_AUTOHELP
  { "help",     'h', POPT_ARG_NONE,   0, 'h', 0, 0  },
  { "verbose",  'v', POPT_ARG_VAL,    &verbose,  1, 0, 0  },
  { "debug",    'd', POPT_ARG_VAL,    &do_debug, 1, 0, 0  },
  { "mapfile",  'm', POPT_ARG_STRING, &mapfile,  0, 0, 0  },
  // "detlist", 'l', ... // list of wildcards of detector names
  { "preserve-subdirs",    's', POPT_ARG_VAL,    &do_subdirs, 1, 0, 0  },
  { "no-preserve-subdirs", 0, POPT_ARG_VAL,    &do_subdirs, 0, 0, 0  },
  { "no-clean",  0, POPT_ARG_VAL,    &do_clean, 0, 0, 0  },
  { "no-verify", 0, POPT_ARG_VAL,    &do_verify, 0, 0, 0  },
  POPT_TABLEEND
};

// Information for a single source database file
struct Filenames_t {
  Filenames_t( const string& _path, time_t _start )
    : path(_path), val_start(_start) {}
  Filenames_t( time_t _start ) : val_start(_start) {}
  string    path;
  time_t    val_start;
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
void help()
{
  // Print help message and exit

  cerr << "Usage: " << prgname << " [options] SRC_DIR  DEST_DIR" << endl;
  cerr << " Convert Podd 1.5 and earlier database files under SRC_DIR to Podd 1.6" << endl;
  cerr << " and later format, written to DEST_DIR" << endl;
  //  cerr << endl;
  cerr << "Options:" << endl;
  cerr << " -h, --help\t\t\tshow this help message" << endl;
  cerr << " -v, --verbose\t\t\tincrease verbosity" << endl;
  cerr << " -d, --debug\t\t\tprint extensive debug info" << endl;
  // cerr << " -o <outfile>: Write output to <outfile>. Default: "
  //      << OUTFILE_DEFAULT << endl;
  // cerr << " <infile>: Read input from <infile>. Default: "
  //      << INFILE_DEFAULT << endl;
  exit(0);
}

//-----------------------------------------------------------------------------
void usage( poptContext& pc )
{
  // Print usage message and exit with error code

  poptPrintUsage(pc, stderr, 0);
  exit(1);
}

//-----------------------------------------------------------------------------
void getargs( int argc, const char** argv )
{
  // Get command line parameters

  prgname = basename(argv[0]);

  poptContext pc = poptGetContext("dbconvert", argc, argv, options, 0);
  poptSetOtherOptionHelp(pc, "SRC_DIR DEST_DIR");

  int opt;
  while( (opt = poptGetNextOpt(pc)) > 0 ) {
    switch( opt ) {
    case 'h':
      help();
    default:
      cerr << "Unhandled option " << (char)opt << endl;
      exit(1);
    }
  }
  if( opt < -1 ) {
    cerr << poptBadOption(pc, POPT_BADOPTION_NOALIAS) << ": "
	 << poptStrerror(opt) << endl;
    usage(pc);
  }

  const char* arg = poptGetArg(pc);
  if( !arg ) {
    cerr << "Error: Must specify SRC_DIR and DEST_DIR" << endl;
    usage(pc);
  }
  srcdir = arg;
  arg = poptGetArg(pc);
  if( !arg ) {
    cerr << "Error: Must specify DEST_DIR" << endl;
    usage(pc);
  }
  destdir = arg;
  if( poptPeekArg(pc) ) {
    cerr << "Error: too many arguments" << endl;
    usage(pc);
  }
  //DEBUG
  if( mapfile )
    cout << "Mapfile name is \"" << mapfile << "\"" << endl;
  cout << "Converting from \"" << srcdir << "\" to \"" << destdir << "\"" << endl;

  poptFreeContext(pc);
}

//-----------------------------------------------------------------------------
static inline time_t MkTime( int yy, int mm, int dd, int hh, int mi, int ss )
{
  struct tm td;
  td.tm_sec   = ss;
  td.tm_min   = mi;
  td.tm_hour  = hh;
  td.tm_year  = yy-1900;
  td.tm_mon   = mm-1;
  td.tm_mday  = dd;
  td.tm_isdst = -1;
  return mktime( &td );
}

//-----------------------------------------------------------------------------
static inline string format_time( time_t t )
{
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
  struct tm tms;
  gmtime_r( &t, &tms );
  stringstream ss;
  ss << "--------[ ";
  ss << tms.tm_year+1900 << "-";
  ss << setw(2) << setfill('0') << tms.tm_mon+1 << "-";
  ss << setw(2) << setfill('0') << tms.tm_mday  << " ";
  ss << setw(2) << setfill('0') << tms.tm_hour  << ":";
  ss << setw(2) << setfill('0') << tms.tm_min   << ":";
  ss << setw(2) << setfill('0') << tms.tm_sec;
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
static inline bool IsDBSubDir( const string& fname, time_t& date )
{
  // Check if the given file name corresponds to a database subdirectory.
  // Subdirectories have filenames of the form "YYYYMMDD" and "DEFAULT".
  // If so, extract its encoded time stamp to 'date'

  if( fname == "DEFAULT" ) {
    date = 0;
    return true;
  }
  if( fname.size() != 8 )
    return false;

  ssiz_t pos = 0;
  for( ; pos<8; ++pos )
    if( !isdigit(fname[pos])) break;
  if( pos != 8 )
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
template <class T> string MakeValue( const T* array, int size = 0 )
{
  ostringstream ostr;
  if( size == 0 ) size = 1;
  for( int i = 0; i < size; ++i ) {
    ostr << array[i];
    if( i+1 < size ) ostr << " ";
  }
  return ostr.str();
}

//-----------------------------------------------------------------------------
static inline string MakeDetmapElemValue( const THaDetMap* detmap, int n,
					  int extras )
{
  ostringstream ostr;
  THaDetMap::Module* d = detmap->GetModule(n);
  ostr << d->crate << " " << d->slot << " " << d->lo << " " << d->hi;
  if( extras >= 1 ) ostr << " " << d->first;
  if( extras >= 2 ) ostr << " " << d->GetModel();
  return ostr.str();
}

//-----------------------------------------------------------------------------
template<> string MakeValue( const THaDetMap* detmap, int extras )
{
  ostringstream ostr;
  for( Int_t i = 0; i < detmap->GetSize(); ++i ) {
    ostr << MakeDetmapElemValue( detmap, i, extras );
    if( i+1 != detmap->GetSize() ) ostr << " ";
  }
  return ostr.str();
}

//-----------------------------------------------------------------------------
template<> string MakeValue( const TVector3* vec3, int )
{
  ostringstream ostr;
  ostr << vec3->X() << " " << vec3->Y() << " " << vec3->Z();
  return ostr.str();
}

//-----------------------------------------------------------------------------
// Define data structures for local caching of database parameters

struct DBvalue {
  DBvalue( const string& valstr, time_t start, const string& ver = string(),
	   int max = 0 )
    : value(valstr), validity_start(start), version(ver), max_per_line(max) {}
  DBvalue( time_t start, const string& ver = string() )
    : validity_start(start), version(ver), max_per_line(0) {}
  string value;
  time_t validity_start;
  string version;
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
void DumpMap( ostream& os = std::cout )
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
int PruneMap()
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
  int nwritten = 0;
  bool header_done = false;
  DBvalue tstamp_val(string(),tstamp);
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

    if( !header_done ) {
      if( tstamp > 0 )
	ofs << format_tstamp(tstamp) << endl << endl;
      header_done = true;
    }

    ofs << key << " = " << vt->value << endl;
    ++nwritten;
  }
  if( nwritten > 0 )
    ofs << endl;

  return nwritten;
}

//-----------------------------------------------------------------------------
int WriteFileDB( const string& target_dir, const vector<string>& subdirs )
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
#ifdef NDEBUG
      IsDBSubDir(subdirs[i],date);
#else
      assert( IsDBSubDir(subdirs[i],date) );
#endif
      dir_times.insert(date);
      dir_names.insert( make_pair(date, MakePath(target_dir,subdirs[i])) );
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
      // Don't create empty files (may never happen?)
      assert( nw > 0 );
      // if( nw == 0 ) {
      // 	unlink( fname.c_str() );
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
    : fName(name), fDetMap(new THaDetMap), fDetMapHasModel(false),
      fNelem(0), fAngle(0) /*, fXax(1.,0,0), fYax(0,1.,0), fZax(0,0,1.) */{
    fSize[0] = fSize[1] = fSize[2] = 0.;
    string::size_type pos = fName.find('.');
    if( pos == string::npos )
      pos = 0;
    else
      ++pos;
    fRealName = fName.substr(pos);
  }
  virtual ~Detector() { delete fDetMap; }

  virtual void Clear() {
    fConfig = ""; fDetMap->Clear(); fDetMapHasModel = false;
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

protected:
  virtual int AddToMap( const string& key, const string& value, time_t start,
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
    Error( Here(here), msg.str().c_str() );
    return kInitError;
  }

  // Bits for flags parameter of ReadBlock()
  enum kReadBlockFlags {
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
  enum kReadBlockRetvals {
    kSuccess = 0, kNoValues = -1, kTooFewValues = -2, kTooManyValues = -3,
    kFileError = -4, kNegativeFound = -5, kLessEqualZeroFound = -6 };
  template <class T>
  int ReadBlock( FILE* fi, T* data, int nval, const char* here, int flags = 0 );

  string      fName;    // Detector "name", actually the prefix without trailing dot
  string      fRealName;// Actual detector name (top level dropped)
  TString     fConfig;  // TString for compatibility with old API
  THaDetMap*  fDetMap;
  bool        fDetMapHasModel;
  Int_t       fNelem;
  Double_t    fSize[3];
  Double_t    fAngle;
  TVector3    fOrigin;//, fXax, fYax, fZax;

  StrMap_t    fDefaults;

private:
  bool TestBit(int flags, int bit) { return (flags & bit) != 0; }

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
  virtual int AddToMap( const string& key, const string& value, time_t start,
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
    : Detector(name), fLOff(0), fROff(0), fLPed(0), fRPed(0), fLGain(0),
      fRGain(0), fTWalkPar(0), fTrigOff(0) {}
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

// Global maps for detector types and names
enum EDetectorType { kNone = 0, kKeep, kCopyFile, kCherenkov, kScintillator,
		     kShower, kTotalShower, kBPM, kRaster, kCoincTime,
		     kTriggerTime, kVDC, kVDCeff, kDecData };
typedef map<string,EDetectorType> NameTypeMap_t;
static NameTypeMap_t detname_map;
static NameTypeMap_t dettype_map;

struct StringToType_t {
  const char*   name;
  EDetectorType type;
};

//-----------------------------------------------------------------------------
// Shower
class Shower : public Detector {
public:
  Shower( const string& name )
    : Detector(name), fPed(0), fGain(0) {}
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
  BPM( const string& name ) : Detector(name) {}

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
    return 0; //TODO
  case kVDCeff:
    return 0; //TODO
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
    pair<NameTypeMap_t::iterator, bool> ins =
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
    { "run",        kCopyFile },
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
    { "BB.ts.ps",   kShower },
    { "BB.ts.sh",   kShower },
    { "R.ts",       kTotalShower },
    { "L.ts",       kTotalShower },
    { "BB.ts",      kTotalShower },
    { "CT",         kCoincTime },
    // { "R.vdc",      kVDC },
    // { "L.vdc",      kVDC },
    // { "vdceff",     kVDCeff },
    // Ignore top-level Beam apparatus db files - these are actually never read.
    // Instead, the apparatus's detector read db_urb.BPMA.dat etc.
    { "beam",       kNone },
    { "rb",         kNone },
    { "Rrb",        kNone },
    { "Lrb",        kNone },
    { "urb",        kNone },
    { "Rurb",       kNone },
    { "Lurb",       kNone },
    // Apparently Bodo's database scheme was so confusing that people started
    // defining BPM database for rastered beams, which only use the Raster
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
    { "BB.mwdc",    kCopyFile },
    { 0,            kNone }
  };
  for( StringToType_t* item = defaults; item->name; ++item ) {
    pair<NameTypeMap_t::iterator, bool> ins =
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
  size_t len = offsetof(struct dirent, d_name) + pathconf(cdir, _PC_NAME_MAX) + 1;
  struct dirent *ent = (struct dirent*)malloc(len), *result;
  int err;
  bool unfinished;
  do {
    unfinished = false;
    while( (err = readdir_r(dir,ent,&result)) == 0 && result != 0 ) {
      // Skip trivial file names
      string fname(result->d_name);
      if( fname.empty() || fname == "." || fname == ".." )
	continue;
      // Operate on the name
      if( (err = action(sdir, fname, depth)) < 0 )
	break;
      n_add += err;
      err = 0;
    }
    // In case 'action' caused the directory contents to change, rewind the
    // directory and scan it again (needed for MacOS HFS file systems, perhaps
    // elsewhere too)
    if( action.MustRewind() ) {
      rewinddir(dir);
      unfinished = true;
    }
  } while( unfinished );

  free(ent);
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
      fFilenames[GetDetName(fpath)].insert( Filenames_t(fpath,fStart) );
      n_add = 1;
    }

    // Recurse down one level into valid subdirectories ("YYYYMMDD" and "DEFAULT")
    else if( S_ISDIR(sb.st_mode) && depth == 0 ) {
      time_t date;
      if( IsDBSubDir(fname,date) ) {
	int err =
	  ForAllFilesInDir(fpath,CopyDBFileName(fFilenames,fSubdirs,date),depth+1);
	if( err < 0 )
	  return err;
	n_add = err;
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
    string fpath = MakePath( dir, fname );
    const char* cpath = fpath.c_str();
    time_t date;
    struct stat sb;

    if( lstat(cpath, &sb) ) {
      perror(cpath);
      return -1;
    }
    // Non-directory (regular, links) files matching db_*.dat or a subdirectory name
    if( !S_ISDIR(sb.st_mode) &&
	(fDoAll || IsDBFileName(fname) || IsDBSubDir(fname,date)) ) {
      if( unlink(cpath) ) {
      	perror(cpath);
	return -2;
      }
      fMustRewind = true;
    }

    // Recurse down into valid subdirectories ("YYYYMMDD" and "DEFAULT")
    else if( S_ISDIR(sb.st_mode) ) {
      if( depth > 0 || IsDBSubDir(fname,date) ) {
	// Complete wipe all date-coded subdirectories, otherwise the logic in
	// THaAnalsysisObject::GetDBFileList may not work correctly. That function finds
	// the closest-matching date-coded directory and then assumes that the
	// requested file is in that directory. This logic may fail if a closer-matching
	// directory exists in the target than in the source for a certain timestamp.
	bool delete_all = (fname != "DEFAULT");
 	if( ForAllFilesInDir(fpath, DeleteDBFile(delete_all), depth+1) )
	  return -2;
	int err;
	if( (err = rmdir(cpath)) && errno != ENOTEMPTY ) {
	  perror(cpath);
	  return -2;
	}
	if( !err )
	  fMustRewind = true;
      }
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

  if( lstat(cpath,&sb) ) {
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
    register char *p = buf;
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
static int ParseTimestamps( FILE* fi, set<time_t>& timestamps )
{
  // Put all timestamps from file 'fi' in given vector 'timestamps'

  size_t LEN = 256;
  char buf[LEN];
  string line;

  rewind(fi);
  while( GetLine(fi,buf,LEN,line) == 0 ) {
    time_t date;
    if( IsDBdate(line, date) )
      timestamps.insert(date);
  }
  return 0;
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
#ifdef NDEBUG
	IsDBSubdir(subdir,date);
	filenames.insert( Filenames_t(deffile,date) );
#else
	assert( IsDBSubDir(subdir,date) );
	Filenames_t ins(deffile,date);
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
int main( int argc, const char** argv )
{
  // Parse command line
  getargs(argc,argv);

  // Read the detector name mapping file. If unavailable, set up defaults.
  if( mapfile ) {
    if( ReadMapfile(mapfile) )
      exit(5);  // Error message already printed
  } else {
    DefaultMap();
  }

  // Get list of all database files to convert
  FilenameMap_t filemap;
  vector<string> subdirs;
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

    fiter_t lastf = filenames.insert( Filenames_t(numeric_limits<time_t>::max()) );
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
	if( ParseTimestamps(fi, timestamps) )
	  goto next;
      }
      if( det->SupportsVariations() ) {
	if( ParseVariations(fi, variations) )
	  goto next;
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

      // Save names of new-format database files to be copied
      if( do_file_copy && type == kCopyFile ) {

      }
    next:
      fclose(fi);
    } // end for st = filenames

    // If requested, remove keys for this detector that only have default values
    if( purge_all_default_keys )
      det->PurgeAllDefaultKeys();
    // Done with this detector
    delete det;
  } // end for ft = filemap

  // Prune the key/value map to remove entries that have the exact
  // same keys/values and only differ by /consecutive/ timestamps.
  // Keep only the earliest timestamp.

  PruneMap();

  // Write out keys/values to database files in target directory.
  // All file names will be preserved; a file that existed anywhere
  // in the source will also appear at least once in the target.
  // User may request that original directory structure be preserved,
  // otherwise just write one file per detector name.
  // Special treatment for keys found in current directory: if requested
  // write the converted versions into a special subdirectory of target.

  if( do_dump )
    DumpMap();

  if( PrepareOutputDir(destdir, subdirs) )
    exit(6);

  if( WriteFileDB(destdir,subdirs) )
    exit(7);

  return 0;
}

//#if 0
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
    return 0;

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
  fseeko(fp, pos, SEEK_SET);
  return 0;
}
//#endif

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
  Int_t yy, mm, dd, hh, mi, ss;
  if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%4d-%2d-%2d %2d:%2d:%2d",
	      &yy, &mm, &dd, &hh, &mi, &ss) != 6
      || yy < 1995 || mm < 1 || mm > 12 || dd < 1 || dd > 31
      || hh < 0 || hh > 23 || mi < 0 || mi > 59 || ss < 0 || ss > 59 ) {
    return false;
  }
  date = MkTime( yy, mm, dd, hh, mi, ss );
  return (date != static_cast<time_t>(-1));
}

//_____________________________________________________________________________
static Int_t IsDBkey( const string& line, string& key, string& text )
{
  // Check if 'line' is of the form "key = value"
  // - If there is no '=', then return 0.
  // - If key found, parse the line, set 'text' to the whitespace-trimmed
  //   text after the "=" and return +1.
  //
  // Note: By construction in ReadDBline, 'line' is not empty, any comments
  // starting with '#' have been removed, and trailing whitespace has been
  // trimmed. Also, all tabs have been converted to spaces.

  // Search for "="
  register const char* ln = line.c_str();
  const char* eq = strchr(ln, '=');
  if( !eq ) return 0;
  // Extract the key
  while( *ln == ' ' ) ++ln; // find_first_not_of(" ")
  assert( ln <= eq );
  if( ln == eq ) return -1;
  register const char* p = eq-1;
  assert( p >= ln );
  while( *p == ' ' ) --p; // find_last_not_of(" ")
  key = string(ln,p-ln+1);
  // Extract the value, trimming leading whitespace.
  ln = eq+1;
  assert( !*ln || *(ln+strlen(ln)-1) != ' ' ); // Trailing space already trimmed
  while( *ln == ' ' ) ++ln;
  text = ln;

  return 1;
}


//-----------------------------------------------------------------------------
template <class T>
int Detector::ReadBlock( FILE* fi, T* data, int nval, const char* here, int flags )
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
  off_t pos, pos_on_entry = ftello(fi);
  std::less<T> std_less;  // to suppress compiler warnings "< 0 is always false"

  errno = 0;
  while( GetLine(fi,buf,LEN,line) == 0 ) {
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
	fseeko(fi,pos,SEEK_SET);
	return kSuccess;
      }
      got_data = true;
      istringstream is(line.c_str());
      while( is && nread < nval ) {
	is >> data[nread];
	if( !is.fail() ) {
	  if( TestBit(flags,kNoNegativeValues) && std_less(data[nread],T(0)) ) {
	    if( !TestBit(flags,kQuietOnErrors) )
	      Error( Here(here), "Negative values not allowed here:\n \"%s\"",
		     line.c_str() );
	    fseeko(fi,pos_on_entry,SEEK_SET);
	    return kNegativeFound;
	  }
	  if( TestBit(flags,kRequireGreaterZero) && data[nread] <= 0 ) {
	    if( !TestBit(flags,kQuietOnErrors) )
	      Error( Here(here), "Values must be greater then zero:\n \"%s\"",
		     line.c_str() );
	    fseeko(fi,pos_on_entry,SEEK_SET);
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
	      Error( Here(here), msg.str().c_str() );
	    else
	      Warning( Here(here), msg.str().c_str() );
	  }
	  fseeko(fi,pos_on_entry,SEEK_SET);
	  return kTooManyValues;
	}
	if( TestBit(flags,kStopAtNval) ) {
	  // If requested, stop here, regardless of whether there is another data line
	  return kSuccess;
	}
      }
    } else {
      // Comment
      found_section = TestBit(flags,kStopAtSection)
	&& line.find('[') != string::npos;
      if( got_data || found_section ) {
	if( nread < nval )
	  goto toofew;
	// Success
	// Rewind to start of terminating line
	fseeko(fi,pos,SEEK_SET);
	return kSuccess;
      }
    }
    pos = ftello(fi);
  }
  if( errno ) {
    perror( Here(here) );
    return kFileError;
  }
  if( nread < nval ) {
  toofew:
    if( !TestBit(flags,kQuietOnErrors) && !TestBit(flags,kQuietOnTooFew) ) {
      ostringstream msg;
      msg << "Too few values (expected " << nval << ") at file position "
	  << ftello(fi) << endl
	  << " Line: \"" << line << "\"";
      Error( Here(here), msg.str().c_str() );
    }
    fseeko(fi,pos_on_entry,SEEK_SET);
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
      istringstream istr( vt->value.c_str() );
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
      // 	   << format_time(curdate) << ", " << key << " = " << value << endl;

      // TODO: add support for "text variables"?

      // Add this key/value pair to our local database
      DBvalue val( value, curdate, version );
      ValSet_t& vals = fDB[key].values;
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
    const ValSet_t& vals = keyval.second.values;
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
      AddToMap( keyval.first, vt->value, date, vt->version,
		vt->max_per_line );
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
    fgets ( buf, LEN, fi );
    int n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		    &crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;
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
    fgets ( buf, LEN, fi );
    n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		&crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;
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
  if( (err = ReadBlock(fi,&fTdc2T,1,here,flags|kRequireGreaterZero)) )
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
  Int_t nclbl; //TODO: use this
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
    nclbl  = ivals[2];
    old_format = true;
  } else {
    fNcols = ivals[0];
    fNrows = ivals[1];
    nclbl  = TMath::Min( 3, fNrows ) * TMath::Min( 3, fNcols );
  }
  Int_t nelem = fNcols * fNrows;

  // Read detector map
  fDetMap->Clear();
  while( ReadComment(fi,buf,LEN) );
  while (1) {
    Int_t crate, slot, first, last;
    int n = fscanf ( fi,"%6d %6d %6d %6d", &crate, &slot, &first, &last );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( n < 4 ) return ErrPrint(fi,here);
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
    fgets( buf, LEN, fi);
    n = sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d",
	       &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if( n < 1 ) return ErrPrint(fi,here);
    if( n == 7 )
      fDetMapHasModel = true;
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
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf",&dummy1,&dummy2,&dummy3,&dummy4);
  if( n < 2 ) return ErrPrint(fi,here);

  // dummy 1 is the z position of the bpm. Used below to set fOrigin.

  // calibration constant, historical
  fCalibRot = dummy2;

  // dummy3 and dummy4 are not used in this apparatus

  // Pedestals
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf",fPed,fPed+1,fPed+2,fPed+3);
  if( n != 4 ) return ErrPrint(fi,here);

  // 2x2 transformation matrix and x/y offset
  fgets( buf, LEN, fi);
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
    fgets( buf, LEN, fi);
    n = sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d",
	       &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if( n < 1 ) return ErrPrint(fi,here);
    if( n == 7 )
      fDetMapHasModel = true;
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

  fgets( buf, LEN, fi);
  // NB: dummy1 is never used. Comment in old db files says it is "z-pos",
  // which are read from the following lines - probably dummy1 is leftover junk
  double dummy1;
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf %15lf",
	     &dummy1,fFreq,fFreq+1,fRPed,fRPed+1,fSPed,fSPed+1);
  if( n != 7 ) return ErrPrint(fi,here);

  // z positions of BPMA, BPMB, and target reference point (usually 0)
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos);
  if( n != 1 ) return ErrPrint(fi,here);
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos+1);
  if( n != 1 ) return ErrPrint(fi,here);
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos+2);
  if( n != 1 ) return ErrPrint(fi,here);

  // Find timestamp, if any, for the raster constants
  TDatime datime(date);
  THaAnalysisObject::SeekDBdate( fi, datime, true );

  // Raster constants: offx/y,amplx/y,slopex/y for BPMA, BPMB, target
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	     fCalibA,fCalibA+1,fCalibA+2,fCalibA+3,fCalibA+4,fCalibA+5);
  if( n != 6 ) return ErrPrint(fi,here);

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 fCalibB,fCalibB+1,fCalibB+2,fCalibB+3,fCalibB+4,fCalibB+5);
  if( n != 6 ) return ErrPrint(fi,here);

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
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
    fgets ( buf, LEN, fi );
    nread = sscanf( buf, "%6d %6d %6d %6d %15lf %20s %15lf",
		    &crate, &slot, &first, &model, &tres, label, &toff );
    if( crate < 0 ) break;
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
int Detector::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";
  int flags = 1;
  if( fDetMapHasModel )  flags++;
  AddToMap( prefix+"detmap",   MakeValue(fDetMap,flags), start, version, 4+flags );
  AddToMap( prefix+"nelem",    MakeValue(&fNelem),       start, version );
  AddToMap( prefix+"angle",    MakeValue(&fAngle),       start, version );
  AddToMap( prefix+"position", MakeValue(&fOrigin),      start, version );
  AddToMap( prefix+"size",     MakeValue(fSize,3),       start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Cherenkov::Save( time_t start, const string& version ) const
{
  // Create database keys for current Cherenkov configuration data

  string prefix = fName + ".";

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

  Detector::Save( start, version );

  AddToMap( prefix+"L.off",    MakeValue(fLOff,fNelem),  start, version );
  AddToMap( prefix+"L.ped",    MakeValue(fLPed,fNelem),  start, version );
  AddToMap( prefix+"L.gain",   MakeValue(fLGain,fNelem), start, version );
  AddToMap( prefix+"R.off",    MakeValue(fROff,fNelem),  start, version );
  AddToMap( prefix+"R.ped",    MakeValue(fRPed,fNelem),  start, version );
  AddToMap( prefix+"R.gain",   MakeValue(fRGain,fNelem), start, version );

  if( fHaveExtras ) {
    AddToMap( prefix+"avgres",   MakeValue(&fResolution), start, version );
    AddToMap( prefix+"atten",    MakeValue(&fAttenuation), start, version );
    AddToMap( prefix+"Cn",       MakeValue(&fCn),start, version );
    AddToMap( prefix+"MIP",      MakeValue(&fAdcMIP), start, version );
    AddToMap( prefix+"tdc.res",  MakeValue(&fTdc2T), start, version );

    AddToMap( prefix+"timewalk_params",  MakeValue(fTWalkPar,fNTWalkPar), start, version );
    AddToMap( prefix+"retiming_offsets", MakeValue(fTrigOff,fNelem), start, version );
  }

  return 0;
}

//-----------------------------------------------------------------------------
int Shower::Save( time_t start, const string& version ) const
{
  // Create database keys for current Shower configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  if( !fChanMap.empty() )
    AddToMap( prefix+"chanmap", MakeValue(&fChanMap[0], fChanMap.size()),
	      start, version );

  AddToMap( prefix+"ncols",     MakeValue(&fNcols), start, version );
  AddToMap( prefix+"nrows",     MakeValue(&fNrows), start, version );
  AddToMap( prefix+"xy",        MakeValue(fXY,2),   start, version );
  AddToMap( prefix+"dxdy",      MakeValue(fDXY,2),  start, version );
  AddToMap( prefix+"emin",      MakeValue(&fEmin),  start, version );
  if( fMaxCl != -1 )
    AddToMap( prefix+"maxcl",   MakeValue(&fMaxCl), start, version );

  AddToMap( prefix+"pedestals", MakeValue(fPed,fNelem),  start, version );
  AddToMap( prefix+"gains",     MakeValue(fGain,fNelem), start, version );

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
  AddToMap( prefix1+"detmap",     MakeDetmapElemValue(fDetMap,0,2), start, version );
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
void Detector::RegisterDefaults()
{
  // Register default values for certain keys

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
int Detector::AddToMap( const string& key, const string& value, time_t start,
			const string& version, int max ) const
{
  // Add given key and value with given validity start time and optional
  // "version" (secondary index) to the in-memory database.
  // If value is empty, do nothing.

  if( value.empty() )
    return 0;

  // Ensure that each key can only be associated with one detector name
  StrMap_t::iterator itn = gKeyToDet.find( key );
  if( itn == gKeyToDet.end() ) {
    gKeyToDet.insert( make_pair(key,fName) );
    gDetToKey.insert( make_pair(fName,key) );
  }
  else if( itn->second != fName ) {
      cerr << "Error: key " << key << " already previously found for "
	   << "detector " << itn->second << ", now for " << fName << endl;
      return 1;
  }

  DBvalue val( value, start, version, max );
  ValSet_t& vals = gDB[key].values;
  // Find existing values with the exact timestamp of 'val' (='start')
  ValSet_t::iterator pos = vals.find(val);
  if( pos != vals.end() ) {
    if( pos->value != val.value ) {
      // User database inconsistent (can this ever happen now?)
      // (This case is silently ignored in THaAnalysisObject::LoadDBvalue,
      // which simply takes the last value encountered.)
      cerr << "WARNING: key " << key << " already exists for time "
	   << format_time(start);
      if( !version.empty() )
	cerr << " and version \"" << version << "\"";
      cerr << ", but with a different value:" << endl;
      cerr << " old = " << pos->value << endl;
      cerr << " new = " << value << endl;
      const_cast<DBvalue&>(*pos).value = value;
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
int CopyFile::AddToMap( const string& key, const string& value, time_t start,
			const string& version, int max ) const
{
  int err = Detector::AddToMap(key, value, start, version, max);
  if( err )
    return err;

  if( fDoingFileCopy ) {
    KeyAttr_t& attr = gDB[key];
    attr.isCopy = true;
  }
  return 0;
}

//-----------------------------------------------------------------------------

#if 0
  //------ OLD OLD OLD OLD ----------------


// ---- VDC ----
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // load global VDC parameters
  const char* const here = "VDC::ReadDB";
  const int LEN = 200;
  char buff[LEN];

  // Look for the section [<prefix>.global] in the file, e.g. [ R.global ]
  TString tag(fPrefix);
  Ssiz_t pos = tag.Index(".");
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]");

  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != 0) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section %s not found!", tag.Data() );
    fclose(file);
    return kInitError;
  }

  // We found the section, now read the data

  // read in some basic constants first
  //  fscanf(file, "%lf", &fSpacing);
  // fSpacing is now calculated from the actual z-positions in Init(),
  // so skip the first line after [ global ] completely:
  fgets(buff, LEN, file); // Skip rest of line

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
  if( (found = SeekDBdate( file, date )) == 0 && !fConfig.IsNull() &&
      (found = SeekDBconfig( file, fConfig.Data() )) == 0 ) {
    // Print warning if a requested (non-empty) config not found
    Warning( Here(here), "Requested configuration section \"%s\" not "
	     "found in database. Using default (first) section.",
	     fConfig.Data() );
  }

  // Second line after [ global ] or first line after a found tag.
  // After a found tag, it must be the comment line. If not found, then it
  // can be either the comment or a non-found tag before the comment...
  fgets(buff, LEN, file);  // Skip line
  if( !found && IsTag(buff) )
    // Skip one more line if this one was a non-found tag
    fgets(buff, LEN, file);

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
      buff[line.size()-1] = 0;
      line.erase(line.size()-1,1);
    }
    // Split the line into whitespace-separated fields
    vector<string> line_spl = Split(line);

    // Stop if the line does not start with a string referring to
    // a known type of matrix element. In particular, this will
    // stop on a subsequent timestamp or configuration tag starting with "["
    if(line_spl.empty())
      continue; //ignore empty lines
    const char* w = line_spl[0].c_str();
    vsiz_t npow = power[w];
    if( npow == 0 )
      break;

    // Looks like a good line, go parse it.
    THaMatrixElement ME;
    ME.pw.resize(npow);
    ME.iszero = true;  ME.order = 0;
    vsiz_t pos;
    for (pos=1; pos<=npow && pos<line_spl.size(); pos++) {
      ME.pw[pos-1] = atoi(line_spl[pos].c_str());
    }
    vsiz_t p_cnt;
    for ( p_cnt=0; pos<line_spl.size() && p_cnt<kPORDER && pos<=npow+kPORDER;
	  pos++,p_cnt++ ) {
      ME.poly[p_cnt] = atof(line_spl[pos].c_str());
      if (ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    if (p_cnt < 1) {
	Error(Here(here), "Could not read in Matrix Element %s%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	Error(Here(here), "Line looks like: %s",line.c_str());
	fclose(file);
	return kInitError;
    }
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
		    "matrix element: %s. Using first definition.", buff);
	  } else
	    m = ME;
	} else
	  Warning(Here(here), "Bad coefficients of focal plane matrix "
		  "element %s", buff);
      }
      else {
	// All other matrix elements are just appended to the respective array
	// but ensure that they are defined only once!
	bool match = false;
	for( vector<THaMatrixElement>::iterator it = mat->begin();
	     it != mat->end() && !(match = it->match(ME)); ++it ) {}
	if( match ) {
	  Warning(Here(here), "Duplicate definition of "
		  "matrix element: %s. Using first definition.", buff);
	} else
	  mat->push_back(ME);
      }
    }
    else if ( fDebug > 0 )
      Warning(Here(here), "Not storing matrix for: %s !", w);

  } //while(fgets)

  // Compute derived quantities and set some hardcoded parameters
  const Double_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "detector system". By definition,
  // the detector system is identical to the VDC origin in the Hall A HRS.
  DefineAxes(0.0*degrad);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // figure out the track length from the origin to the s1 plane

  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = 0;
  if( GetApparatus() )
    s1 = GetApparatus()->GetDetector("s1");
  if(s1 == 0)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  CalcMatrix(1.,fLMatrixElems); // tensor without explicit polynomial in x_fp

  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  fclose(file);


//----- VDCPlane -----
  const int LEN = 100;
  char buff[LEN];

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Chop(); // delete trailing dot of prefix
  tag.Prepend("["); tag.Append("]");
  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
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
    fclose(file);
    return kInitError;
  }

  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  do {
    fgets( buff, LEN, file );
    // bad kludge to allow a variable number of detector map lines
    if( strchr(buff, '.') ) // any floating point number indicates end of map
      break;
    // Get crate, slot, low channel and high channel from file
    Int_t crate, slot, lo, hi;
    if( sscanf( buff, "%6d %6d %6d %6d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      fclose(file);
      return kInitError;
    }
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (hi - lo + 1);
    nWires += prev_nwires;
  } while( *buff );  // sanity escape
  // Load z, wire beginning postion, wire spacing, and wire angle
  sscanf( buff, "%15lf %15lf %15lf %15lf", &fZ, &fWBeg, &fWSpac, &fWAngle );
  fWAngle *= TMath::Pi()/180.0; // Convert to radians
  fSinAngle = TMath::Sin( fWAngle );
  fCosAngle = TMath::Cos( fWAngle );

  // Load drift velocity (will be used to initialize crude Time to Distance
  // converter)
  fscanf(file, "%15lf", &fDriftVel);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  // first read in the time offsets for the wires
  float* wire_offsets = new float[nWires];
  int*   wire_nums    = new int[nWires];

  for (int i = 0; i < nWires; i++) {
    int wnum = 0;
    float offset = 0.0;
    fscanf(file, " %6d %15f", &wnum, &offset);
    wire_nums[i] = wnum-1; // Wire numbers in file start at 1
    wire_offsets[i] = offset;
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

  // Define time-to-drift-distance converter
  // Currently, we use the analytic converter.
  // FIXME: Let user choose this via a parameter
  delete fTTDConv;
  fTTDConv = new THaVDCAnalyticTTDConv(fDriftVel);

  //THaVDCLookupTTDConv* ttdConv = new THaVDCLookupTTDConv(fT0, fNumBins, fTable);

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!
  for (int i = 0; i < nWires; i++) {
    THaVDCWire* wire = new((*fWires)[i])
      THaVDCWire( i, fWBeg+i*fWSpac, wire_offsets[i], fTTDConv );
    if( wire_nums[i] < 0 )
      wire->SetFlag(1);
  }
  delete [] wire_offsets;
  delete [] wire_nums;

  // Set location of chamber center (in detector coordinates)
  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  // Read additional parameters (not in original database)
  // For backward compatibility with database version 1, these parameters
  // are in an optional section, labelled [ <prefix>.extra_param ]
  // (e.g. [ R.vdc.u1.extra_param ]) or [ R.extra_param ].  If this tag is
  // not found, a warning is printed and defaults are used.

  tag = "["; tag.Append(fPrefix); tag.Append("extra_param]");
  TString tag2(tag);
  found = false;
  rewind(file);
  while (!found && fgets(buff, LEN, file) != NULL) {
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
    tag2 = fPrefix;
    Ssiz_t pos = tag2.Index(".");
    if( pos != kNPOS )
      tag2 = tag2(0,pos+1);
    else
      tag2.Append(".");
    tag2.Prepend("[");
    tag2.Append("extra_param]");
    while (!found && fgets(buff, LEN, file) != NULL) {
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
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    if( fscanf( file, "%d %d %d %d %d %lf %lf", &fMinClustSize, &fMaxClustSpan,
		&fNMaxGap, &fMinTime, &fMaxTime, &fMinTdiff, &fMaxTdiff ) != 7 ) {
      Error( Here(here), "Error reading min_clust_size, max_clust_span, "
	     "max_gap, min_time, max_time, min_tdiff, max_tdiff.\n"
	     "Line = %s\nFix database.", buff );
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    // Time-to-distance converter parameters
    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      // THaVDCAnalyticTTDConv
      // Format: 4*A1 4*A2 dtime(s)  (9 floats)
      Double_t A1[4], A2[4], dtime;
      if( fscanf(file, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
		 &A1[0], &A1[1], &A1[2], &A1[3],
		 &A2[0], &A2[1], &A2[2], &A2[3], &dtime ) != 9) {
	Error( Here(here), "Error reading THaVDCAnalyticTTDConv parameters\n"
	       "Line = %s\nExpect 9 floating point numbers. Fix database.",
	       buff );
	fclose(file);
	return kInitError;
      } else {
	analytic_conv->SetParameters( A1, A2, dtime );
      }
    }
//     else if( (... *conv = dynamic_cast<...*>(fTTDConv)) ) {
//       // handle parameters of other TTD converters here
//     }

    fgets(buff, LEN, file); // Read to end of line

    Double_t h, w;

    if( fscanf(file, "%lf %lf", &h, &w) != 2) {
	Error( Here(here), "Error reading height/width parameters\n"
	       "Line = %s\nExpect 2 floating point numbers. Fix database.",
	       buff );
	fclose(file);
	return kInitError;
      } else {
	   fSize[0] = h/2.0;
	   fSize[1] = w/2.0;
      }

    fgets(buff, LEN, file); // Read to end of line
    // Sanity checks
    if( fMinClustSize < 1 || fMinClustSize > 6 ) {
      Error( Here(here), "Invalid min_clust_size = %d, must be betwwen 1 and "
	     "6. Fix database.", fMinClustSize );
      fclose(file);
      return kInitError;
    }
    if( fMaxClustSpan < 2 || fMaxClustSpan > 12 ) {
      Error( Here(here), "Invalid max_clust_span = %d, must be betwwen 1 and "
	     "12. Fix database.", fMaxClustSpan );
      fclose(file);
      return kInitError;
    }
    if( fNMaxGap < 0 || fNMaxGap > 2 ) {
      Error( Here(here), "Invalid max_gap = %d, must be betwwen 0 and 2. "
	     "Fix database.", fNMaxGap );
      fclose(file);
      return kInitError;
    }
    if( fMinTime < 0 || fMinTime > 4095 ) {
      Error( Here(here), "Invalid min_time = %d, must be betwwen 0 and 4095. "
	     "Fix database.", fMinTime );
      fclose(file);
      return kInitError;
    }
    if( fMaxTime < 1 || fMaxTime > 4096 || fMinTime >= fMaxTime ) {
      Error( Here(here), "Invalid max_time = %d. Must be between 1 and 4096 "
	     "and >= min_time = %d. Fix database.", fMaxTime, fMinTime );
      fclose(file);
      return kInitError;
    }
  } else {
    Warning( Here(here), "No database section \"%s\" or \"%s\" found. "
	     "Using defaults.", tag.Data(), tag2.Data() );
    fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan
    fT0Resolution = 6e-8; // 60 ns --- crude guess
    fMinClustSize = 4;
    fMaxClustSpan = 7;
    fNMaxGap = 0;
    fMinTime = 800;
    fMaxTime = 2200;
    fMinTdiff = 3e-8;   // 30ns  -> ~20 deg track angle
    fMaxTdiff = 1.5e-7; // 150ns -> ~60 deg track angle

    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      Double_t A1[4], A2[4], dtime = 4e-9;
      A1[0] = 2.12e-3;
      A1[1] = A1[2] = A1[3] = 0.0;
      A2[0] = -4.2e-4;
      A2[1] = 1.3e-3;
      A2[2] = 1.06e-4;
      A2[3] = 0.0;
      analytic_conv->SetParameters( A1, A2, dtime );
    }
  }

  THaDetectorBase *sdet = GetParent();
  if( sdet )
    fOrigin += sdet->GetOrigin();

  // finally, find the timing-offset to apply on an event-by-event basis
  //FIXME: time offset handling should go into the enclosing apparatus -
  //since not doing so leads to exactly this kind of mess:
  THaApparatus* app = GetApparatus();
  const char* nm = "trg"; // inside an apparatus, the apparatus name is assumed
  if( !app ||
      !(fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm))) ) {
    Warning(Here(here),"Trigger-time detector \"%s\" not found. "
	    "Event-by-event time offsets will NOT be used!!",nm);
  }

  fIsInit = true;
  fclose(file);



#endif
