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
#include "THaTextvars.h"
#include "THaGlobals.h"
#include "TClass.h"
#include "TDatime.h"
#include "TROOT.h"
#include "TMath.h"
#include "TError.h"
#include "TVector3.h"
#include "TSystem.h"
#include "TObjArray.h"
#include "TVirtualMutex.h"
#include "TThread.h"
#include "Varargs.h"

#include <cstring>
#include <cctype>
#include <errno.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <map>
#include <limits>
#include <iomanip>

using namespace std;
typedef string::size_type ssiz_t;
typedef vector<string>::iterator vsiter_t;

TList* THaAnalysisObject::fgModules = NULL;

const Double_t THaAnalysisObject::kBig = 1.e38;

// Mutex for concurrent access to global Here function
static TVirtualMutex* gHereMutex = 0;

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject( const char* name, 
				      const char* description ) :
  TNamed(name,description), fPrefix(NULL), fStatus(kNotinit), 
  fDebug(0), fIsInit(false), fIsSetup(false), fProperties(0),
  fOKOut(false), fInitDate(19950101,0), fNEventsWithWarnings(0),
  fExtra(0)
{
  // Constructor

  if( !fgModules ) fgModules = new TList;
  fgModules->Add( this );
}

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject()
  : fPrefix(NULL), fStatus(kNotinit), fDebug(0), fIsInit(false),
    fIsSetup(false), fProperties(), fOKOut(false), fNEventsWithWarnings(0),
    fExtra(0)
{
  // only for ROOT I/O
}

//_____________________________________________________________________________
THaAnalysisObject::~THaAnalysisObject()
{
  // Destructor

  delete fExtra; fExtra = 0;

  if (fgModules) {
    fgModules->Remove( this );
    if( fgModules->GetSize() == 0 ) {
      delete fgModules;
      fgModules = 0;
    }
  }
  delete [] fPrefix; fPrefix = 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::Begin( THaRunBase* /* run */ )
{
  // Method called right before the start of the event loop
  // for 'run'. Begin() is similar to Init(), but since there is a default
  // Init() implementing standard database and global variable setup,
  // Begin() can be used to implement start-of-run setup tasks for each
  // module cleanly without interfering with the standard Init() prodecure.
  //
  // The default Begin() method clears the history of warning messages
  // and warning message event counter

  fMessages.clear();
  fNEventsWithWarnings = 0;

  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::End( THaRunBase* /* run */ )
{
  // Method called right after the end of the event loop for 'run'.
  // May be used by modules to clean up, compute averages, write summaries, etc.
  //
  // The default End() method prints warning message summary

  if( !fMessages.empty() ) {
    ULong64_t ntot = 0;
    for( map<string,UInt_t>::const_iterator it = fMessages.begin();
	 it != fMessages.end(); ++it ) {
      ntot += it->second;
    }
    ostringstream msg;
    msg << endl
	<< "  Encountered " << fNEventsWithWarnings << " events with "
	<< "warnings, " << ntot << " total warnings"
	<< endl
	<< "  Call Print(\"WARN\") for channel list. "
	<< "Re-run with fDebug>0 for per-event details.";
    Warning( Here("End"), "%s", msg.str().c_str() );
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVariables( EMode /* mode */ )
{ 
  // Default method for defining global variables. Currently does nothing.

  return kOK;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const VarDef* list, EMode mode,
					     const char* var_prefix ) const
{ 
  return DefineVarsFromList( list, kVarDef, mode, var_prefix );
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const RVarDef* list, EMode mode,
					     const char* var_prefix ) const
{ 
  return DefineVarsFromList( list, kRVarDef, mode, var_prefix );
}
 
//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list, 
					     EType type, EMode mode,
					     const char* var_prefix ) const
{
  // Add/delete variables defined in 'list' to/from the list of global 
  // variables, using prefix of the current apparatus.
  // Internal function that can be called during initialization.

  TString here(GetClassName());
  here.Append("::DefineVarsFromList");
  return DefineVarsFromList( list, type, mode, var_prefix, this, 
			     fPrefix, here.Data() );
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list, 
					     EType type, EMode mode,
					     const char* var_prefix,
					     const TObject* obj,
					     const char* prefix,
					     const char* here )
{
  // Actual implementation of the variable definition utility function.
  // Static function that can be used by classes other than THaAnalysisObjects

  if( !gHaVars ) {
    TString action;
    if( mode == kDefine )
      action = "defined";
    else if( mode == kDelete )
      action = "deleted (this is safe when exiting)";
    ::Warning( ::Here(here,prefix), "No global variable list found. "
	       "No variables %s.",  action.Data() );
    return (mode==kDefine ? kInitError : kOK);
  }

  if( mode == kDefine ) {
    if( type == kVarDef )
      gHaVars->DefineVariables( static_cast<const VarDef*>(list),
				prefix, ::Here(here,prefix) );
    else if( type == kRVarDef )
      gHaVars->DefineVariables( static_cast<const RVarDef*>(list), obj,
				prefix, ::Here(here,prefix), var_prefix );
  }
  else if( mode == kDelete ) {
    if( type == kVarDef ) {
      const VarDef* item;
      const VarDef* theList = static_cast<const VarDef*>(list);
      while( (item = theList++) && item->name ) {
	TString name(prefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
      }
    } else if( type == kRVarDef ) {
      const RVarDef* item;
      const RVarDef* theList = static_cast<const RVarDef*>(list);
      while( (item = theList++) && item->name ) {
	TString name(prefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
      }
    }
  }

  return kOK;
}

//_____________________________________________________________________________
THaAnalysisObject* THaAnalysisObject::FindModule( const char* name,
						  const char* classname,
						  bool do_error )
{
  // Locate the object 'name' in the global list of Analysis Modules 
  // and check if it inherits from 'classname' (if given), and whether 
  // it is properly initialized.
  // Return pointer to valid object, else return NULL.
  // If do_error == true (default), also print error message and set fStatus
  // to kInitError.
  // If do_error == false, don't print error messages and not test if object is
  // initialized.
  //
  // This function is intended to be called from physics module initialization
  // routines.

  static const char* const here = "FindModule";
  static const char* const anaobj = "THaAnalysisObject";

  if( !name || !*name ) {
    if( do_error )
      Error( Here(here), "No module name given." );
    fStatus = kInitError;
    return NULL;
  }

  // Find the module in the list, comparing 'name' to the module's fPrefix
  TIter next(fgModules);
  TObject* obj = 0;
  while( (obj = next()) ) {
#ifdef NDEBUG
    THaAnalysisObject* module = static_cast<THaAnalysisObject*>(obj);
#else
    THaAnalysisObject* module = dynamic_cast<THaAnalysisObject*>(obj);
    assert(module);
#endif
    const char* cprefix = module->GetPrefix();
    if( !cprefix ) {
      module->MakePrefix();
      cprefix = module->GetPrefix();
      if( !cprefix )
	continue;
    }
    TString prefix(cprefix);
    if( prefix.EndsWith(".") )
      prefix.Chop();
    if( prefix == name )
      break;
  }
  if( !obj ) {
    if( do_error )
      Error( Here(here), "Module %s does not exist.", name );
    fStatus = kInitError;
    return NULL;
  }

  // Type check (similar to dynamic_cast, except resolving the class name as
  // a string at run time
  if( !obj->IsA()->InheritsFrom( anaobj )) {
    if( do_error )
      Error( Here(here), "Module %s (%s) is not a %s.",
	     obj->GetName(), obj->GetTitle(), anaobj );
    fStatus = kInitError;
    return NULL;
  }
  if( classname && *classname && strcmp(classname,anaobj) &&
      !obj->IsA()->InheritsFrom( classname )) {
    if( do_error )
      Error( Here(here), "Module %s (%s) is not a %s.",
	     obj->GetName(), obj->GetTitle(), classname );
    fStatus = kInitError;
    return NULL;
  }
  THaAnalysisObject* aobj = static_cast<THaAnalysisObject*>( obj );
  if( do_error ) {
    if( !aobj->IsOK() ) {
      Error( Here(here), "Module %s (%s) not initialized.",
	     obj->GetName(), obj->GetTitle() );
      fStatus = kInitError;
      return NULL;
    }
  }
  return aobj;
}

//_____________________________________________________________________________
vector<string> THaAnalysisObject::GetDBFileList( const char* name, 
						 const TDatime& date,
						 const char* here )
{
  // Return the database file searchlist as a vector of strings.
  // The file names are relative to the current directory.

  static const string defaultdir = "DEFAULT";
#ifdef WIN32
  static const string dirsep = "\\", allsep = "/\\";
#else
  static const string dirsep = "/", allsep = "/";
#endif

  const char* dbdir = NULL;
  const char* result;
  void* dirp;
  size_t pos;
  vector<string> time_dirs, dnames, fnames;
  vsiter_t it;
  string item, filename, thedir;
  Int_t item_date;
  bool have_defaultdir = false, found = false;

  if( !name || !*name )
    goto exit;

  // If name contains a directory separator, we take the name verbatim
  filename = name;
  if( filename.find_first_of(allsep) != string::npos ) {
    fnames.push_back( filename );
    goto exit;
  }

  // Build search list of directories
  if( (dbdir = gSystem->Getenv("DB_DIR")))
    dnames.push_back( dbdir );
  dnames.push_back( "DB" );
  dnames.push_back( "db" );
  dnames.push_back( "." );

  // Try to open the database directories in the search list.
  // The first directory that can be opened is taken as the database
  // directory. Subsequent directories are ignored.
  it = dnames.begin();
  while( !(dirp = gSystem->OpenDirectory( (*it).c_str() )) && 
	 (++it != dnames.end()) ) {}

  // None of the directories can be opened?
  if( it == dnames.end() ) {
    ::Error( here, "Cannot open any database directories. Check your disk!");
    goto exit;
  }

  // Pointer to database directory string
  thedir = *it;

  // In the database directory, get the names of all subdirectories matching 
  // a YYYYMMDD pattern.
  while( (result = gSystem->GetDirEntry(dirp)) ) {
    item = result;
    if( item.length() == 8 ) {
      for( pos=0; pos<8; ++pos )
	if( !isdigit(item[pos])) break;
      if( pos==8 )
	time_dirs.push_back( item );
    } else if ( item == defaultdir )
      have_defaultdir = true;
  }
  gSystem->FreeDirectory(dirp);

  // Search a date-coded subdirectory that corresponds to the requested date.
  if( time_dirs.size() > 0 ) {
    sort( time_dirs.begin(), time_dirs.end() );
    for( it = time_dirs.begin(); it != time_dirs.end(); ++it ) {
      item_date = atoi((*it).c_str());
      if( it == time_dirs.begin() && date.GetDate() < item_date )
	break;
      if( it != time_dirs.begin() && date.GetDate() < item_date ) {
	--it;
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
  // If filename does not start with "db_", make it so
  if( filename.substr(0,3) != "db_" )
    filename.insert(0,"db_");
  // If filename does not end with ".dat", make it so
#ifndef NDEBUG
  // should never happen
  assert( filename.length() >= 4 );
#else
  if( filename.length() < 4 ) { fnames.clear(); goto exit; }
#endif
  if( *filename.rbegin() == '.' ) {
    filename += "dat";
  } else if( filename.substr(filename.length()-4) != ".dat" ) {
    filename += ".dat";
  }

  // Build the searchlist of file names in the order:
  // ./filename <dbdir>/<date-dir>/filename 
  //    <dbdir>/DEFAULT/filename <dbdir>/filename
  fnames.push_back( filename );
  if( found ) {
    item = thedir + dirsep + *it + dirsep + filename;
    fnames.push_back( item );
  }
  if( have_defaultdir ) {
    item = thedir + dirsep + defaultdir + dirsep + filename;
    fnames.push_back( item );
  }
  fnames.push_back( thedir + dirsep + filename );

 exit:
  return fnames;
}

//_____________________________________________________________________________
const char* THaAnalysisObject::GetDBFileName() const
{
  return GetPrefix();
}

//_____________________________________________________________________________
const char* THaAnalysisObject::GetClassName() const
{
  const char* classname = "UnknownClass";
  if( TROOT::Initialized() )
    classname = ClassName();
  return classname;
}

//_____________________________________________________________________________
void THaAnalysisObject::DoError( int level, const char* here,
				 const char* fmt, va_list va) const
{
  // Interface to ErrorHandler. Inserts this object's name after the class name.
  // If 'here' = ("prefix")::method -> print <Class("prefix")::method>
  // If 'here' = method             -> print <Class::method>

  TString location(here);
  if( !location.BeginsWith("(\"") )
    location.Prepend("::");

  location.Prepend(GetClassName());

  ::ErrorHandler(level, location.Data(), fmt, va);
}

//_____________________________________________________________________________
const char* Here( const char* method, const char* prefix )
{
  // Utility function for error messages. The return value points to a static
  // string buffer that is unique to the current thread.
  // There are two usage cases:
  // ::Here("method","prefix")        -> returns ("prefix")::method
  // ::Here("Class::method","prefix") -> returns Class("prefix")::method

  // One static string buffer per thread ID
  static map<Long_t,TString> buffers;

  TString txt;
  if( prefix && *prefix ) {
    TString full_prefix(prefix);
    // delete trailing dot of prefix, if any
    if( full_prefix.EndsWith(".") )
      full_prefix.Chop();
    full_prefix.Prepend("(\""); full_prefix.Append("\")");
    const char* scope;
    if( method && *method && (scope = strstr(method, "::")) ) {
      Ssiz_t pos = scope - method;
      txt = method;
      assert(pos >= 0 && pos < txt.Length());
      txt.Insert(pos, full_prefix);
      method = 0;
    } else {
      txt = full_prefix + "::";
    }
  }
  if( method )
    txt.Append(method);

  R__LOCKGUARD2(gHereMutex);

  TString& ret = (buffers[ TThread::SelfId() ] = txt);

  return ret.Data(); // pointer to the C-string of a std::string in static map
}

//_____________________________________________________________________________
const char* THaAnalysisObject::Here( const char* here ) const
{
  // Return a string consisting of ("fPrefix")::here
  // Used for generating diagnostic messages.
  // The return value points to an internal static buffer that
  // one should not try to delete ;)
  
  return ::Here( here, fPrefix );
}

//_____________________________________________________________________________
const char* THaAnalysisObject::ClassNameHere( const char* here ) const
{
  // Return a string consisting of Class("fPrefix")::here

  TString method(here);
  if( method.Index("::") == kNPOS ) {
    method.Prepend("::");
    method.Prepend(GetClassName());
  }
  return ::Here( method.Data(), fPrefix );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaAnalysisObject::Init()
{
  // Initialize this object for current time. See Init(date) below.

  return Init( TDatime() );
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

  static const char* const here = "Init";

  if( IsZombie() )
    return fStatus = kNotinit;

  fInitDate = date;
  
  const char* fnam = "run.";

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Open the run database and call the reader. If database cannot be opened,
  // fail only if this object needs the run database
  // Call this object's actual database reader
  Int_t status = ReadRunDatabase(date);
  if( status && (status != kFileError || (fProperties & kNeedsRunDB) != 0))
    goto err;

  // Read the database for this object.
  // Don't bother if this object has not implemented its own database reader.

  // Note: requires ROOT >= 3.01 because of TClass::GetMethodAllAny()
  if( IsA()->GetMethodAllAny("ReadDatabase") != 
      gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadDatabase") ) {

    // Call this object's actual database reader
    fnam = GetDBFileName();
    try {
      status = ReadDatabase(date);
    }
    catch( const std::bad_alloc& ) {
      Error( Here(here), "Out of memory in ReadDatabase. Machine too busy? "
	     "Call expert." );
      status = kInitError;
    }
    catch( const std::exception& e ) {
      Error( Here(here), "Exception %s caught in ReadDatabase. "
	     "Module not initialized. Check database or call expert.",
	     e.what() );
      status = kInitError;
    }
    if( status )
      goto err;
  } 
  else if ( fDebug>0 ) {
    Info( Here(here), "No ReadDatabase function defined. Database not read." );
  }

  // Define this object's variables.
  status = DefineVariables(kDefine);

  Clear("I");
  goto exit;

 err:
  if( status == kFileError )
    Error( Here(here), "Cannot open database file db_%sdat", fnam );
  else if( status == kInitError )
    Error( Here(here), "Error when reading file db_%sdat", fnam);
 exit:
  return fStatus = (EStatus)status;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::InitOutput( THaOutput* /* output */ )
{
  // This method is called from THaAnalyzer::DoInit,
  // after THaOutput is initialized.
  // The TTree to work with can be retrieved like:
  // TTree *tree = output->GetTree()
  //
  // tree is the TTree to append the branches to
  //
  // construct all branches here. Set kOKOut=true if
  // all is okay, and return 0
  //
  // anything else will trigger error messages.
  fOKOut = true;
  return kOK;
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
  if( basename && *basename ) {
    fPrefix = new char[ strlen(basename) + strlen(GetName()) + 3 ];
    strcpy( fPrefix, basename );
    strcat( fPrefix, "." );
  } else {
    fPrefix = new char[ strlen(GetName()) + 2 ];
    *fPrefix = 0;
  }
  strcat( fPrefix, GetName() );
  strcat( fPrefix, "." );
}

//_____________________________________________________________________________
void THaAnalysisObject::MakePrefix()
{
  // Make default prefix: GetName() + "."

  MakePrefix(0);
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenFile( const char *name, const TDatime& date,
				   const char *here, const char *filemode, 
				   const int debug_flag )
{
  // Open database file and return a pointer to the C-style file descriptor.

  // Ensure input is sane
  if( !name || !*name )
    return NULL;
  if( !here )
    here="";
  if( !filemode )
    filemode="r";

  // Get list of database file candidates and try to open them in turn
  FILE* fi = NULL;
  vector<string> fnames( GetDBFileList(name, date, here) );
  if( !fnames.empty() ) {
    vsiter_t it = fnames.begin();
    do {
      if( debug_flag>1 )
	cout << "Info in <" << here << ">: Opening database file " << *it;
      // Open the database file
      fi = fopen( (*it).c_str(), filemode);
      
      if( debug_flag>1 ) 
	if( !fi ) cout << " ... failed" << endl;
	else      cout << " ... ok" << endl;
      else if( debug_flag>0 && fi )
	cout << "<" << here << ">: Opened database file " << *it << endl;
      // continue until we succeed
    } while ( !fi && ++it != fnames.end() );
  }
  if( !fi && debug_flag>0 ) {
    ::Error(here,"Cannot open database file db_%s%sdat",name,
	    (name[strlen(name)-1]=='.'?"":"."));
  }

  return fi;
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenFile( const TDatime& date )
{ 
  // Default method for opening database file

  return OpenFile(GetDBFileName(), date, ClassNameHere("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenRunDBFile( const TDatime& date )
{ 
  // Default method for opening run database file

  return OpenFile("run", date, ClassNameHere("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
char* THaAnalysisObject::ReadComment( FILE* fp, char *buf, const int len )
{
  // Read database comment lines (those not starting with a space (' ')),
  // returning the comment.
  // If the line is data, then nothing is done and NULL is returned,
  // so one can search for the next data line with:
  //   while ( ReadComment(fp, buf, len) );
  int ch = fgetc(fp);  // peak ahead one character
  ungetc(ch,fp);

  if (ch == EOF || ch == ' ')
    return NULL; // a real line of data
  
  char *s= fgets(buf,len,fp); // read the comment
  return s;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::ReadDatabase( const TDatime& /* date */ )
{
  // Default database reader. Currently does nothing.

  return kOK;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::ReadRunDatabase( const TDatime& date )
{
  // Default run database reader. Reads one key, <prefix>.config, into 
  // fConfig. If not found, fConfig is empty. If fConfig was explicitly
  // set with SetConfig(), the run database is not parsed and fConfig is
  // not touched.

  if( !fPrefix ) return kInitError;

  if( (fProperties & kConfigOverride) == 0) {
    FILE* file = OpenRunDBFile( date );
    if( !file ) return kFileError;

    TString name(fPrefix); name.Append("config");
    Int_t ret = LoadDBvalue( file, date, name, fConfig );
    fclose(file);

    if( ret == -1 ) return kFileError;
    if( ret < 0 )   return kInitError;
    if( ret == 1 )  fConfig = "";
  }
  return kOK;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::RemoveVariables()
{
  // Default method for removing global variables of this object

  return DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaAnalysisObject::SetName( const char* name )
{
  // Set/change the name of the object.

  if( !name || !*name ) {
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
void THaAnalysisObject::SetConfig( const char* label )
{
  // Set the "configuration" to select in the database.
  // In text-based database files, this will make the database reader
  // seek to a section header [ config=label ] if the module supports it.

  fConfig = label;
  if( fConfig.IsNull() )
    fProperties &= ~kConfigOverride;
  else
    fProperties |= kConfigOverride;
}

//_____________________________________________________________________________
void THaAnalysisObject::SetDebug( Int_t level )
{
  // Set debug level

  fDebug = level;
}

//---------- Geometry functions -----------------------------------------------
//_____________________________________________________________________________
Bool_t THaAnalysisObject::IntersectPlaneWithRay( const TVector3& xax,
						 const TVector3& yax,
						 const TVector3& org,
						 const TVector3& ray_start,
						 const TVector3& ray_vect,
						 Double_t& length,
						 TVector3& intersect )
{
  // Find intersection point of plane (given by 'xax', 'yax', 'org') with
  // ray (given by 'ray_start', 'ray_vect'). 
  // Returns true if intersection found, else false (ray parallel to plane).
  // Output is in 'length' and 'intersect', where
  //   intersect = ray_start + length*ray_vect
  // 'length' and 'intersect' must be provided by the caller.

  // Calculate explicitly for speed.

  Double_t nom[9], den[9];
  nom[0] = den[0] = xax.X();
  nom[3] = den[3] = xax.Y();
  nom[6] = den[6] = xax.Z();
  nom[1] = den[1] = yax.X();
  nom[4] = den[4] = yax.Y();
  nom[7] = den[7] = yax.Z();
  den[2] = -ray_vect.X();
  den[5] = -ray_vect.Y();
  den[8] = -ray_vect.Z();

  Double_t det1 = den[0]*(den[4]*den[8]-den[7]*den[5])
    -den[3]*(den[1]*den[8]-den[7]*den[2])
    +den[6]*(den[1]*den[5]-den[4]*den[2]);
  if( fabs(det1) < 1e-5 )
    return false;

  nom[2] = ray_start.X()-org.X();
  nom[5] = ray_start.Y()-org.Y();
  nom[8] = ray_start.Z()-org.Z();
  Double_t det2 = nom[0]*(nom[4]*nom[8]-nom[7]*nom[5])
    -nom[3]*(nom[1]*nom[8]-nom[7]*nom[2])
    +nom[6]*(nom[1]*nom[5]-nom[4]*nom[2]);

  length = det2/det1;
  intersect = ray_start + length*ray_vect;
  return true;
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
  Double_t tmp = ct*cp;
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
  if( ph_sph/twopi - floor(ph_sph/twopi) > 0.5 ) ph_geo = -ph_geo;
}

//---------- Database utility functions ---------------------------------------

static string errtxt;
static int loaddb_depth = 0; // Recursion depth in LoadDB
static string loaddb_prefix; // Actual prefix of object in LoadDB (for err msg)

// Local helper functions (could be in an anonymous namespace)
//_____________________________________________________________________________
static Int_t IsDBdate( const string& line, TDatime& date, bool warn = true )
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
  if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%4d-%2d-%2d %2d:%2d:%2d",
	      &yy, &mm, &dd, &hh, &mi, &ss) != 6
      || yy < 1995 || mm < 1 || mm > 12 || dd < 1 || dd > 31
      || hh < 0 || hh > 23 || mi < 0 || mi > 59 || ss < 0 || ss > 59 ) {
    if( warn )
      ::Warning("THaAnalysisObject::IsDBdate()", 
		"Invalid date tag %s", line.c_str());
    return 0;
  }
  date.Set(yy, mm, dd, hh, mi, ss);
  return 1;
}

//_____________________________________________________________________________
static Int_t IsDBkey( const string& line, const char* key, string& text )
{
  // Check if 'line' is of the form "key = value" and, if so, whether the key 
  // equals 'key'. Keys are not case sensitive.
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
  // Extract the key
  while( *ln == ' ' ) ++ln; // find_first_not_of(" ")
  assert( ln <= eq );
  if( ln == eq ) return -1;
  const char* p = eq-1;
  assert( p >= ln );
  while( *p == ' ' ) --p; // find_last_not_of(" ")
  if( strncmp(ln, key, p-ln+1) ) return -1;
  // Key matches. Now extract the value, trimming leading whitespace.
  ln = eq+1;
  assert( !*ln || *(ln+strlen(ln)-1) != ' ' ); // Trailing space already trimmed
  while( *ln == ' ' ) ++ln;
  text = ln;

  return 1;
}

//_____________________________________________________________________________
static Int_t ChopPrefix( string& s )
{
  // Remove trailing level from prefix. Example "L.vdc." -> "L."
  // Return remaining number of dots, or zero if empty/invalid prefix

  ssiz_t len = s.size(), pos;
  Int_t ndot = 0;
  if( len<2 )
    goto null;
  pos = s.rfind('.',len-2);
  if( pos == string::npos ) 
    goto null;
  s.erase(pos+1);
  for( ssiz_t i = 0; i <= pos; i++ ) {
    if( s[i] == '.' )
      ndot++;
  }
  return ndot;

 null:
  s.clear();
  return 0;

}

//_____________________________________________________________________________
bool THaAnalysisObject::IsTag( const char* buf )
{
  // Return true if the string in 'buf' matches regexp ".*\[.+\].*",
  // i.e. it is a database tag.  Generic utility function.

  const char* p = buf;
  while( *p && *p != '[' ) p++;
  if( !*p ) return false;
  p++;
  if( !*p || *p == ']' ) return false;
  p++;
  while( *p && *p != ']' ) p++;
  return ( *p == ']' );
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

//_____________________________________________________________________________
static void Trim( string& str )
{
  // Trim leading and trailing space from string 'str'

  if( str.empty() )
    return;
  const char* p = str.c_str();
  while( *p == ' ' ) ++p;
  if( *p == '\0' )
    str.clear();
  else if( p != str.c_str() ) {
    ssiz_t pos = p-str.c_str();
    str.substr(pos).swap(str);
  }
  if( !str.empty() ) {
    const char* q = str.c_str() + str.length()-1;
    p = q;
    while( *p == ' ' ) --p;
    if( p != q ) {
      ssiz_t pos = p-str.c_str();
      str.erase(pos+1);
    }
  }
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::ReadDBline( FILE* file, char* buf, size_t bufsiz,
				     string& line )
{
  // Get a text line from the database file 'file'. Ignore all comments
  // (anything after a #). Trim trailing whitespace. Concatenate continuation
  // lines (ending with \).
  // Only returns if a non-empty line was found, or on EOF.

  line.clear();

  Int_t r = 0;
  bool maybe_continued = false, unfinished = true;
  string linbuf;
  fpos_t oldpos;
  while( unfinished && fgetpos(file, &oldpos) == 0 &&
	 (r = GetLine(file,buf,bufsiz,linbuf)) == 0 ) {
    // Search for comment or continuation character.
    // If found, remove it and everything that follows.
    bool continued = false, comment = false, has_equal = false,
      trailing_space = false, leading_space = false;
    ssiz_t pos = linbuf.find_first_of("#\\");
    if( pos != string::npos ) {
      if( linbuf[pos] == '\\' )
	continued = true;
      else
	comment = true;
      linbuf.erase(pos);
    }
    // Trim leading and trailing space
    if( !linbuf.empty() ) {
      if( linbuf[0] == ' ')
	leading_space = true;
      if( linbuf[linbuf.length()-1] == ' ')
	trailing_space = true;
      if( leading_space || trailing_space )
	Trim(linbuf);
    }

    if( line.empty() && linbuf.empty() )
      // Nothing to do, i.e. no line building in progress and no data
      continue;

    if( !linbuf.empty() ) {
      has_equal = (linbuf.find('=') != string::npos);
      // Tentative continuation is canceled by a subsequent line with a '='
      if( maybe_continued && has_equal ) {
	// We must have data at this point, so we can exit. However, the line
	// we've just read is obviously a good one, so we must also rewind the
	// file to the previous position so this line can be read again.
	assert( !line.empty() );  // else maybe_continued not set correctly
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
      // skipped anyay, we can simply exit
      break;
    }

    if( line.empty() && !continued && has_equal ) {
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
      linbuf += ' ';
    if( leading_space && !line.empty() && line[line.length()-1] != ' ')
      line += ' ';

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
    assert( !line.empty() );
    if( line[line.length()-1] == ' ')
      line.erase(line.length()-1);
  }
  return r;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* key, string& text )
{
  // Load a data value tagged with 'key' from the database 'file'.
  // Lines starting with "#" are ignored.
  // If 'key' is found, then the most recent value seen (based on time stamps
  // and position within the file) is returned in 'text'.
  // Values with time stamps later than 'date' are ignored.
  // This allows incremental organization of the database where
  // only changes are recorded with time stamps.
  // Return 0 if success, 1 if key not found, <0 if unexpected error.

  if( !file || !key ) return -255;
  TDatime keydate(950101,0), prevdate(950101,0);

  errno = 0;
  errtxt.clear();
  rewind(file);

  static const size_t bufsiz = 256;
  char* buf = new char[bufsiz];

  bool found = false, ignore = false;
  string dbline;
  vector<string> lines;
  while( ReadDBline(file, buf, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Replace text variables in this database line, if any. Multi-valued
    // variables are supported here, although they are only sensible on the LHS
    lines.assign( 1, dbline );
    gHaTextvars->Substitute( lines );
    for( vsiter_t it = lines.begin(); it != lines.end(); ++it ) {
      string& line = *it;
      Int_t status;
      if( !ignore && (status = IsDBkey( line, key, text )) != 0 ) {
	if( status > 0 ) {
	  // Found a matching key for a newer date than before
	  found = true;
	  prevdate = keydate;
	  // we do not set ignore to true here so that the _last_, not the first,
	  // of multiple identical keys is evaluated.
	}
      } else if( IsDBdate( line, keydate ) != 0 ) 
	ignore = ( keydate>date || keydate<prevdate );
    }
  }
  delete [] buf;

  if( errno ) {
    perror( "THaAnalysisObject::LoadDBvalue" );
    return -1;
  }
  return found ? 0 : 1;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* key, Double_t& value )
{
  // Locate key in database, convert the text found to double-precision,
  // and return result in 'value'.
  // This is a convenience function.

  string text;
  Int_t err = LoadDBvalue( file, date, key, text );
  if( err == 0 )
    value = atof(text.c_str());
  return err;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* key, Int_t& value )
{
  // Locate key in database, convert the text found to integer
  // and return result in 'value'.
  // This is a convenience function.

  string text;
  Int_t err = LoadDBvalue( file, date, key, text );
  if( err == 0 )
    value = atoi(text.c_str());
  return err;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDBvalue( FILE* file, const TDatime& date, 
				      const char* key, TString& text )
{
  // Locate key in database, convert the text found to TString
  // and return result in 'text'.
  // This is a convenience function.

  string _text;
  Int_t err = LoadDBvalue( file, date, key, _text );
  if( err == 0 )
    text = _text.c_str();
  return err;
}

//_____________________________________________________________________________
template <class T>
Int_t THaAnalysisObject::LoadDBarray( FILE* file, const TDatime& date, 
				      const char* key, vector<T>& values )
{
  string text;
  Int_t err = LoadDBvalue( file, date, key, text );
  if( err )
    return err;
  values.clear();
  text += " ";
  istringstream inp(text);
  T dval;
  while( 1 ) {
    inp >> dval;
    if( inp.good() )
      values.push_back(dval);
    else
      break;
  }
  return 0;
}

//_____________________________________________________________________________
template <class T>
Int_t THaAnalysisObject::LoadDBmatrix( FILE* file, const TDatime& date, 
				       const char* key, 
				       vector<vector<T> >& values,
				       UInt_t ncols )
{
  // Read a matrix of values of type T into a vector of vectors.
  // The matrix is square with ncols columns.

  vector<T>* tmpval = new vector<T>;
  if( !tmpval )
    return -255;
  Int_t err = LoadDBarray( file, date, key, *tmpval );
  if( err ) {
    delete tmpval;
    return err;
  }
  if( (tmpval->size() % ncols) != 0 ) {
    delete tmpval;
    errtxt = "key = "; errtxt += key;
    return -129;
  }
  values.clear();
  typename vector<vector<T> >::size_type nrows = tmpval->size()/ncols, irow;
  for( irow = 0; irow < nrows; ++irow ) {

    vector<T> row;
    for( typename vector<T>::size_type i=0; i<ncols; ++i ) {
      row.push_back( tmpval->at(i+irow*ncols) );
    }
    values.push_back( row );
  }
  delete tmpval;
  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDB( FILE* f, const TDatime& date,
				 const DBRequest* req, Int_t search )
{
  // Member function version of LoadDB, uses current object's fPrefix and
  // class name

  TString here(GetClassName());
  here.Append("::LoadDB");
  return LoadDB( f, date, req, GetPrefix(), search, here.Data() );
}

#define CheckLimits(T,val) \
  if( (val) < -std::numeric_limits<T>::max() ||    \
      (val) >  std::numeric_limits<T>::max() ) {   \
    ostringstream txt;				   \
    txt << (val);				   \
    errtxt = txt.str();                            \
    goto rangeerr;				   \
  }

#define CheckLimitsUnsigned(T,val) \
  if( (val) < 0 || static_cast<ULong64_t>(val)     \
      > std::numeric_limits<T>::max() ) {          \
    ostringstream txt;				   \
    txt << (val);				   \
    errtxt = txt.str();                            \
    goto rangeerr;                                 \
  }

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDB( FILE* f, const TDatime& date,
				 const DBRequest* req, const char* prefix,
				 Int_t search, const char* here )
{
  // Load a list of parameters from the database file 'f' according to 
  // the contents of the 'req' structure (see VarDef.h).

  if( !req ) return -255;
  if( !prefix ) prefix = "";
  Int_t ret = 0;
  if( loaddb_depth++ == 0 )
    loaddb_prefix = prefix;

  const DBRequest* item = req;
  while( item->name ) {
    if( item->var ) {
      string keystr = prefix; keystr.append(item->name);
      UInt_t nelem = item->nelem;
      const char* key = keystr.c_str();
      if( item->type == kDouble || item->type == kFloat ) {
	if( nelem < 2 ) {
	  Double_t dval;
	  ret = LoadDBvalue( f, date, key, dval );
	  if( ret == 0 ) {
	    if( item->type == kDouble )
	      *((Double_t*)item->var) = dval;
	    else {
	      CheckLimits( Float_t, dval );
	      *((Float_t*)item->var) = dval;
	    }
	  }
	} else {
	  // Array of reals requested
	  vector<double> dvals;
	  ret = LoadDBarray( f, date, key, dvals );
	  if( ret == 0 && static_cast<UInt_t>(dvals.size()) != nelem ) {
	    nelem = dvals.size();
	    ret = -130;
	  } else if( ret == 0 ) {
	    if( item->type == kDouble ) {
	      for( UInt_t i = 0; i < nelem; i++ )
		((Double_t*)item->var)[i] = dvals[i];
	    } else {
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimits( Float_t, dvals[i] );
		((Float_t*)item->var)[i] = dvals[i];
	      }
	    }
	  }
	}
      } else if( item->type >= kInt && item->type <= kByte ) {
	// Implies a certain order of definitions in VarType.h
	if( nelem < 2 ) {
	  Int_t ival;
	  ret = LoadDBvalue( f, date, key, ival );
	  if( ret == 0 ) {
	    switch( item->type ) {
	    case kInt:
	      *((Int_t*)item->var) = ival;
	      break;
	    case kUInt:
	      CheckLimitsUnsigned( UInt_t, ival );
	      *((UInt_t*)item->var) = ival;
	      break;
	    case kShort:
	      CheckLimits( Short_t, ival );
	      *((Short_t*)item->var) = ival;
	      break;
	    case kUShort:
	      CheckLimitsUnsigned( UShort_t, ival );
	      *((UShort_t*)item->var) = ival;
	      break;
	    case kChar:
	      CheckLimits( Char_t, ival );
	      *((Char_t*)item->var) = ival;
	      break;
	    case kByte:
	      CheckLimitsUnsigned( Byte_t, ival );
	      *((Byte_t*)item->var) = ival;
	      break;
	    default:
	      goto badtype;
	    }
	  }
	} else {
	  // Array of integers requested
	  vector<Int_t> ivals;
	  ret = LoadDBarray( f, date, key, ivals );
	  if( ret == 0 && static_cast<UInt_t>(ivals.size()) != nelem ) {
	    nelem = ivals.size();
	    ret = -130;
	  } else if( ret == 0 ) {
	    switch( item->type ) {
	    case kInt:
	      for( UInt_t i = 0; i < nelem; i++ )
		((Int_t*)item->var)[i] = ivals[i];
	      break;
	    case kUInt:
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimitsUnsigned( UInt_t, ivals[i] );
		((UInt_t*)item->var)[i] = ivals[i];
	      }
	      break;
	    case kShort:
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimits( Short_t, ivals[i] );
		((Short_t*)item->var)[i] = ivals[i];
	      }
	      break;
	    case kUShort:
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimitsUnsigned( UShort_t, ivals[i] );
		((UShort_t*)item->var)[i] = ivals[i];
	      }
	      break;
	    case kChar:
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimits( Char_t, ivals[i] );
		((Char_t*)item->var)[i] = ivals[i];
	      }
	      break;
	    case kByte:
	      for( UInt_t i = 0; i < nelem; i++ ) {
		CheckLimitsUnsigned( Byte_t, ivals[i] );
		((Byte_t*)item->var)[i] = ivals[i];
	      }
	      break;
	    default:
	      goto badtype;
	    }
	  }
	}
      } else if( item->type == kString ) {
	ret = LoadDBvalue( f, date, key, *((string*)item->var) );
      } else if( item->type == kTString ) {
	ret = LoadDBvalue( f, date, key, *((TString*)item->var) );
      } else if( item->type == kFloatV ) {
	ret = LoadDBarray( f, date, key, *((vector<float>*)item->var) );
	if( ret == 0 && nelem > 0 && nelem !=
	    static_cast<UInt_t>(((vector<float>*)item->var)->size()) ) {
	  nelem = ((vector<float>*)item->var)->size();
	  ret = -130;
	}
      } else if( item->type == kDoubleV ) {
	ret = LoadDBarray( f, date, key, *((vector<double>*)item->var) );
	if( ret == 0 && nelem > 0 && nelem !=
	    static_cast<UInt_t>(((vector<double>*)item->var)->size()) ) {
	  nelem = ((vector<double>*)item->var)->size();
	  ret = -130;
	}
      } else if( item->type == kIntV ) {
	ret = LoadDBarray( f, date, key, *((vector<Int_t>*)item->var) );
	if( ret == 0 && nelem > 0 && nelem !=
	    static_cast<UInt_t>(((vector<Int_t>*)item->var)->size()) ) {
	  nelem = ((vector<Int_t>*)item->var)->size();
	  ret = -130;
	}
      } else if( item->type == kFloatM ) {
	ret = LoadDBmatrix( f, date, key, 
			    *((vector<vector<float> >*)item->var), nelem );
      } else if( item->type == kDoubleM ) {
	ret = LoadDBmatrix( f, date, key, 
			    *((vector<vector<double> >*)item->var), nelem );
      } else if( item->type == kIntM ) {
	ret = LoadDBmatrix( f, date, key,
			    *((vector<vector<Int_t> >*)item->var), nelem );
      } else {
      badtype:
	if( item->type >= kDouble && item->type <= kObject2P )
	  ::Error( ::Here(here,loaddb_prefix.c_str()),
		   "Key \"%s\": Reading of data type \"%s\" not implemented",
		   key, THaVar::GetEnumName(item->type) );
	else
	  ::Error( ::Here(here,loaddb_prefix.c_str()),
		   "Key \"%s\": Reading of data type \"(#%d)\" not implemented",
		   key, item->type );
	ret = -2;
	break;
      rangeerr:
	::Error( ::Here(here,loaddb_prefix.c_str()),
		 "Key \"%s\": Value %s out of range for requested type \"%s\"",
		 key, errtxt.c_str(), THaVar::GetEnumName(item->type) );
	ret = -3;
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
	    memset( newreq+1, 0, sizeof(DBRequest) );
	    newreq->search = 0;
	    if( newsearch < 0 )
	      newsearch++;
	    ret = LoadDB( f, date, newreq, newprefix.c_str(), newsearch, here );
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
	    ::Error( ::Here(here,loaddb_prefix.c_str()),
		     "Required key \"%s\" (%s) missing in the database.",
		     key, item->descript );
	  } else {
	    ::Error( ::Here(here,loaddb_prefix.c_str()),
		     "Required key \"%s\" missing in the database.", key );
	  }
	  // For missing keys, the return code is the index into the request 
	  // array + 1. In this way the caller knows which key is missing.
	  ret = 1+(item-req);
	  break;
	}
      } else if( ret == -128 ) {  // Line too long
	::Error( ::Here(here,loaddb_prefix.c_str()),
		 "Text line too long. Fix the database!\n\"%s...\"",
		 errtxt.c_str() );
	break;
      } else if( ret == -129 ) {  // Matrix ncols mismatch
	::Error( ::Here(here,loaddb_prefix.c_str()),
		 "Number of matrix elements not evenly divisible by requested "
		 "number of columns. Fix the database!\n\"%s...\"",
		 errtxt.c_str() );
	break;
      } else if( ret == -130 ) {  // Vector/array size mismatch
	::Error( ::Here(here,loaddb_prefix.c_str()),
		 "Incorrect number of array elements found for key = %s. "
		 "%u requested, %u found. Fix database.", keystr.c_str(),
		 item->nelem, nelem );
	break;
      } else {  // other ret < 0: unexpected zero pointer etc.
	::Error( ::Here(here,loaddb_prefix.c_str()), 
		 "Program error when trying to read database key \"%s\". "
		 "CALL EXPERT!", key );
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
Int_t THaAnalysisObject::SeekDBconfig( FILE* file, const char* tag, 
				       const char* label,
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

  static const char* const here = "THaAnalysisObject::SeekDBconfig";

  if( !file || !tag || !*tag ) return 0;
  string _label("[");
  if( label && *label ) {
    _label.append(label);  _label.append("=");
  }
  ssiz_t llen = _label.size();

  bool found = false;

  errno = 0;
  off_t pos = ftello(file);
  if( pos != -1 ) {
    bool quit = false;
    const int LEN = 256;
    char buf[LEN];
    while( !errno && !found && !quit && fgets( buf, LEN, file)) {
      size_t len = strlen(buf);
      if( len<2 || buf[0] == '#' ) continue;      //skip comments
      if( buf[len-1] == '\n') buf[len-1] = 0;     //delete trailing newline
      char* cbuf = ::Compress(buf);
      string line(cbuf); delete [] cbuf;
      ssiz_t lbrk = line.find(_label);
      if( lbrk != string::npos && lbrk+llen < line.size() ) {
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
    perror(here);
    found = false;
  }
  // If not found, rewind to previous position
  if( !found && pos >= 0 && fseeko(file, pos, SEEK_SET) )
    perror(here); // TODO: should throw exception

  return found;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::SeekDBdate( FILE* file, const TDatime& date,
				     Bool_t end_on_tag )
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
  off_t pos = ftello(file);
  if( pos == -1 ) {
    if( errno ) 
      perror(here);
    return 0;
  }
  off_t foundpos = -1;
  bool found = false, quit = false;
  while( !errno && !quit && fgets( buf, LEN, file)) {
    size_t len = strlen(buf);
    if( len<2 || buf[0] == '#' ) continue;
    if( buf[len-1] == '\n') buf[len-1] = 0; //delete trailing newline
    string line(buf);
    if( IsDBdate( line, tagdate, kNoWarn )
	&& tagdate<=date && tagdate>=prevdate ) {
      prevdate = tagdate;
      foundpos = ftello(file);
      found = true;
    } else if( end_on_tag && IsTag(buf))
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
vector<string> THaAnalysisObject::vsplit(const string& s)
{
  // Static utility function to split a string into
  // whitespace-separated strings
  vector<string> ret;
  typedef string::size_type ssiz_t;
  ssiz_t i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      ssiz_t j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
}

#ifdef WITH_DEBUG
//_____________________________________________________________________________
void THaAnalysisObject::DebugPrint( const DBRequest* list )
{
  // Print values of database parameters given in 'list'

  TString prefix(fPrefix);
  if( prefix.EndsWith(".") ) prefix.Chop();
  cout << prefix << " database parameters: " << endl;
  size_t maxw = 1;
  ios_base::fmtflags fmt = cout.flags();
  for( const DBRequest* it = list; it->name; ++it )
    maxw = TMath::Max(maxw,strlen(it->name));
  for( const DBRequest* it = list; it->name; ++it ) {
    cout << "  " << std::left << setw(maxw) << it->name;
    size_t maxc = it->nelem;
    if( maxc == 0 ) maxc = 1;
    if( it->type == kDoubleV )
      maxc = ((vector<Double_t>*)it->var)->size();
    else if( it->type == kIntV )
      maxc = ((vector<Int_t>*)it->var)->size();
    for( size_t i=0; i<maxc; ++i ) {
      cout << "  ";
      switch( it->type ) {
      case kDouble:
	cout << ((Double_t*)it->var)[i];
	break;
      case kFloat:
	cout << ((Float_t*)it->var)[i];
	break;
      case kInt:
	cout << ((Int_t*)it->var)[i];
	break;
      case kIntV:
        cout << ((vector<Int_t>*)it->var)->at(i);
        break;
      case kDoubleV:
        cout << ((vector<Double_t>*)it->var)->at(i);
        break;
      default:
	break;
      }
    }
    cout << endl;
  }
  cout.flags(fmt); // Restore previous cout formatting
}

//_____________________________________________________________________________
template <typename T>
void THaAnalysisObject::WriteValue( T val, int p, int w )
{
  // Helper function for printing debug information
  ios_base::fmtflags fmt = cout.flags();
  streamsize prec = cout.precision();
  if( val < kBig )
    cout << fixed << setprecision(p) << setw(w) << val;
  else
    cout << " --- ";
  cout.flags(fmt);
  cout.precision(prec);
}

// Explicit instantiations
template void THaAnalysisObject::WriteValue<Double_t>( Double_t val, int p=0, int w=5 );
template void THaAnalysisObject::WriteValue<Float_t>( Float_t val, int p=0, int w=5 );

#endif

//_____________________________________________________________________________
TString& THaAnalysisObject::GetObjArrayString( const TObjArray* array, Int_t i )
{
  // Get the string at index i in the given TObjArray

  return (static_cast<TObjString*>(array->At(i)))->String();
}

//_____________________________________________________________________________
void THaAnalysisObject::Print( Option_t* opt ) const
{
  cout << "AOBJ: " << IsA()->GetName()
       << "\t" << GetName()
       << "\t\"";
  if( fPrefix )
    cout << fPrefix;
  cout << "\"\t" << GetTitle()
       << endl;

  // Print warning details
  if( opt && strstr(opt,"WARN") != 0 && !fMessages.empty() ) {
    string name = GetPrefix();
    string::size_type len = name.length();
    if( len>0 && name[len-1] == '.' )
      name.erase(len-1);
    cout << "Module \"" << name << "\" encountered warnings:" << endl;
    for( map<string,UInt_t>::const_iterator it = fMessages.begin();
	 it != fMessages.end(); ++it ) {
      cout << "  " << it->first << ": " << it->second << " times" << endl;
    }
  }
}

//_____________________________________________________________________________
void THaAnalysisObject::PrintObjects( Option_t* opt )
{
  // Print all defined analysis objects (useful for debugging)

  TIter next(fgModules);
  TObject* obj;
  while( (obj = next()) ) {
    obj->Print(opt);
  }
}

//_____________________________________________________________________________
ClassImp(THaAnalysisObject)

