//*-- Author :    Ole Hansen   15-Jun-01

//////////////////////////////////////////////////////////////////////////
//
// THaAnalysisObject
//
// Abstract base class for a detector or subdetector.
//
// Derived classes must define all internal variables, a constructor 
// that registers any internal variables of interest in the global 
// physics variable list, and a Decode() method that fills the variables
// based on the information in the THaEvData structure.
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TClass.h"
#include "TDatime.h"
#include "TROOT.h"
#include "TMath.h"
#include "TError.h"

#include <cstring>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <cstdlib>

ClassImp(THaAnalysisObject)

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject( const char* name, 
				  const char* description ) :
  TNamed(name,description), fPrefix(NULL), fStatus(kNotinit), fDebug(0),
  fIsInit(false), fIsSetup(false)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaAnalysisObject::~THaAnalysisObject()
{
  // Destructor. Deletes global variables defined by this object.

  delete [] fPrefix; fPrefix = 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list, 
					     EType type, EMode mode ) const
{
  // Add/delete variables defined in 'list' to/from the list of global 
  // variables, using prefix of the current apparatus.
  // Internal function that can be called during initialization.

  if( !gHaVars ) {
    TString action;
    if( mode == kDefine )
      action = "defined";
    else if( mode == kDelete )
      action = "deleted";
    Warning( Here("DefineVariables()"), 
	     "No global variable list found. No variables %s.", 
	     action.Data() );
    return kInitError;
  }

  if( mode == kDefine ) {
    if( type == kVarDef )
      gHaVars->DefineVariables( (const VarDef*)list, 
				fPrefix, Here(ClassName()) );
    else if( type == kRVarDef )
      gHaVars->DefineVariables( (const RVarDef*)list, this,
				fPrefix, Here(ClassName()) );
  }
  else if( mode == kDelete ) {
    if( type == kVarDef ) {
      const VarDef* item;
      const VarDef* theList = (const VarDef*)list;
      while( (item = theList++) && item->name ) {
	TString name(fPrefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
      }
    } else if( type == kRVarDef ) {
      const RVarDef* item;
      const RVarDef* theList = (const RVarDef*)list;
      while( (item = theList++) && item->name ) {
	TString name(fPrefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
      }
    }
  }

  return kOK;
}

//_____________________________________________________________________________
const char* THaAnalysisObject::Here( const char* here ) const
{
  // Return a string consisting of 'here' followed by fPrefix.
  // Used for generating diagnostic messages.
  // The return value points to an internal static buffer that
  // one should not try to delete ;)

  static char buf[256];
  strcpy( buf, "\"" );
  if(fPrefix != NULL) {
    strcat( buf, fPrefix );
    *(buf+strlen(buf)-1) = 0;   // Delete trailing dot of prefix
  }
  strcat( buf, "\"::" );
  strcat( buf, here );
  return buf;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaAnalysisObject::Init()
{
  // Initialize this object for current time. See Init(date) below.

  TDatime now;
  return Init(now);
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaAnalysisObject::Init( const TDatime& date )
{
  // Common Init function for THaAnalysisObjects.
  //
  // Check if this object or any base classes have defined a custom 
  // ReadDatabase() method. If so, open the database file called 
  // "db_<fPrefix>dat" and, if successful, call ReadDatabase().
  // 
  // This implementation will change once the real database is  available.

  Int_t status;
  fStatus = kNotinit;

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // If this object has implemented its own ReadRunDatabase(), open
  // the run database and call the reader.
  if( IsA()->GetMethodAllAny("ReadRunDatabase") != 
      gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadRunDatabase")
      ) {

    FILE* fi = OpenFile( "run", date, Here("OpenFile()") );
    if (!fi) {
      return fStatus = kInitError;
    }

    // Call this object's actual database reader
    status = ReadRunDatabase( fi, date );
    fclose(fi);
    if( status )
      return fStatus = kInitError;
  } 
#ifdef WITH_DEBUG
  else if ( fDebug>0 ) {
    cout << "Info: Not reading run database call for object " << GetName() 
	 << " since no ReadRunDatabase function defined.\n";
  }
#endif

  // Open the database file proper associated with this object's prefix, 
  // but don't bother if this object has not implemented its own database reader.

  // Note: requires ROOT >= 3.01 because of TClass::GetMethodAllAny()
  if( IsA()->GetMethodAllAny("ReadDatabase")
      != gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadDatabase") ) {

    FILE* fi = OpenFile( date );
    if (!fi) {
      return fStatus = kInitError;
    }

    // Call this object's actual database reader
    status = ReadDatabase( fi, date );
    fclose(fi);
    if( status )
      return fStatus = kInitError;
  } 
#ifdef WITH_DEBUG
  else if ( fDebug>0 ) {
    cout << "Info: Skipping database call for object " << GetName() 
	 << " since no ReadDatabase function defined.\n";
  }
#endif

  // Define this object's variables.
  status = DefineVariables( kDefine );
  if( status )
    return fStatus = kInitError;
  
  return fStatus = kOK;
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenFile( const char *name, const TDatime& date,
				   const char *here, const char *filemode, 
				   const int debug_flag )
{
  // Open database file and return a pointer to the C-style file descriptor.

  // FIXME: Try to write this in a system-independent way. Avoid direct Unix 
  // system calls, call ROOT's OS interface instead.

  FILE* fi = NULL;
  vector<string> fnames;

  fnames = GetDBFileList( name, date, here );
  if( !fnames.empty() ) {
    vector<string>::iterator it = fnames.begin();
    do {
#ifdef WITH_DEBUG
      if( debug_flag>0 ) {
	cout << "<" << here << ">: Opening database file " << *it;
      }
#endif
      // Open the database file
      fi = fopen( (*it).c_str(), filemode);
      
#ifdef WITH_DEBUG
      if( debug_flag>0 ) 
	if( !fi ) cout << " ... failed" << endl;
	else      cout << " ... ok" << endl;
#endif
    } while ( !fi && ++it != fnames.end() );
  }
  if( !fi )
    cerr<<"<"<<here<<">: Cannot open database file for prefix "<<name<<endl;

  return fi;
}

//_____________________________________________________________________________
vector<string> THaAnalysisObject::GetDBFileList( const char* name, 
						 const TDatime& date,
						 const char* here )
{
  // Return the database file searchlist as a vector of strings.
  // The file names are relative to the current directory.

  // FIXME: Try to write this in a system-independent way. Avoid direct Unix 
  // system calls, call ROOT's OS interface instead.

  char *dbdir = NULL;
  struct dirent* result;
  DIR* dir;
  size_t pos;
  vector<string> time_dirs, dnames, fnames;
  vector<string>::iterator it, thedir;
  string item, filename;
  Int_t item_date;
  const string defaultdir("DEFAULT");
  bool have_defaultdir = false, found = false;

  // Build search list of directories
  if( (dbdir = getenv("DB_DIR")))
    dnames.push_back( dbdir );
  dnames.push_back( "DB" );
  dnames.push_back( "db" );
  dnames.push_back( "." );

  // Try to open the database directories in the search list.
  // The first directory that can be opened is taken to be the database
  // directory. Subsequent directories are ignored.
  
  it = dnames.begin();
  while( !(dir = opendir( (*it).c_str() )) && (++it != dnames.end()) );

  // None of the directories can be opened?
  if( it == dnames.end() ) goto error;

  // Pointer to database directory string
  thedir = it;

  // In the database directory, get the names of all subdirectories matching 
  // a YYYYMMDD pattern.
  errno = 0;
  while( (result = readdir(dir)) ) {
    item = result->d_name;
    if( item.length() == 8 ) {
      for( pos=0; pos<8; ++pos )
	if( !isdigit(item[pos])) break;
      if( pos==8 )
	time_dirs.push_back( item );
    } else if ( item == defaultdir )
      have_defaultdir = true;
  }
  if( errno )  goto error;
  closedir(dir);

  // Search a date-coded subdirectory that corresponds to the requested date.
  if( time_dirs.size() > 0 ) {
    sort( time_dirs.begin(), time_dirs.end() );
    for( it = time_dirs.begin(); it != time_dirs.end(); it++ ) {
      item_date = atoi((*it).c_str());
      if( it == time_dirs.begin() && date.GetDate() < item_date )
	break;
      if( it != time_dirs.begin() && date.GetDate() < item_date ) {
	it--;
	found = true;
	break;
      }
      // Assume that the last directory is valid until infinity.
      if( it+1 == time_dirs.end() && date.GetDate() >= item_date ) {
	found = true;
	break;
      }
    }
  }

  // Construct the database file name. It is of the form db_<prefix>.dat.
  // Subdetectors use the same files as their parent detectors!
  filename = "db_";
  filename += name;
  // Make sure that "name" ends with a dot. If not, add one.
  if( filename[ filename.length()-1 ] != '.' )
    filename += '.';
  filename += "dat";

  // Build the searchlist of file names in the order:
  // <date-dir>/filename DEFAULT/filename ./filename

  if( found ) {
    item = *thedir + "/" + *it + "/" + filename;
    fnames.push_back( item );
  }
  if( have_defaultdir ) {
    item = *thedir + "/" + defaultdir + "/" + filename;
    fnames.push_back( item );
  }
  fnames.push_back( *thedir + "/" + filename );
  goto exit;

 error:
  perror(here);

 exit:
  return fnames;
}

//_____________________________________________________________________________
void THaAnalysisObject::MakePrefix( const char* basename )
{
  // Set up name prefix for global variables. 
  // Internal function called by constructors of derived classes.
  // If basename != NULL, 
  //   fPrefix = basename  + "." + GetName() + ".",
  // else 
  //   fPrefix = GetName() + "."

  delete [] fPrefix;
  if( basename && strlen(basename) > 0 ) {
    fPrefix = new char[ strlen(basename) + strlen(GetName()) + 3 ];
    strcpy( fPrefix, basename );
    strcat( fPrefix, "." );
    strcat( fPrefix, GetName() );
    strcat( fPrefix, "." );
  } else {
    fPrefix = new char[ strlen(GetName()) + 2 ];
    strcpy( fPrefix, GetName() );
    strcat( fPrefix, "." );
  }
}

//_____________________________________________________________________________
void THaAnalysisObject::SetName( const char* name )
{
  // Set/change the name of the object.

  if( !name || strlen(name) == 0 ) {
    Warning( Here("SetName()"),
	     "Cannot set an empty object name. Name not set.");
    return;
  }
  TNamed::SetName( name );
  MakePrefix();
}

//_____________________________________________________________________________
void THaAnalysisObject::SetNameTitle( const char* name, const char* title )
{
  // Set name and title of the object.

  SetName( name );
  SetTitle( title );
}

//_____________________________________________________________________________
void THaAnalysisObject::GeoToSph( Double_t  th_geo, Double_t  ph_geo,
				  Double_t& th_sph, Double_t& ph_sph )
{
  // Convert geographical to spherical angles. Units are rad.
  // th_geo and ph_geo can be anything.
  // th_sph is in [0,pi], ph_sph in [-pi,pi].

  static const Double_t twopi = 2.0*TMath::Pi();
  Double_t ct = cos(th_geo), cp = cos(ph_geo);
  register Double_t tmp = ct*cp;
  th_sph = acos( tmp );
  tmp = sqrt(1.0 - tmp*tmp);
  ph_sph = (fabs(tmp) < 1e-6 ) ? 0.0 : acos( sqrt(1.0-ct*ct)*cp/tmp );
  if( th_geo/twopi-floor(th_geo/twopi) > 0.5 ) ph_sph = TMath::Pi() - ph_sph;
  if( ph_geo/twopi-floor(ph_geo/twopi) > 0.5 ) ph_sph = -ph_sph;
}

//_____________________________________________________________________________
void THaAnalysisObject::SphToGeo( Double_t  th_sph, Double_t  ph_sph,
				  Double_t& th_geo, Double_t& ph_geo )
{
  // Convert spherical to geographical angles. Units are rad.
  // th_sph and ph_sph can be anything, although th_sph outside
  // [0,pi] is not really meaningful.
  // th_geo is in [-pi,pi] and ph_sph in [-pi/2,pi/2]

  static const Double_t twopi = 2.0*TMath::Pi();
  Double_t ct = cos(th_sph), st = sin(th_sph), cp = cos(ph_sph);
  if( fabs(ct) > 1e-6 ) {
    th_geo = atan( st/ct*cp );
    if( cp>0.0 && th_geo<0.0 )      th_geo += TMath::Pi();
    else if( cp<0.0 && th_geo>0.0 ) th_geo -= TMath::Pi();
  } else {
    th_geo = TMath::Pi()/2.0;
    if( cp<0.0 ) th_geo = -th_geo;
  }
  ph_geo = acos( sqrt( st*st*cp*cp + ct*ct ));
  if( ph_sph/twopi - floor(ph_sph/twopi) > 0.5 ) ph_geo =- ph_geo;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::IsRunDBdate( const string& line, TDatime& date )
{
  // Check if 'line' contains a valid run database time stamp. If so, 
  // parse the line, set 'date' to the extracted time stamp, and return 1.
  // Else return 0;
  // Time stamps must have the format [ yyyy-mm-dd:hh:mm:ss ].

  string::size_type lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size()-12 ) return 0;
  string::size_type rbrk = line.find(']',lbrk);
  if( rbrk <= lbrk+11 ) return 0;
  Int_t yy, mm, dd, hh, mi, ss;
  if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%d-%d-%d:%d:%d:%d",
	      &yy, &mm, &dd, &hh, &mi, &ss) != 6) {
    ::Warning("THaAnalysisObject::IsRunDBdate()", 
	      "Invalid date tag %s", line.c_str());
    return 0;
  }
  date.Set(yy, mm, dd, hh, mi, ss);
  return 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::IsRunDBtag( const string& line, const char* tag,
				     Double_t& value )
{
  // Check if 'line' is a tag = value pair and whether the tag equals 'tag'. 
  // If tag = value, but tag not found, return -1. If tag found, 
  // parse the line, set 'value' to the extracted value, and return +1.
  // Tags are not case sensitive.

  string::size_type pos = line.find('=');
  if( pos == string::npos ) return 0;
  if( pos == 0 || pos == line.size()-1 ) return -1;
  string::size_type pos1 = line.substr(0,pos).find_first_not_of(" \t");
  if( pos1 == string::npos ) return -1;
  string::size_type pos2 = line.substr(0,pos).find_last_not_of(" \t");
  if( pos2 == string::npos ) return -1;
  // Ignore case
  string t1(line.substr(pos1,pos2-pos1+1)), t2(tag);
  transform( t1.begin(), t1.end(), t1.begin(), tolower );
  transform( t2.begin(), t2.end(), t2.begin(), tolower );
  if( t1 != t2 ) return -1;
  value = std::atof( line.substr(pos+1).c_str() );
  return 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadRunDBvalue( FILE* file, const TDatime& date, 
					 const char* tag, Double_t& value )
{
  // Load a data value tagged with 'tag' from the run database 'file'.
  // Lines before the first valid time stamp or starting with "#" are ignored.
  // If 'tag' is found, then the most recent value seen (based on time stamps
  // and position within the file) is returned.
  // Values with time stamps later than 'date' are ignored.
  // This allows incremental organization of the run database where
  // only changes are recorded with time stamps.
  // Values are always treated as doubles.
  // Return 0 if success, >0 if tag not found, <0 if file error.

  if( !file || !tag ) return 2;
  const int LEN = 256;
  char buf[LEN];
  string line;
  TDatime tagdate(950101,0), prevdate(950101,0);

  errno = 0;
  rewind(file);

  bool found = false, ignore = true;
  while( fgets( buf, LEN, file) != NULL) {
    size_t len = strlen(buf);
    if( len>0 && buf[len-1] == '\n') buf[len-1] = 0; //delete trailing newline
    if( len<2 || buf[0] == '#' ) continue;
    line = buf;
    Int_t status;
    if( !ignore && (status = IsRunDBtag( line, tag, value )) != 0) {
      if( status > 0 ) {
	found = true;
	prevdate = tagdate;
	// ignore is not set to true here so that the _last_, not the first,
	// of multiple identical tags is evaluated.
      }
    } else if( IsRunDBdate( line, tagdate ) != 0 )
      ignore = ( tagdate>date || tagdate<prevdate );
  }
  if( errno ) {
    perror( "THaAnalysisObject::LoadRunDBvalue()" );
    return -1;
  }
  return found ? 0 : 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadRunDB( FILE* f, const TDatime& date,
				    const TagDef* tags, const char* prefix )
{
  // Load a list of parameters from the run database according to 
  // the contents of the 'tags' structure (see VarDef.h).

  if( !tags ) return -1;
  const Int_t LEN = 256;
  Int_t np = strlen(prefix);
  if( np > LEN-2 ) return -2;
  char tag[LEN];

  const TagDef* item = tags;
  while( item->name ) {
    if( item->var ) {
      tag[0] = 0;
      strncat(tag,prefix,LEN-1);
      strncat(tag,item->name,LEN-np-1);
      if( LoadRunDBvalue( f, date, tag, *(item->var)) && item->fatal )
	return item->fatal;
    }
    item++;
  }
  return 0;
}
