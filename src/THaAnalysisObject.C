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
#include "THaString.h"
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

using namespace std;
typedef string::size_type ssiz_t;

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list, 
					     EType type, EMode mode,
					     const char* var_prefix ) const
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
				fPrefix, Here(ClassName()), var_prefix );
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

  fStatus = kInitError;

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Open the run database and call the reader. If database cannot be opened,
  // fail only if this object needs the run database
  //  FILE* fi = OpenFile( "run", date, Here("OpenFile()"));
  //  if( fi ) {

  // Call this object's actual database reader
  Int_t status = ReadRunDatabase(date);
  if( status && (status != kFileError || (fProperties & kNeedsRunDB) != 0))
    return fStatus;

  // Read the database for this object.
  // Don't bother if this object has not implemented its own database reader.

  // Note: requires ROOT >= 3.01 because of TClass::GetMethodAllAny()
  if( IsA()->GetMethodAllAny("ReadDatabase")
      != gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadDatabase") ) {

    // Call this object's actual database reader
    if( ReadDatabase(date) )
      return fStatus;
  } 
#ifdef WITH_DEBUG
  else if ( fDebug>0 ) {
    cout << "Info: Skipping database call for object " << GetName() 
	 << " since no ReadDatabase function defined.\n";
  }
#endif

  // Define this object's variables.
  if( DefineVariables(kDefine) )
    return fStatus;
  
  return fStatus = kOK;
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenFile( const char *name, const TDatime& date,
				   const char *here, const char *filemode, 
				   const int debug_flag )
{
  // Open database file and return a pointer to the C-style file descriptor.

  FILE* fi = NULL;
  vector<string> fnames( GetDBFileList(name, date, here) );
  if( !fnames.empty() ) {
    vector<string>::iterator it = fnames.begin();
    do {
#ifdef WITH_DEBUG
      if( debug_flag>0 )
      	cout << "<" << here << ">: Opening database file " << *it;
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
Int_t THaAnalysisObject::IsDBdate( const string& line, TDatime& date,
				   bool warn )
{
  // Check if 'line' contains a valid database time stamp. If so, 
  // parse the line, set 'date' to the extracted time stamp, and return 1.
  // Else return 0;
  // Time stamps must be in SQL format: [ yyyy-mm-dd hh:mi:ss ]

  ssiz_t lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size()-12 ) return 0;
  ssiz_t rbrk = line.find(']',lbrk);
  if( rbrk == string::npos || rbrk <= lbrk+11 ) return 0;
  Int_t yy, mm, dd, hh, mi, ss;
  if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%d-%d-%d %d:%d:%d",
	      &yy, &mm, &dd, &hh, &mi, &ss) != 6) {
    if( warn )
      ::Warning("THaAnalysisObject::IsDBdate()", 
		"Invalid date tag %s", line.c_str());
    return 0;
  }
  date.Set(yy, mm, dd, hh, mi, ss);
  return 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::IsDBtag( const string& line, const char* tag,
				  string& text )
{
  // Check if 'line' is a tag = value pair and whether the tag equals 'tag'. 
  // If tag = text, but tag not found, return -1. If tag found, 
  // parse the line, set 'text' to the text after the "=", and return +1.
  // Tags are NOT case sensitive.
  // 'text' is unchanged unless a valid tag is found.

  ssiz_t pos = line.find('=');
  if( pos == string::npos ) return 0;
  if( pos == 0 || pos == line.size()-1 ) return -1;
  ssiz_t pos1 = line.substr(0,pos).find_first_not_of(" \t");
  if( pos1 == string::npos ) return -1;
  ssiz_t pos2 = line.substr(0,pos).find_last_not_of(" \t");
  if( pos2 == string::npos ) return -1;
  // Ignore case of the tag
  THaString t1(line.substr(pos1,pos2-pos1+1));
  if( t1.CmpNoCase(tag) != 0 ) return -1;
  // Extract the text, discarding any whitespace at beginning and end
  string rhs = line.substr(pos+1);
  pos1 = rhs.find_first_not_of(" \t");
  pos2 = rhs.find_last_not_of(" \t");
  text = rhs.substr(pos1,pos2);
  return 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* tag, string& text )
{
  // Load a data value tagged with 'tag' from the database 'file'.
  // Lines before the first valid time stamp or starting with "#" are ignored.
  // If 'tag' is found, then the most recent value seen (based on time stamps
  // and position within the file) is returned in 'text'.
  // Values with time stamps later than 'date' are ignored.
  // This allows incremental organization of the database where
  // only changes are recorded with time stamps.
  // Return 0 if success, >0 if tag not found, <0 if file error.

  if( !file || !tag ) return 2;
  const int LEN = 256;
  char buf[LEN];
  TDatime tagdate(950101,0), prevdate(950101,0);

  errno = 0;
  rewind(file);

  bool found = false, ignore = true;
  while( fgets( buf, LEN, file) != NULL) {
    size_t len = strlen(buf);
    if( len<2 || buf[0] == '#' ) continue;
    if( buf[len-1] == '\n') buf[len-1] = 0; //delete trailing newline
    string line(buf);
    Int_t status;
    if( !ignore && (status = IsDBtag( line, tag, text )) != 0) {
      if( status > 0 ) {
	found = true;
	prevdate = tagdate;
	// ignore is not set to true here so that the _last_, not the first,
	// of multiple identical tags is evaluated.
      }
    } else if( IsDBdate( line, tagdate ) != 0 )
      ignore = ( tagdate>date || tagdate<prevdate );
  }
  if( errno ) {
    perror( "THaAnalysisObject::LoadDBvalue()" );
    return -1;
  }
  return found ? 0 : 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* tag, Double_t& value )
{
  // Locate tag in database, convert the text found to double-precision,
  // and return result in 'value'.
  // This is a convenience function.

  string text;
  Int_t err = LoadDBvalue( file, date, tag, text );
  if( err == 0 )
    value = atof(text.c_str());
  return err;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* tag, TString& text )
{
  // Locate tag in database, convert the text found to TString
  // and return result in 'text'.
  // This is a convenience function.

  string _text;
  Int_t err = LoadDBvalue( file, date, tag, _text );
  if( err == 0 )
    text = _text.c_str();
  return err;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDB( FILE* f, const TDatime& date,
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
      tag[0] = 0; strncat(tag,prefix,LEN-1); strncat(tag,item->name,LEN-np-1);
      if( LoadDBvalue( f, date, tag, *(item->var)) && item->fatal )
	return item->fatal;
    }
    item++;
  }
  return 0;
}

//_____________________________________________________________________________
static bool IsTag( const char* buf )
{
  // Return true if the string in 'buf' matches regexp "[ \t]*\[.*\].*",
  // i.e. it is a tag. Internal function.

  register const char* p = buf;
  while( isspace(*p)) p++;
  if( *p != '[' ) return false;
  while( *p && *p != ']' ) p++;
  return ( *p == ']' );
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::SeekDBdate( FILE* file, const TDatime& date,
				     bool end_on_tag )
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
  
  static const char* const here = "THaAnalysisObject::SeekDBdateTag";

  if( !file ) return 0;
  const int LEN = 256;
  char buf[LEN];
  TDatime tagdate(950101,0), prevdate(950101,0);
  const bool kNoWarn = false;

  errno = 0;
  long pos = ftell(file);
  if( pos == -1 ) {
    if( errno ) 
      perror(here);
    return 0;
  }
  long foundpos = -1;
  bool found = false, quit = false;
  while( !errno && !quit && fgets( buf, LEN, file)) {
    size_t len = strlen(buf);
    if( len<2 || buf[0] == '#' ) continue;
    if( buf[len-1] == '\n') buf[len-1] = 0; //delete trailing newline
    string line(buf);
    if( IsDBdate( line, tagdate, kNoWarn )
	&& tagdate<=date && tagdate>=prevdate ) {
      prevdate = tagdate;
      foundpos = ftell(file);
      found = true;
    } else if( end_on_tag && IsTag(buf))
      quit = true;
  }
  if( errno ) {
    perror(here);
    found = false;
  }
  fseek( file, (found ? foundpos: pos), SEEK_SET );
  return found;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::SeekDBconfig( FILE* file, const char* tag, 
				       const char* label,
				       bool end_on_tag )
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

  if( !file || !tag || !strlen(tag) ) return 0;
  string _label("[");
  if( label && strlen(label)>0 ) {
    _label.append(label);  _label.append("=");
  }
  ssiz_t llen = _label.size();

  bool found = false, quit = false;;
  const int LEN = 256;
  char buf[LEN];

  errno = 0;
  long pos = ftell(file);
  if( pos != -1 ) {
    while( !errno && !found && !quit && fgets( buf, LEN, file)) {
      size_t len = strlen(buf);
      if( len<2 || buf[0] == '#' ) continue;      //skip comments
      if( buf[len-1] == '\n') buf[len-1] = 0;     //delete trailing newline
      char* cbuf = ::Compress(buf);
      string line(cbuf); delete [] cbuf;
      ssiz_t lbrk = line.find(_label);
      if( lbrk != string::npos && lbrk < line.size()-llen ) {
	ssiz_t rbrk = line.find(']',lbrk+llen);
	if( rbrk == string::npos ) continue;
	if( line.substr(lbrk+llen,rbrk-lbrk-llen) == tag ) {
	  found = true;
	  break;
	}
      } else if( end_on_tag && IsTag(buf) )
	quit = true;
    }
  }
  if( errno ) {
    perror( "THaAnalysisObject::SeekDBconfig" );
    found = false;
  }
  if( !found && pos >= 0 )
    fseek( file, pos, SEEK_SET ); 
  return found;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::ReadRunDatabase( const TDatime& date )
{
  // Default run database reader. Reads one value, <prefix>.config, into 
  // fConfig.

  FILE* file = OpenRunDBFile( date );
  if( !file ) return kFileError;

  string name(fPrefix); name.append("config");
  LoadDBvalue( file, date, name.c_str(), fConfig );

  fclose(file);
  return kOK;
}

//_____________________________________________________________________________
THaAnalysisObject* THaAnalysisObject::FindModule( const char* name,
						  const char* classname,
						  TList* list )
{
  // Locate the object 'name' in the list 'list' and check if it inherits 
  // from both THaAnalysisObject and from 'classname' (if given), and whether 
  // it is properly initialized.
  // By default, classname = "THaApparatus" and list = gHaApps.
  // Return pointer to valid object, else return NULL and set fStatus.

  static const char* const here = "FindModule()";
  static const char* const anaobj = "THaAnalysisObject";

  EStatus save_status = fStatus;
  fStatus = kInitError;
  if( !name || strlen(name) == 0 ) {
    Error( Here(here), "No module name given." );
    return NULL;
  }
  if( !list ) {
    Error( Here(here), "No list of modules given." );
    return NULL;
  }
  TObject* obj = list->FindObject( name );
  if( !obj ) {
    Error( Here(here), "Module %s does not exist.", name );
    return NULL;
  }
  if( !obj->IsA()->InheritsFrom( anaobj )) {
    Error( Here(here), "Module %s (%s) is not a %s.",
	   obj->GetName(), obj->GetTitle(), anaobj );
    return NULL;
  }
  if( classname && strlen(classname) > 0 && strcmp(classname,anaobj) &&
      !obj->IsA()->InheritsFrom( classname )) {
    Error( Here(here), "Module %s (%s) is not a %s.",
	   obj->GetName(), obj->GetTitle(), classname );
    return NULL;
  }
  THaAnalysisObject* aobj = static_cast<THaAnalysisObject*>( obj );
  if( !aobj->IsOK() ) {
    Error( Here(here), "Module %s (%s) not initialized.",
	   obj->GetName(), obj->GetTitle() );
    return NULL;
  }
  fStatus = save_status;
  return aobj;
}

