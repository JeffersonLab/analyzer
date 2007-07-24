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
#include "TVector3.h"
#include "TSystem.h"

#include <cstring>
#include <cctype>
#include <errno.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>

using namespace std;
typedef string::size_type ssiz_t;

TList* THaAnalysisObject::fgModules = NULL;

const Double_t THaAnalysisObject::kBig = 1.e38;

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject( const char* name, 
				      const char* description ) :
  TNamed(name,description), fPrefix(NULL), fStatus(kNotinit), 
  fDebug(0), fIsInit(false), fIsSetup(false), fProperties(0),
  fOKOut(false), fInitDate(19950101,0)
{
  // Constructor

  if( !fgModules ) fgModules = new TList;
  fgModules->Add( this );
}

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject( ) : fPrefix(NULL), fIsSetup(false) {
  // only for ROOT I/O
}

//_____________________________________________________________________________
THaAnalysisObject::~THaAnalysisObject()
{
  // Destructor

  if (fgModules) {
    fgModules->Remove( this );
    if( fgModules->GetSize() == 0 ) {
      delete fgModules; fgModules = 0;
    }
  }
  if (fPrefix) {
    delete [] fPrefix; fPrefix = 0;
  }
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::Begin( THaRunBase* run )
{
  // Method usually called right before the start of the event loop
  // for 'run'. Begin() is similar to Init(), but since there is a default
  // Init() implementing standard database and global variable setup,
  // Begin() can be used to implement start-of-run setup tasks for each
  // module cleanly without interfering with the standard Init() prodecure.
  //
  // The default Begin() method does nothing.

  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVariables( EMode mode )
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

  return DefineVarsFromList( list, type, mode, var_prefix, this, 
			     fPrefix, Here(ClassName()) );
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
      action = "deleted (this is safe when exitting)";
    ::Warning( "DefineVariables", 
	     "No global variable list found. No variables %s.", 
	     action.Data() );
    return (mode==kDefine ? kInitError : kOK);
  }

  if( mode == kDefine ) {
    if( type == kVarDef )
      gHaVars->DefineVariables( (const VarDef*)list, prefix, here );
    else if( type == kRVarDef )
      gHaVars->DefineVariables( (const RVarDef*)list, obj,
				prefix, here, var_prefix );
  }
  else if( mode == kDelete ) {
    if( type == kVarDef ) {
      const VarDef* item;
      const VarDef* theList = (const VarDef*)list;
      while( (item = theList++) && item->name ) {
	TString name(prefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
      }
    } else if( type == kRVarDef ) {
      const RVarDef* item;
      const RVarDef* theList = (const RVarDef*)list;
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
Int_t THaAnalysisObject::End( THaRunBase* run )
{
  // Method usually called right after the end of the event loop for 'run'.
  // May be used by modules to clean up, compute averages, write summaries, etc.
  //
  // The default End() method does nothing.

  return 0;
}

//_____________________________________________________________________________
THaAnalysisObject* THaAnalysisObject::FindModule( const char* name,
						  const char* classname )
{
  // Locate the object 'name' in the global list of Analysis Modules 
  // and check if it inherits from 'classname' (if given), and whether 
  // it is properly initialized.
  // Return pointer to valid object, else return NULL and set fStatus.

  static const char* const here = "FindModule()";
  static const char* const anaobj = "THaAnalysisObject";

  EStatus save_status = fStatus;
  fStatus = kInitError;
  if( !name || !*name ) {
    Error( Here(here), "No module name given." );
    return NULL;
  }
  TObject* obj = fgModules->FindObject( name );
  if( !obj ) {
    Error( Here(here), "Module %s does not exist.", name );
    return NULL;
  }
  if( !obj->IsA()->InheritsFrom( anaobj )) {
    Error( Here(here), "Module %s (%s) is not a %s.",
	   obj->GetName(), obj->GetTitle(), anaobj );
    return NULL;
  }
  if( classname && *classname && strcmp(classname,anaobj) &&
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

//_____________________________________________________________________________
vector<string> THaAnalysisObject::GetDBFileList( const char* name, 
						 const TDatime& date,
						 const char* here )
{
  // Return the database file searchlist as a vector of strings.
  // The file names are relative to the current directory.

  const char* dbdir = NULL;
  const char* result;
  void* dirp;
  size_t pos;
  vector<string> time_dirs, dnames, fnames;
  vector<string>::iterator it, thedir;
  string item, filename;
  Int_t item_date;
  const string defaultdir("DEFAULT");
  bool have_defaultdir = false, found = false;

  // Build search list of directories
  if( (dbdir = gSystem->Getenv("DB_DIR")))
    dnames.push_back( dbdir );
  dnames.push_back( "DB" );
  dnames.push_back( "db" );
  dnames.push_back( "." );

  // Try to open the database directories in the search list.
  // The first directory that can be opened is taken to be the database
  // directory. Subsequent directories are ignored.
  
  it = dnames.begin();
  while( !(dirp = gSystem->OpenDirectory( (*it).c_str() )) && 
	 (++it != dnames.end()) );

  // None of the directories can be opened?
  if( it == dnames.end() ) {
    ::Error( here, "Cannot open any database directories. Check your disk!");
    goto exit;
  }

  // Pointer to database directory string
  thedir = it;

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
  // ./filename <dbdir/<date-dir>/filename <dbdir>/DEFAULT/filename <dbdir>/filename

  fnames.push_back( filename );
  if( found ) {
    item = *thedir + "/" + *it + "/" + filename;
    fnames.push_back( item );
  }
  if( have_defaultdir ) {
    item = *thedir + "/" + defaultdir + "/" + filename;
    fnames.push_back( item );
  }
  fnames.push_back( *thedir + "/" + filename );

 exit:
  return fnames;
}

//_____________________________________________________________________________
const char* THaAnalysisObject::GetDBFileName() const
{
  return GetPrefix();
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

  if( IsZombie() )
    return fStatus = kNotinit;

  fInitDate = date;
  
  Int_t status = kOK;
  const char* fnam = "run.";

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Open the run database and call the reader. If database cannot be opened,
  // fail only if this object needs the run database
  // Call this object's actual database reader
  status = ReadRunDatabase(date);
  if( status && (status != kFileError || (fProperties & kNeedsRunDB) != 0))
    goto err;

  // Read the database for this object.
  // Don't bother if this object has not implemented its own database reader.

  // Note: requires ROOT >= 3.01 because of TClass::GetMethodAllAny()
  if( IsA()->GetMethodAllAny("ReadDatabase")
      != gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadDatabase")){

    // Call this object's actual database reader
    fnam = fPrefix;
    if( (status = ReadDatabase(date)) )
      goto err;
  } 
  else if ( fDebug>0 ) {
    cout << "Info: Skipping database call for object " << GetName() 
	 << " since no ReadDatabase function defined.\n";
  }

  // Define this object's variables.
  status = DefineVariables(kDefine);

  Clear();
  goto exit;

 err:
  if( status == kFileError )
    Error(Here("Init"),"Cannot open database file db_%sdat",fnam);
 exit:
  return fStatus = (EStatus)status;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::InitOutput( THaOutput *output )
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
    strcat( fPrefix, GetName() );
    strcat( fPrefix, "." );
  } else {
    fPrefix = new char[ strlen(GetName()) + 2 ];
    strcpy( fPrefix, GetName() );
    strcat( fPrefix, "." );
  }
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
    vector<string>::iterator it = fnames.begin();
    do {
      if( debug_flag>1 )
      	cout << "<" << here << ">: Opening database file " << *it;
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

  return OpenFile(GetDBFileName(), date, Here("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenRunDBFile( const TDatime& date )
{ 
  // Default method for opening run database file

  return OpenFile("run", date, Here("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
char* THaAnalysisObject::ReadComment( FILE* fp, char *buf, const int len )
{
  // Read blank and comment lines ( those starting non-starting with a
  //  space (' '), returning the comment.
  // If the line is data, then nothing is done and NULL is returned
  // (so one can search for the next data line with:
  //   while ( ReadComment(fp, buf, len) );
  int ch = fgetc(fp);  // peak ahead one character
  ungetc(ch,fp);

  if (ch == EOF || ch == ' ')
    return NULL; // a real line of data
  
  char *s= fgets(buf,len,fp); // read the comment
  return s;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::ReadDatabase( const TDatime& date )
{
  // Default database reader. Currently does nothing.

  return kOK;
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

//---------- Database utility functions ---------------------------------------
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
  // Check if 'line' is of the form "tag = value" and, if so, whether the tag 
  // equals 'tag'. If there is no '=', then return zero;
  // If there is a '=', but the left-hand side doesn't match 'tag',
  // or if there is NO text after the '=' (obviously a bad database entry), 
  // then return -1. 
  // If tag found, parse the line, set 'text' to the text after the "=", 
  // and return +1. Tags are NOT case sensitive.
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
  if( pos1 == string::npos ) return -1;
  pos2 = rhs.find_last_not_of(" \t");
  text = rhs.substr(pos1,pos2-pos1+1);
  return 1;
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
      Int_t ret=0;
      switch ( item->type ) {
      case kDouble:
	ret = LoadDBvalue( f, date, tag, *((Double_t*)item->var)); break;
      default:
	cerr << "THaAnalysisObject::LoadDB  "
	     << "READING of VarType " << item->type << " for item " << tag
	     << " (see VarType.h) not implemented at this point !!! \n"
	     << endl;
	ret = -1;
      }

      if (ret && item->fatal )
	return item->fatal;
    }
    item++;
  }
  return 0;
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

  bool found = false, ignore = false;
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

  if( !file || !tag || !*tag ) return 0;
  string _label("[");
  if( label && *label ) {
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
ClassImp(THaAnalysisObject)

