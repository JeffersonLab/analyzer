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

#include <cstring>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>
#ifdef WITH_DEBUG
#include <iostream>
#endif

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
  // ReadDatabase() method. If so, open the database file called "db_<fPrefix>dat" 
  // and, if successful,  call ReadDatabase().
  // 
  // Returns immediately with a warning if the object is already initialized.
  //
  // This implementation will change once the real database is  available.

  Int_t status;
  fStatus = kNotinit;

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Open database file. Don't bother if this object has not implemented
  // its own database reader.

  // Note: requires ROOT >= 3.01 because it needs TClass::GetMethodAllAny()
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
				   const char *here = "OpenFile()",
				   const char *filemode = "r", 
				   const int debug_flag = 1
				   )
{
  // Open database file.

  // FIXME: Try to write this in a system-independent way. Avoid direct Unix 
  // system calls, call ROOT's OS interface instead.

  char *buf = NULL, *dbdir = NULL;
  struct dirent* result;
  DIR* dir;
  size_t size, pos;
  FILE* fi = NULL;
  vector<string> time_dirs;
  vector<string>::iterator it;
  string item, cwd, filename;
  Int_t item_date;
  const string defaultdir("DEFAULT");
  bool have_defaultdir = false, retried = false, found = false;

  // Save current directory name

  errno = 0;
  size = pathconf(".",_PC_PATH_MAX);
  if( size<=0 || errno ) goto error;
  buf = new char[ size ];
  if( !buf || !getcwd( buf, size )) goto error;
  cwd = buf;

  // If environment variable DB_DIR defined, look for database files there,
  // otherwise look in subdirectories "DB" and "db" in turn, then look in the
  // current directory, whichever is found first.

  if( (dbdir = getenv("DB_DIR")) && chdir( dbdir ))
    goto error;
  else if ( (chdir("DB") == 0) || (chdir("db") == 0) ) {
    ;
  }

  // If any subdirectories named "YYYYMMDD" exist, where YYYY, MM, and DD are
  // digits, assume that they contain time-dependent database files. Select 
  // the directory matching the timestamp 'date' of the current run. 
  // If no such directories exist, or none is valid for the give date, 
  // but "DEFAULT" exists, use it. 
  // Otherwise, use the current directory to look for database files.

  // Get names of all subdirectories matching the YYYYMMDD pattern

  if( !(dir = opendir("."))) goto error;
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

  // If any found, select the one that corresponds to the given date.
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
    // If there are YYYMMDD subdirectories, one of them must be valid for the
    // given date.
    if( !found && !have_defaultdir ) {
      cerr<<here<<"Time-dependent database subdirectories exist, but "
    	  <<"none is valid for the requested date: "<<date.AsString()<<endl;
      goto exit;
    }
    if( found && chdir( (*it).c_str()) ) {
      cerr<<here<<": cannot open database directory"<<(*it).c_str()<<endl;
      goto exit;
    }
  }
  if( !found && have_defaultdir && chdir( defaultdir.c_str()) ) {
    cerr<<here<<": cannot open database directory"<<defaultdir.c_str()<<endl;
    goto exit;
  }
  if( !getcwd(buf,size)) goto error;

  // Construct the database file name. It is of the form db_<prefix>.dat.
  // Subdetectors use the same files as their parent detectors!
  filename = "db_";
  filename.append(name);
  filename.append("dat");

 retry:
#ifdef WITH_DEBUG
  if( debug_flag>0 ) {
    cout << "<THaAnalysisObject::" << here 
	 << ">: Opening database file " << buf << "/" << filename;
  }
#endif
  // Open the database file
  fi = fopen( filename.c_str(), filemode);

  // If database cannot be opened, but "DEFAULT" exists, and we are not already in 
  // "DEFAULT", try opening the file in "DEFAULT" before failing.
#ifdef WITH_DEBUG
  if( debug_flag>0 ) 
    if( !fi ) cout << " ... failed" << endl;
    else      cout << " ... ok" << endl;
#endif
  if( !fi && !retried && found && have_defaultdir && chdir( ".." ) == 0
      && chdir( defaultdir.c_str()) == 0 ) {
    if( !getcwd(buf,size)) goto error;
    retried = true;
    goto retry;
  }
  if( !fi )
    cerr<<here<<": Cannot open database file "<<buf<<filename.c_str()<<"."<<endl;
  goto exit;

 error:
  perror(here);

 exit:
  if( cwd.length() > 0 ) 
    chdir( cwd.c_str() );
  delete [] buf;
  return fi;

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
  // Set/change the name of the detector.

  if( !name || strlen(name) == 0 ) {
    Warning( Here("SetName()"),
	     "Cannot set an empty detector name. Name not set.");
    return;
  }
  TNamed::SetName( name );
  MakePrefix();
}

//_____________________________________________________________________________
void THaAnalysisObject::SetNameTitle( const char* name, const char* title )
{
  // Set/change the name of the detector.

  SetName( name );
  SetTitle( title );
}


