//*-- Author :    Ole Hansen   15-Jun-01

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
// Abstract base class for a detector or subdetector.
//
// Derived classes must define all internal variables, a constructor 
// that registers any internal variables of interest in the global 
// physics variable list, and a Decode() method that fills the variables
// based on the information in the THaEvData structure.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"
#include "THaDetMap.h"
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

ClassImp(THaDetectorBase)

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase( const char* name, 
				  const char* description ) :
  TNamed(name,description), fPrefix(NULL), fStatus(kNotinit), fDebug(0),
  fIsInit(false), fIsSetup(false)
{
  // Normal constructor. Creates an empty detector map.

  fSize[0] = 0.0;
  fSize[1] = 0.0;
  fSize[2] = 0.0;
  fDetMap = new THaDetMap;
}

//_____________________________________________________________________________
THaDetectorBase::~THaDetectorBase()
{
  // Destructor

  delete [] fPrefix;
  delete fDetMap;
}

//_____________________________________________________________________________
Int_t THaDetectorBase::DefineVariables( const VarDef* list ) const
{
  // Add variables defined in 'list' to the list of global variables,
  // using prefix of the current detector.
  // Internal function called by detector initialization methods.

  Int_t retval = 0;

  if( gHaVars ) {
    retval = gHaVars->DefineVariables( list, fPrefix, Here(ClassName()) );
  } else
    Warning( Here("DefineVariables()"), 
		  "No global variable list found. No variables defined.");
			      
  return retval;
}

//_____________________________________________________________________________
Int_t THaDetectorBase::DefineVariables( const RVarDef* list ) const
{
  // Add variables defined in 'list' to the list of global variables,
  // using prefix of the current detector.
  // Internal function called by detector initialization methods.
  // This form uses ROOT object/RTTI-based definitions.

  Int_t retval = 0;

  if( gHaVars ) {
    retval = gHaVars->DefineVariables( list, this, 
				       fPrefix, Here(ClassName()) );
  } else
    Warning( Here("DefineVariables()"), 
		  "No global variable list found. No variables defined.");
			      
  return retval;
}

//_____________________________________________________________________________
const char* THaDetectorBase::Here( const char* here ) const
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
THaDetectorBase::EStatus THaDetectorBase::Init()
{
  // Initialize detector for current time. See Init(date) below.

  TDatime now;
  return Init(now);
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaDetectorBase::Init( const TDatime& date )
{
  // Common Init function for all detectors and subdetectors.
  // Opens database file "db_<fPrefix>dat" and, if successful,  
  // calls ReadDatabase(), which must be implemented by all detector classes.
  // 
  // Returns immediately with a warning if the detector is already initialized.
  //
  // The behavior of this function will change once the real database is 
  // available.

  Int_t status;
  fStatus = kNotinit;

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Open database file. Don't bother if this object has not implemented
  // its own database reader.

  // FIXME: This is what I really want, but GetMethodAllAny is not
  // available in all ROOT 3.0x versions ... :-(
  //  TClass* c = gROOT->GetClass("THaDetectorBase");
  //  if( !c || ( this->IsA()->GetMethodAllAny("ReadDatabase")
  //      != c->GetMethodAllAny("ReadDatabase") )) {
  if( this->IsA()->GetMethodAny("ReadDatabase") ) {
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
    cout << "Info: Skipping database call for detector " << GetName() 
	 << " since no ReadDatabase function defined.\n";
  }
#endif

  // Call this object's setup function
  status = SetupDetector( date );
  if( status )
    return fStatus = kInitError;
  
  return fStatus = kOK;
}

//_____________________________________________________________________________
FILE* THaDetectorBase::OpenFile( const char *name, const TDatime& date,
				 const char *here = "OpenFile()",
				 const char *filemode = "r", 
				 const int debug_flag = 1
				 )
{
  // Open database file.

  // FIXME: Try to write this in a system-independent way. Avoid direct Unix 
  // system calls, call ROOT's OS interface instead.

  //static const char* const here = "OpenFile()";
  char *buf = NULL, *dbdir = NULL;
  struct dirent* result;
  DIR* dir;
  size_t size, pos;
  FILE* fi = NULL;
  vector<string> time_dirs;
  vector<string>::iterator it;
  string item, cwd, filename;
  Int_t item_date;

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
  // the directory matching the timestamp 'date' of the current run. Otherwise, 
  // use the current directory to look for database files.

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
    }
  }
  if( errno )  goto error;
  closedir(dir);

  // If any found, select the one that corresponds to the given date.
  if( time_dirs.size() > 0 ) {
    bool found = false;
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
    if( !found ) {
      cerr<<here<<"Time-dependent database subdirectories exist, but "
	  <<"none is valid for the requested date: "<<date.AsString()<<endl;
      goto exit;
    }
    if( chdir( (*it).c_str()) ) {
      cerr<<here<<": cannot open database directory"<<(*it).c_str()<<endl;
      goto exit;
    }
  }
  if( !getcwd(buf,size)) goto error;

  // Construct the database file name. It is of the form db_<prefix>.dat.
  // Subdetectors use the same files as their parent detectors!
  filename = "db_";
  filename.append(name);
  filename.append("dat");

#ifdef WITH_DEBUG
  if( debug_flag>0 ) {
    cout << "<THaDetectorBase::" << here 
	 << ">: Opening database file " << buf << "/" << filename << endl;
  }
#endif
  fi = fopen( filename.c_str(), filemode);
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
void THaDetectorBase::MakePrefix( const char* basename )
{
  // Set up name prefix for global variables. 
  // Internal function called by constructors of derived classes.

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
void THaDetectorBase::PrintDetMap( Option_t* opt ) const
{
  // Print out detector map.

  fDetMap->Print( opt );
}

//_____________________________________________________________________________
Int_t THaDetectorBase::RemoveVariables() const
{
  // Remove variables with my prefix from global list.
  // Protected function called by detector destructors.

  if( !gHaVars ) return 0;
  string expr(fPrefix);
  expr.append("*");
  return gHaVars->RemoveRegexp( expr.c_str(), kTRUE );
}

//_____________________________________________________________________________
void THaDetectorBase::SetName( const char* name )
{
  // Set/change the name of the detector.

  if( !name || strlen(name) == 0 ) {
    Warning( Here("SetName()"), "Cannot set an empty detector name. "
	     "Name not set.");
    return;
  }
  TNamed::SetName( name );
  MakePrefix();
}

//_____________________________________________________________________________
void THaDetectorBase::SetNameTitle( const char* name, const char* title )
{
  // Set/change the name of the detector.

  SetName( name );
  SetTitle( title );
}


//_____________________________________________________________________________
void THaDetectorBase::DefineAxes(Double_t rotation_angle)
{
  // define variables used for calculating intercepts of tracks
  // with the detector
  // right now, we assume that all detectors except VDCs are 
  // perpendicular to the Transport frame

  fXax.SetXYZ( TMath::Sin(rotation_angle), 0.0, TMath::Cos(rotation_angle) );
  fYax.SetXYZ( 0.0, 1.0, 0.0 );
  fZax = fXax.Cross(fYax);

  //cout<<"Z: "<<fZax.X()<<" "<<fZax.Y()<<" "<<fZax.Z()<<endl;

  fDenom.ResizeTo( 3, 3 );
  fNom.ResizeTo( 3, 3 );
  fDenom.SetColumn( fXax, 0 );
  fDenom.SetColumn( fYax, 1 );
  fNom.SetColumn( fXax, 0 );
  fNom.SetColumn( fYax, 1 );
}

//_____________________________________________________________________________
bool THaDetectorBase::CalcTrackIntercept(THaTrack *theTrack, 
					 Double_t &t, Double_t &xcross, 
					 Double_t &ycross)
{
  // projects a given track on to the plane of the detector
  // xcross and ycross are the x and y coords of this intersection
  // t is the distance from the origin of the track to the given plane

  TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
  Double_t norm = TMath::Sqrt(1.0 + theTrack->GetTheta()*theTrack->GetTheta() +
			      theTrack->GetPhi()*theTrack->GetPhi());
  TVector3 t_hat( theTrack->GetTheta()/norm, theTrack->GetPhi()/norm, 1.0/norm );
  fDenom.SetColumn( -t_hat, 2 );
  fNom.SetColumn( t0-fOrigin, 2 );

  // first get the distance...
  Double_t det = fDenom.Determinant();
  if( fabs(det) < 1e-5 ) 
      return false;  // No useful solution for this track
  cout<<theTrack->GetX()<<" "<<theTrack->GetY()<<endl;
  t = fNom.Determinant() / det;

  // ...then the intersection point
  TVector3 v = t0 + t*t_hat - fOrigin;
  xcross = v.Dot(fXax);
  ycross = v.Dot(fYax);

  return true;
}

//_____________________________________________________________________________
bool THaDetectorBase::CheckIntercept(THaTrack *track)
{
  Double_t x, y, t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaDetectorBase::CalcInterceptCoords(THaTrack *track, Double_t &x, Double_t &y)
{
  Double_t t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaDetectorBase::CalcPathLen(THaTrack *track, Double_t &t)
{
  Double_t x, y;
  return CalcTrackIntercept(track, t, x, y);
}
