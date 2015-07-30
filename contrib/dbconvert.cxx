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
static int do_clean = 1, do_verify = 1, do_dump = 0;
static string srcdir;
static string destdir;
static const char* prgname = 0;
static char* mapfile = 0;

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
template<> string MakeValue( const THaDetMap* detmap, int extras )
{
  ostringstream ostr;
  for( Int_t i = 0; i < detmap->GetSize(); ++i ) {
    THaDetMap::Module* d = detmap->GetModule(i);
    ostr << d->crate << " " << d->slot << " " << d->lo << " " << d->hi;
    if( extras >= 1 ) ostr << " " << d->first;
    if( extras >= 2 ) ostr << " " << d->GetModel();
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
template <class T>
string MakeValueUnless( T trivial_value, const T* array, int size = 0 )
{
  if( size == 0 ) size = 1;
  int i = 0;
  for( ; i < size; ++i ) {
    if( array[i] != trivial_value )
      break;
  }
  if( i == size )
    return string();

  return MakeValue( array, size );
}

//-----------------------------------------------------------------------------
string MakeValueUnless( double triv, const TVector3* vec )
{
  if( vec->X() == triv && vec->Y() == triv && vec->Z() == triv )
    return string();
  return MakeValue( vec );
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
typedef MStrMap_t::const_iterator iter_t;

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
      while( kt != vals.end() ) {
	if( kt->value == jt->value )
	  vals.erase( kt++ );
	else
	  break;
      }
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

protected:
  virtual int AddToMap( const string& key, const string& value, time_t start,
			const string& version = string(), int max = 0 ) const;
  // void DefineAxes( Double_t rot ) {
  //   fXax.SetXYZ( TMath::Cos(rot), 0.0, TMath::Sin(rot) );
  //   fYax.SetXYZ( 0.0, 1.0, 0.0 );
  //   fZax = fXax.Cross(fYax);
  // }
  const char* Here( const char* here ) { return here; }

  string      fName;    // Detector "name", actually the prefix without trailing dot
  string      fRealName;// Actual detector name (top level dropped)
  TString     fConfig;  // TString for compatibility with old API
  THaDetMap*  fDetMap;
  bool        fDetMapHasModel;
  Int_t       fNelem;
  Double_t    fSize[3];
  Float_t     fAngle;
  TVector3    fOrigin;//, fXax, fYax, fZax;
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
		     kShower, kTotalShower, kBPM, kRaster, kVDC, kVDCeff };
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

  virtual int ReadDB( FILE* infile, time_t date_from, time_t date_until );
  virtual int Save( time_t start, const string& version = string() ) const;
  virtual const char* GetClassName() const { return "Shower"; }

private:
  // Mapping
  vector<unsigned int> fChanMap;

  // Geometry, configuration
  Int_t      fNcols;
  Int_t      fNrows;
  Float_t    fXY[2], fDXY[2];
  Float_t    fEmin;

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

private:
  Double_t fFreq[2], fRPed[2], fSPed[2];
  Double_t fZpos[3];
  Double_t fCalibA[6], fCalibB[6], fCalibT[6];
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
  case kVDC:
    return 0; //TODO
  case kVDCeff:
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
    { "R.s0",       kScintillator },
    { "R.s1",       kScintillator },
    { "R.s2",       kScintillator },
    { "L.s0",       kScintillator },
    { "L.s1",       kScintillator },
    { "L.s2",       kScintillator },
    { "R.sh",       kShower },
    { "R.ps",       kShower },
    { "L.sh",       kShower },
    { "L.ps",       kShower },
    { "L.prl1",     kShower },
    { "L.prl2",     kShower },
    { "R.ts",       kTotalShower },
    { "L.ts",       kTotalShower },
    // { "R.vdc",      kVDC },
    // { "L.vdc",      kVDC },
    // { "vdceff",     kVDCeff },
    // Ignore top-level Beam apparatus db files - these are actually never read.
    // Instead, the apparatus's detector read db_urb.BPMA.dat etc.
    { "beam",       kNone },
    { "rb",         kNone },
    { "urb",        kNone },
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
	if( n_add > 0 )
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

  return 0;
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
  // If there are default files, select the one that the analyzer would pick
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
    if( defs.first->path.find("/DEFAULT/db_") != string::npos ) {
      filenames.erase( defs.second );
      deffile = defs.first->path;
    } else if( defs.second->path.find("/DEFAULT/db_") != string::npos ) {
      filenames.erase( defs.first );
      deffile = defs.second->path;
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
  if( !deffile.empty() ) {
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

  bool have_dated_subdirs = !subdirs.empty() &&
    (subdirs.size() > 1 || subdirs[0] != "DEFAULT");

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
      cerr << "WARNING: unknown detector name \"" << detname
	   << "\", corresponding files will not be converted" << endl;
      continue;
    }
    EDetectorType type = (*it).second;
    Detector* det = MakeDetector( type, detname );
    if( !det )
      continue;

    if( have_dated_subdirs && InsertDefaultFiles(subdirs, filenames) )
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
	  if( det->ReadDB(fi,date_from,date_until) )
	    cerr << "Error reading " << path << " as " << det->GetClassName()
		 << endl;
	  else {
	    cout << "Read " << path << endl;
	    //TODO: support variations
	    det->Save( date_from );
	  }
	}
      }

      // Save names of new-format database files to be copied
      if( do_file_copy && type == kCopyFile ) {

      }

    next:
      fclose(fi);
      det->Clear();
    }
    delete det;
  }

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

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  Int_t n = fscanf ( fi, "%5d", &nelem );   // Number of mirrors
  if( n != 1 ) return kInitError;
  if( nelem <= 0 ) {
    Error( Here(here), "Invalid number of mirrors = %d. Must be > 0.", nelem );
    return kInitError;
  }

  // Read detector map.  Assumes that the first half of the entries
  // is for ADCs, and the second half, for TDCs
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  fDetMap->Clear();
  fDetMapHasModel = false;
  while (1) {
    Int_t crate, slot, first, last, first_chan,model;
    int pos;
    fgets ( buf, LEN, fi );
    n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		&crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return kInitError;
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  fgets ( buf, LEN, fi );

  // Read geometry
  Float_t x,y,z;
  n = fscanf ( fi, "%15f %15f %15f", &x, &y, &z );        // Detector's X,Y,Z coord
  if( n != 3 ) return kInitError;
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  n = fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 );   // Sizes of det on X,Y,Z
  if( n != 3 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  n = fscanf ( fi, "%15f", &fAngle );                    // Rotation angle of det
  if( n != 1 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  // Calibration data
  if( nelem != fNelem ) {
    DeleteArrays();
    fNelem = nelem;
    fOff = new Float_t[ fNelem ];
    fPed = new Float_t[ fNelem ];
    fGain = new Float_t[ fNelem ];
  }

  // Read calibrations
  for (i=0;i<fNelem;i++) {
    n = fscanf( fi, "%15f", fOff+i );               // TDC offsets
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) {
    n = fscanf( fi, "%15f", fPed+i );               // ADC pedestals
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) {
    n = fscanf( fi, "%15f", fGain+i);               // ADC gains
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );

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

  fDetMapHasModel = fHaveExtras = false;

  // Read data from database
  while( ReadComment(fi, buf, LEN) );
  Int_t n = fscanf ( fi, "%5d", &nelem );                  // Number of  paddles
  fgets ( buf, LEN, fi );
  if( n != 1 ) return kInitError;
  if( nelem <= 0 ) {
    Error( Here(here), "Invalid number of paddles = %d. Must be > 0.", nelem );
    return kInitError;
  }

  // Read detector map. Unless a model-number is given
  // for the detector type, this assumes that the first half of the entries
  // are for ADCs and the second half, for TDCs.
  while( ReadComment(fi, buf, LEN) );
  int i = 0;
  fDetMap->Clear();
  while (1) {
    int pos;
    Int_t first_chan, model;
    Int_t crate, slot, first, last;
    fgets ( buf, LEN, fi );
    n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		&crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return kInitError;
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( model != 0 )
      fDetMapHasModel = true;

    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  while( ReadComment(fi, buf, LEN) );

  // Read geometry
  Float_t x,y,z;
  n = fscanf ( fi, "%15f %15f %15f", &x, &y, &z );   // Detector's X,Y,Z coord
  if( n != 3 ) return kInitError;
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  while( ReadComment(fi, buf, LEN) );
  n = fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 ); // Sizes of det on X,Y,Z
  if( n != 3 ) return kInitError;
  fgets ( buf, LEN, fi );
  while( ReadComment(fi, buf, LEN) );

  n = fscanf ( fi, "%15f", &fAngle );               // Rotation angle of detector
  if( n != 1 ) return kInitError;
  fgets ( buf, LEN, fi );

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

    fNTWalkPar = 2*fNelem; // 1 paramter per paddle
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

  while( ReadComment(fi, buf, LEN) );
  // Read calibration data
  for (i=0;i<fNelem;i++) {
    n = fscanf(fi,"%15lf",fLOff+i);                // Left Pads TDC offsets
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );   // finish line
  while( ReadComment(fi, buf, LEN) );
  for (i=0;i<fNelem;i++) {
    n = fscanf(fi,"%15lf",fROff+i);                // Right Pads TDC offsets
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );   // finish line
  while( ReadComment(fi, buf, LEN) );
  for (i=0;i<fNelem;i++) {
    n = fscanf(fi,"%15lf",fLPed+i);                // Left Pads ADC Pedest-s
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );   // finish line, etc.
  while( ReadComment(fi, buf, LEN) );
  for (i=0;i<fNelem;i++) {
    n = fscanf(fi,"%15lf",fRPed+i);                // Right Pads ADC Pedes-s
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );
  while( ReadComment(fi, buf, LEN) );
  for (i=0;i<fNelem;i++) {
    n = fscanf (fi,"%15lf",fLGain+i);              // Left Pads ADC Coeff-s
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );
  while( ReadComment(fi, buf, LEN) );
  for (i=0;i<fNelem;i++) {
    n = fscanf (fi,"%15lf",fRGain+i);              // Right Pads ADC Coeff-s
    if( n != 1 ) return kInitError;
  }
  fgets ( buf, LEN, fi );

  while( ReadComment(fi, buf, LEN) );

  // Here on down, look ahead line-by-line
  // stop reading if a '[' is found on a line (corresponding to the next date-tag)
  // read in TDC resolution (s/channel)
  if ( ! fgets(buf, LEN, fi) || strchr(buf,'[') ) goto exit;
  fHaveExtras = true;
  n = sscanf(buf,"%15lf",&fTdc2T);
  if( n != 1 ) return kInitError;
  fResolution = 3.*fTdc2T;      // guess at timing resolution

  while( ReadComment(fi, buf, LEN) );
  // Speed of light in the scintillator material
  if ( !fgets(buf, LEN, fi) ||  strchr(buf,'[') ) goto exit;
  n = sscanf(buf,"%15lf",&fCn);
  if( n != 1 ) return kInitError;

  // Attenuation length (inverse meters)
  while( ReadComment(fi, buf, LEN) );
  if ( !fgets ( buf, LEN, fi ) ||  strchr(buf,'[') ) goto exit;
  n = sscanf(buf,"%15lf",&fAttenuation);
  if( n != 1 ) return kInitError;

  while( ReadComment(fi, buf, LEN) );
  // Time-walk correction parameters
  if ( !fgets(buf, LEN, fi) ||  strchr(buf,'[') ) goto exit;
  n = sscanf(buf,"%15lf",&fAdcMIP);
  if( n != 1 ) return kInitError;

  while( ReadComment(fi, buf, LEN) );
  // timewalk parameters
  {
    int cnt=0;
    while ( cnt<fNTWalkPar && fgets( buf, LEN, fi ) && ! strchr(buf,'[') ) {
      char *ptr = buf;
      int pos=0;
      while ( cnt < fNTWalkPar && sscanf(ptr,"%15lf%n",&fTWalkPar[cnt],&pos)>0 ) {
	ptr += pos;
	cnt++;
      }
    }
  }

  while( ReadComment(fi, buf, LEN) );
  // trigger timing offsets
  {
    int cnt=0;
    while ( cnt<fNelem && fgets( buf, LEN, fi ) && ! strchr(buf,'[') ) {
      char *ptr = buf;
      int pos=0;
      while ( cnt < fNelem && sscanf(ptr,"%15lf%n",&fTrigOff[cnt],&pos)>0 ) {
	ptr += pos;
	cnt++;
      }
    }
  }

 exit:
  return kOK;
}

//-----------------------------------------------------------------------------
int Shower::ReadDB( FILE* fi, time_t date, time_t /* date_until */ )
{
  // Read legacy shower database

  const char* const here = "Shower::ReadDB";
  const int LEN = 100;
  char buf[LEN];

  // Read data from database

  // Blocks, rows, max blocks per cluster
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  Int_t n = fscanf ( fi, "%5d %5d", &fNcols, &fNrows );
  if( n != 2 ) return kInitError;
  Int_t nelem = fNcols * fNrows;
  Int_t nclbl = TMath::Min( 3, fNrows ) * TMath::Min( 3, fNcols );
  if( fNrows <= 0 || fNcols <= 0 || nclbl <= 0 ) {
    Error( Here(here), "Illegal number of rows or columns: "
	   "%d %d", fNrows, fNcols );
    return kInitError;
  }

  // Read detector map
  fDetMap->Clear();
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  while (1) {
    Int_t crate, slot, first, last;
    n = fscanf ( fi,"%6d %6d %6d %6d", &crate, &slot, &first, &last );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( n < 4 ) return kInitError;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }

  // Set up the new channel map
  UShort_t mapsize = fDetMap->GetSize();
  if( mapsize == 0 ) {
    Error( Here(here), "No modules defined in detector map.");
    return kInitError;
  }

  fChanMap.clear();
  // Read channel map
  //
  // Loosen the formatting restrictions: remove from each line the portion
  // after a '#', and do the pattern matching to the remaining string
  fgets ( buf, LEN, fi );

  // get the line and end it at a '#' symbol
  *buf = '\0';
  char *ptr=buf;
  int nchar=0;
  bool trivial = true;
  for ( UShort_t i = 0; i < nelem; i++ ) {
    while ( !strpbrk(ptr,"0123456789") ) {
      fgets ( buf, LEN, fi );
      if( (ptr = strchr(buf,'#')) ) *ptr = '\0';
      ptr = buf;
      nchar=0;
    }
    unsigned int chan;
    sscanf (ptr, "%6u %n", &chan, &nchar );
    fChanMap.push_back(chan);
    if( trivial && fChanMap.size() > 1 && fChanMap[fChanMap.size()-1] != chan )
      trivial = false;
    ptr += nchar;
  }
  if( fChanMap.size() != static_cast<vector<unsigned int>::size_type>(nelem) ) {
    Error( Here(here), "Incorrect number of elements in mapping table = %d, "
	   "need %d. Fix database.", (int)fChanMap.size(), nelem );
    return kInitError;
  }
  if( trivial )
    fChanMap.clear();
  fgets ( buf, LEN, fi );

  // Read geometry
  Float_t x,y,z;
  n = fscanf ( fi, "%15f %15f %15f", &x, &y, &z );               // Detector's X,Y,Z coord
  if( n != 3 ) return kInitError;
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  n = fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 );  // Sizes of det in X,Y,Z
  if( n != 3 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  n = fscanf ( fi, "%15f", &angle );                       // Rotation angle of det
  if( n != 1 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  n = fscanf ( fi, "%15f %15f", fXY, fXY+1 );              // Block 1 center position
  if( n != 2 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  n = fscanf ( fi, "%15f %15f", fDXY, fDXY+1 );            // Block spacings in x and y
  if( n != 2 ) return kInitError;
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  n = fscanf ( fi, "%15f", &fEmin );                       // Emin thresh for center
  if( n != 1 ) return kInitError;
  fgets ( buf, LEN, fi );

  // Arrays for per-block geometry and calibration data
  if( nelem != fNelem ) {
    DeleteArrays();
    fNelem  = nelem;
    fPed    = new Float_t[ fNelem ];
    fGain   = new Float_t[ fNelem ];
  }

  // Read calibrations.
  // Before doing this, search for any date tags that follow, and start reading from
  // the best matching tag if any are found. If none found, but we have a configuration
  // string, search for it.
  TDatime datime(date);
  if( THaAnalysisObject::SeekDBdate( fi, datime ) == 0 && fConfig.Length() > 0 &&
      THaAnalysisObject::SeekDBconfig( fi, fConfig.Data() )) {}

  fgets ( buf, LEN, fi );
  // Crude protection against a missed date/config tag
  if( buf[0] == '[' ) fgets ( buf, LEN, fi );

  // Read ADC pedestals and gains (in order of logical channel number)
  for (int j=0; j<fNelem; j++) {
    n = fscanf (fi,"%15f",fPed+j);
    if( n != 1 ) return kInitError;
  }

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (int j=0; j<fNelem; j++) {
    n = fscanf (fi, "%15f",fGain+j);
    if( n != 1 ) return kInitError;
  }

  return kOK;
}

//-----------------------------------------------------------------------------
int TotalShower::ReadDB( FILE* fi, time_t, time_t )
{
  // Read legacy THaTotalShower database

  const int LEN = 100;
  char line[LEN];

  fgets ( line, LEN, fi ); fgets ( line, LEN, fi );
  int n = fscanf ( fi, "%15f %15f", fMaxDXY, fMaxDXY+1 );  // Max diff of shower centers
  if( n != 2 ) return kInitError;

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
    if( n < 1 ) return kInitError;
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
    Error( Here(here), "Invalid BPM detector map. Needs to define exactly 4 "
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
  if( n < 2 ) return kInitError;

  // dummy 1 is the z position of the bpm. Used below to set fOrigin.

  // calibration constant, historical
  fCalibRot = dummy2;

  // dummy3 and dummy4 are not used in this apparatus

  // Pedestals
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf",fPed,fPed+1,fPed+2,fPed+3);
  if( n != 4 ) return kInitError;

  // 2x2 transformation matrix and x/y offset
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	     fRot,fRot+1,fRot+2,fRot+3,&dummy5,&dummy6);
  if( n != 6 ) return kInitError;

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
    if( n < 1 ) return kInitError;
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
    Error( Here(here), "Invalid raster detector map. Needs to define exactly 4 "
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
  if( n != 7 ) return kInitError;

  // z positions of BPMA, BPMB, and target reference point (usually 0)
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos);
  if( n != 1 ) return kInitError;
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos+1);
  if( n != 1 ) return kInitError;
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf",fZpos+2);
  if( n != 1 ) return kInitError;

  // Find timestamp, if any, for the raster constants
  TDatime datime(date);
  THaAnalysisObject::SeekDBdate( fi, datime, true );

  // Raster constants: offx/y,amplx/y,slopex/y for BPMA, BPMB, target
  fgets( buf, LEN, fi);
  n = sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	     fCalibA,fCalibA+1,fCalibA+2,fCalibA+3,fCalibA+4,fCalibA+5);
  if( n != 6 ) return kInitError;

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 fCalibB,fCalibB+1,fCalibB+2,fCalibB+3,fCalibB+4,fCalibB+5);
  if( n != 6 ) return kInitError;

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 fCalibT,fCalibT+1,fCalibT+2,fCalibT+3,fCalibT+4,fCalibT+5);
  if( n != 6 ) return kInitError;

  return kOK;
}

//-----------------------------------------------------------------------------
int Detector::Save( time_t start, const string& version ) const
{
  string prefix = fName + ".";
  int flags = 1;
  if( fDetMapHasModel )  flags++;
  AddToMap( prefix+"detmap",   MakeValue(fDetMap,flags), start, version, 4+flags );
  AddToMap( prefix+"nelem",    MakeValue(&fNelem), start, version );
  AddToMap( prefix+"angle",    MakeValueUnless(0.F,&fAngle), start, version );
  AddToMap( prefix+"position", MakeValueUnless(0.,&fOrigin), start, version );
  AddToMap( prefix+"size",     MakeValueUnless(0.,fSize,3), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Cherenkov::Save( time_t start, const string& version ) const
{
  // Create database keys for current Cherenkov configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  AddToMap( prefix+"tdc.offsets",   MakeValueUnless(0.F,fOff,fNelem),  start, version );
  AddToMap( prefix+"adc.pedestals", MakeValueUnless(0.F,fPed,fNelem),  start, version );
  AddToMap( prefix+"adc.gains",     MakeValueUnless(1.F,fGain,fNelem), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Scintillator::Save( time_t start, const string& version ) const
{
  // Create database keys for current Scintillator configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  AddToMap( prefix+"L.off",    MakeValueUnless(0.,fLOff,fNelem),  start, version );
  AddToMap( prefix+"L.ped",    MakeValueUnless(0.,fLPed,fNelem),  start, version );
  AddToMap( prefix+"L.gain",   MakeValueUnless(1.,fLGain,fNelem), start, version );
  AddToMap( prefix+"R.off",    MakeValueUnless(0.,fROff,fNelem),  start, version );
  AddToMap( prefix+"R.ped",    MakeValueUnless(0.,fRPed,fNelem),  start, version );
  AddToMap( prefix+"R.gain",   MakeValueUnless(1.,fRGain,fNelem), start, version );

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
  AddToMap( prefix+"xy",        MakeValue(fXY,2),  start, version );
  AddToMap( prefix+"dxdy",      MakeValue(fDXY,2), start, version );
  AddToMap( prefix+"emin",      MakeValue(&fEmin),  start, version );

  AddToMap( prefix+"pedestals", MakeValueUnless(0.F,fPed,fNelem),  start, version );
  AddToMap( prefix+"gains",     MakeValueUnless(1.F,fGain,fNelem), start, version );

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
  AddToMap( prefix+"pedestals",  MakeValueUnless(0.,fPed,4), start, version );
  AddToMap( prefix+"rotmatrix",  MakeValueUnless(0.,fRot,4), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Raster::Save( time_t start, const string& version ) const
{
  // Create database keys for current TotalShower configuration data

  string prefix = fName + ".";

  Detector::Save( start, version );

  AddToMap( prefix+"zpos",        MakeValue(fZpos,3), start, version );
  AddToMap( prefix+"freqs",       MakeValue(fFreq,2), start, version );
  AddToMap( prefix+"rast_peds",   MakeValueUnless(0.,fRPed,2), start, version );
  AddToMap( prefix+"slope_peds",  MakeValueUnless(0.,fSPed,2), start, version );
  AddToMap( prefix+"raw2posA",    MakeValue(fCalibA,6), start, version );
  AddToMap( prefix+"raw2posB",    MakeValue(fCalibB,6), start, version );
  AddToMap( prefix+"raw2posT",    MakeValue(fCalibT,6), start, version );

  return 0;
}

//-----------------------------------------------------------------------------
int Detector::AddToMap( const string& key, const string& value, time_t start,
			const string& version, int max ) const
{
  // Add given key and value with given validity start time and optional
  // "version" (secondary index) to the in-memory database.
  // If value is empty, do nothing (for use with MakeValueUnless).

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





//--- THaCoincTime ----

  const int LEN = 200;
  char buf[LEN];

  FILE* fi = OpenFile( date );
  if( !fi ) {
    // look for more general coincidence-timing database
    fi = OpenFile( "CT", date );
  }
  if ( !fi )
    return kFileError;

  //  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fDetMap->Clear();

  int cnt = 0;

  fTdcRes[0] = fTdcRes[1] = 0.;
  fTdcOff[0] = fTdcOff[1] = 0.;

  while ( 1 ) {
    Int_t model;
    Float_t tres;   //  TDC resolution
    Float_t toff;   //  TDC offset (in seconds)
    char label[21]; // string label for TDC = Stop_by_Start
		    // Label is a space-holder to be used in the future

    Int_t crate, slot, first, last;

    while( ReadComment(fi, buf, LEN) );

    fgets ( buf, LEN, fi );

    int nread = sscanf( buf, "%6d %6d %6d %6d %15f %20s %15f",
			&crate, &slot, &first, &model, &tres, label, &toff );
    if ( crate < 0 ) break;
    if ( nread < 6 ) {
      Error( Here(here), "Invalid detector map! Need at least 6 columns." );
      fclose(fi);
      return kInitError;
    }
    last = first; // only one channel per entry (one ct measurement)
    // look for the label in our list of spectrometers
    int ind = -1;
    for (int i=0; i<2; i++) {
      // enforce logical channel number 0 == 2by1, etc.
      // matching between fTdcLabels and the detector map
      if ( fTdcLabels[i] == label ) {
	ind = i;
	break;
      }
    }
    if (ind <0) {
      TString listoflabels;
      for (int i=0; i<2; i++) {
	listoflabels += " " + fTdcLabels[i];
      }
      listoflabels += '\0';
      Error( Here(here), "Invalid detector map! The timing measurement %s does not"
	     " correspond\n to the spectrometers. Expected one of \n"
	     "%s",label, listoflabels.Data());
      fclose(fi);
      return kInitError;
    }

    if( fDetMap->AddModule( crate, slot, first, last, ind, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }

    if ( ind+(last-first) < 2 )
      for (int j=ind; j<=ind+(last-first); j++)  {
	fTdcRes[j] = tres;
	if (nread>6) fTdcOff[j] = toff;
      }
    else
      Warning( Here(here), "Too many entries. Expected 2 but found %d",
	       cnt+1);
    cnt+= (last-first+1);
  }

  fclose(fi);

  return kOK;
}

// --- end THaCoincTime ----


// ----- THaTriggerTime ----

  const int LEN = 200;
  char buf[LEN];

  // first is the list of channels to watch to determine the event type
  // This could just come from THaDecData, but for robustness we need
  // another copy.

  // Read data from database
  FILE* fi = OpenFile( date );
  // however, this is not unexpected since most of the time it is un-necessary
  if( !fi ) return kOK;

  while( ReadComment(fi, buf, LEN) );

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
  int trg;
  float toff;
  float ch2t=-0.5e-9;
  int crate,slot,chan;
  fTrgTypes.clear();
  fToffsets.clear();
  fTDCRes = -0.5e-9; // assume 1872 TDC's.

  while ( fgets(buf,LEN,fi) ) {
    int fnd = sscanf( buf,"%8d %16f %16f",&trg,&toff,&ch2t);
    if( fnd < 2 ) continue;
    if( trg == 0 ) {
      fGlOffset = toff;
      fTDCRes = ch2t;
    }
    else {
      fnd = sscanf( buf,"%8d %16f %8d %8d %8d",&trg,&toff,&crate,&slot,&chan);
      if( fnd != 5 ) {
	cerr << "Cannot parse line: " << buf << endl;
	continue;
      }
      fTrgTypes.push_back(trg);
      fToffsets.push_back(toff);
      fDetMap->AddModule(crate,slot,chan,chan,trg);
    }
  }
  fclose(fi);

  // now construct the appropriate arrays
  delete [] fTrgTimes;
  fNTrgType = fTrgTypes.size();
  fTrgTimes = new Double_t[fNTrgType];
//   for (unsigned int i=0; i<fTrgTypes.size(); i++) {
//     if (fTrgTypes[i]==0) continue;
//     fTrg_gl.push_back(gHaVars->Find(Form("%s.bit%d",fDecDataNm.Data(),
// 					 fTrgTypes[i])));
//   }

// ---- End TriggerTime ---
#endif
