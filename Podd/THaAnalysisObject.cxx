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
#include "TVector3.h"
#include "TSystem.h"
#include "TString.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>
#include <exception>
#include <cassert>
#include <iomanip>
#include <type_traits>
#include <limits>

using namespace std;
using namespace Podd;

TList* THaAnalysisObject::fgModules = nullptr;

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject( const char* name,
				      const char* description ) :
  TNamed(name,description), fPrefix(nullptr), fStatus(kNotinit),
  fDebug(0), fIsInit(false), fIsSetup(false), fProperties(0),
  fOKOut(false), fInitDate(19950101,0), fNEventsWithWarnings(0),
  fExtra(nullptr)
{
  // Constructor

  if( !fgModules ) fgModules = new TList;
  fgModules->Add( this );
}

//_____________________________________________________________________________
THaAnalysisObject::THaAnalysisObject()
  : fPrefix(nullptr), fStatus(kNotinit), fDebug(0), fIsInit(false),
    fIsSetup(false), fProperties(), fOKOut(false), fNEventsWithWarnings(0),
    fExtra(nullptr)
{
  // only for ROOT I/O
}

//_____________________________________________________________________________
THaAnalysisObject::~THaAnalysisObject()
{
  // Destructor

  RemoveVariables();

  delete fExtra; fExtra = nullptr;

  if (fgModules) {
    fgModules->Remove( this );
    if( fgModules->GetSize() == 0 ) {
      delete fgModules;
      fgModules = nullptr;
    }
  }
  delete [] fPrefix; fPrefix = nullptr;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::Begin( THaRunBase* /* run */ )
{
  // Method called right before the start of the event loop
  // for 'run'. Begin() is similar to Init(), but since there is a default
  // Init() implementing standard database and global variable setup,
  // Begin() can be used to implement start-of-run setup tasks for each
  // module cleanly without interfering with the standard Init() procedure.
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
    for(const auto& fMessage : fMessages) {
      ntot += fMessage.second;
    }
    ostringstream msg;
    msg << endl
	<< "  Encountered " << fNEventsWithWarnings << " events with "
	<< "warnings, " << ntot << " total warnings"
	<< endl
	<< R"/(  Call Print("WARN") for channel list. )/"
	<< "Re-run with fDebug>0 for per-event details.";
    Warning( Here("End"), "%s", msg.str().c_str() );
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVariablesWrapper( EMode mode )
{
  // Wrapper for calling this object's virtual DefineVariables method.
  // It takes care of setting/unsetting the guard variable fIsSetup which
  // prevents multiple attempts to define variables.

  if( mode == kDefine && fIsSetup )
    // Exit if already set up.
    // Multiple attempts to remove variables are OK: If this detector's
    // variables have already been removed, nothing will be done.
    // This test can be present in DefineVariables as well, but does not
    // have to be. However, unlike in analyzer versions before 1.7,
    // DefineVariables methods should NOT set or clear fIsSetup themselves.
    return kOK;

  Int_t ret = DefineVariables(mode);
  if( ret )
    // If setting or removing failed, allow retry. Currently, this would only
    // happen if no global variable list was defined.
    // Inability to define individual variables simply triggers warnings.
    // TODO: Have this behave differently? Failing variable definitions can
    // TODO: be hard to spot and so may waste quite a bit of time ...
    // Inability to find variables for removal will not even be reported since
    // it has no practical effect.
    return ret;

  fIsSetup = (mode == kDefine);

  return ret;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVariables( EMode /* mode */ )
{
  // Default method for defining global variables. Currently does nothing.
  // If there should be any common variables for all analysis objects,
  // define them here.

  return kOK;
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const VarDef* list, EMode mode,
                                             const char* def_prefix,
                                             const char* comment_subst ) const
{
  return DefineVarsFromList(list, kVarDef, mode, def_prefix, comment_subst);
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const RVarDef* list, EMode mode,
                                             const char* def_prefix,
                                             const char* comment_subst ) const
{
  return DefineVarsFromList(list, kRVarDef, mode, def_prefix, comment_subst);
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list, EType type,
                                             EMode mode, const char* def_prefix,
                                             const char* comment_subst ) const
{
  // Add/delete variables defined in 'list' to/from the list of global
  // variables, using prefix of the current apparatus.
  // Internal function that can be called during initialization.

  TString here(GetClassName());
  here.Append("::DefineVarsFromList");
  return DefineVarsFromList(list, type, mode, def_prefix, this,
                            fPrefix, here.Data(), comment_subst);
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::DefineVarsFromList( const void* list,
                                             EType type, EMode mode,
                                             const char* def_prefix,
                                             const TObject* obj,
                                             const char* prefix,
                                             const char* here,
                                             const char* comment_subst )
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
      gHaVars->DefineVariables(static_cast<const RVarDef*>(list), obj,
                               prefix, ::Here(here, prefix), def_prefix,
                               comment_subst);
  }
  else if( mode == kDelete ) {
    if( type == kVarDef ) {
      const auto *item = static_cast<const VarDef*>(list);
      while( item && item->name ) {
	TString name(prefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
	++item;
      }
    } else if( type == kRVarDef ) {
      const auto *item = static_cast<const RVarDef*>(list);
      while( item && item->name ) {
	TString name(prefix);
	name.Append( item->name );
	gHaVars->RemoveName( name );
	++item;
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
  // Return pointer to valid object, else return nullptr.
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
    return nullptr;
  }

  // Find the module in the list, comparing 'name' to the module's fPrefix
  TIter next(fgModules);
  TObject* obj = nullptr;
  while( (obj = next()) ) {
#ifdef NDEBUG
    auto* module = static_cast<THaAnalysisObject*>(obj);
#else
    auto* module = dynamic_cast<THaAnalysisObject*>(obj);
    assert(module);
#endif
    TString prefix = module->GetPrefixName();
    if( prefix.IsNull() ) {
      module->MakePrefix();
      prefix = module->GetPrefixName();
      if( prefix.IsNull() )
	continue;
    }
    if( prefix == name )
      break;
  }
  if( !obj ) {
    if( do_error )
      Error( Here(here), "Module %s does not exist.", name );
    fStatus = kInitError;
    return nullptr;
  }

  // Type check (similar to dynamic_cast, except resolving the class name as
  // a string at run time
  if( !obj->IsA()->InheritsFrom( anaobj )) {
    if( do_error )
      Error( Here(here), "Module %s (%s) is not a %s.",
	     obj->GetName(), obj->GetTitle(), anaobj );
    fStatus = kInitError;
    return nullptr;
  }
  if( classname && *classname && strcmp(classname,anaobj) != 0 &&
      !obj->IsA()->InheritsFrom( classname )) {
    if( do_error )
      Error( Here(here), "Module %s (%s) is not a %s.",
	     obj->GetName(), obj->GetTitle(), classname );
    fStatus = kInitError;
    return nullptr;
  }
  auto* aobj = static_cast<THaAnalysisObject*>( obj );
  if( do_error ) {
    if( !aobj->IsOK() ) {
      Error( Here(here), "Module %s (%s) not initialized.",
	     obj->GetName(), obj->GetTitle() );
      fStatus = kInitError;
      return nullptr;
    }
  }
  return aobj;
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
class database_error : public std::runtime_error {
public:
  explicit database_error( Int_t st, const char* fnam )
    : std::runtime_error("Error reading database"),
      status(st), filename(fnam) {}
  Int_t status;
  const char* filename;
};

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

  // Generate the name prefix for global variables. Do this here, not in
  // the constructor, so we can use a virtual function - detectors and
  // especially subdetectors may have their own idea what prefix they like.
  MakePrefix();

  // Skip reinitialization if there is no (relevant) date change.
  if( DBDatesDiffer(date, fInitDate) ) {
    try {
      // Open the run database and call the reader. If database cannot be opened,
      // fail only if this object needs the run database
      // Call this object's actual database reader
      Int_t status = ReadRunDatabase(date);
      if( status && (status != kFileError || (fProperties & kNeedsRunDB) != 0) ) {
        throw database_error(status, "run.");
      }

      // Read the database for this object.
      // Don't bother if this object has not implemented its own database reader.
      if( IsA()->GetMethodAllAny("ReadDatabase") !=
          gROOT->GetClass("THaAnalysisObject")->GetMethodAllAny("ReadDatabase") ) {

        // Call this object's actual database reader
        if( (status = ReadDatabase(date)) )
          throw database_error(status, GetDBFileName());

      } else if( fDebug > 2 ) {
        Info(Here(here), "No ReadDatabase function defined. "
                         "Database not read.");
      }
    }

    catch( const database_error& e ) {
      if( e.status == kFileError )
        Error(Here(here), "Cannot open database file db_%sdat", e.filename);
      else
        Error(Here(here), "Error while reading file db_%sdat", e.filename);
      return fStatus = static_cast<EStatus>(e.status);
    }
    catch( const std::bad_alloc& ) {
      Error(Here(here), "Out of memory in ReadDatabase.");
      return fStatus = kInitError;
    }
    catch( const std::exception& e ) {
      Error(Here(here), "Exception \"%s\" caught in ReadDatabase. "
                        "Module not initialized. Check database or call expert.",
            e.what());
      return fStatus = kInitError;
    }
  } else if( fDebug > 1 ) {
    Info(Here(here), "Not re-reading database for same date.");
  }

  // Save the last successful initialization date. This is used to prevent
  // unnecessary reinitialization.
  fInitDate = date;

  // Define this object's variables.
  fStatus = static_cast<EStatus>( DefineVariablesWrapper(kDefine) );

  // Clear() the object. The "I" option indicates that the call comes from
  // Init(), which can be used to perform or skip certain steps, as needed.
  Clear("I");

  return fStatus;
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
  // If basename != nullptr,
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

  MakePrefix(nullptr);
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
    Int_t ret = Podd::LoadDBvalue(file, date, name, fConfig );
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
  // Remove global variables of this object.
  // Should be called form the destructor of every class that defines any
  // global variables via the standard DefineVariables mechanism.

  if( fIsSetup )
    DefineVariablesWrapper( kDelete );
  return kOK;
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

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenFile( const TDatime& date )
{
  // Default method for opening database file

  return OpenDBFile(GetDBFileName(), date, ClassNameHere("OpenDBFile()"),
                    "r", fDebug);
}

//_____________________________________________________________________________
FILE* THaAnalysisObject::OpenRunDBFile( const TDatime& date )
{
  // Default method for opening run database file

  return OpenDBFile("run", date, ClassNameHere("OpenDBFile()"),
                    "r", fDebug);
}

//_____________________________________________________________________________
Int_t THaAnalysisObject::LoadDB( FILE* f, const TDatime& date,
                                 const DBRequest* req, Int_t search ) const {
  // Member function version of LoadDB, uses current object's fPrefix and
  // class name

  TString here(GetClassName());
  here.Append("::LoadDB");
  return LoadDB(f, date, req, GetPrefix(), search, here.Data());
}

#ifdef WITH_DEBUG
//_____________________________________________________________________________
void THaAnalysisObject::DebugPrint( const DBRequest* list ) const
{
  // Print values of database parameters given in 'list'

  if( !list )
    return;
  cout << GetPrefixName() << " database parameters: " << endl;
  size_t maxw = 1;
  ios_base::fmtflags fmt = cout.flags();
  for( const auto *it = list; it->name; ++it )
    maxw = TMath::Max(maxw,strlen(it->name));
  for( const auto *it = list; it->name; ++it ) {
    cout << "  " << std::left << setw(static_cast<int>(maxw)) << it->name;
    size_t maxc = it->nelem;
    if( maxc == 0 ) maxc = 1;
    if( it->type == kDoubleV )
      maxc = ((vector<Double_t>*)it->var)->size();
    else if( it->type == kFloatV )
      maxc = ((vector<Float_t>*)it->var)->size();
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
      case kFloatV:
        cout << ((vector<Float_t>*)it->var)->at(i);
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
  if( std::is_floating_point<T>::value && val < kBig )
    cout << fixed << setprecision(p) << setw(w) << val;
  else if( std::is_integral<T>::value && val != THaVar::kInvalidInt &&
           val != numeric_limits<T>::max() && val < 10 * static_cast<T>(w) &&
           (std::is_unsigned<T>::value || -val < 10 * static_cast<T>(w)) )
    cout << setw(w) << val;
  else
    cout << " --- ";
  cout.flags(fmt);
  cout.precision(prec);
}

// Explicit instantiations
template void THaAnalysisObject::WriteValue<Double_t>( Double_t val, int p=0, int w=5 );
template void THaAnalysisObject::WriteValue<Float_t>( Float_t val, int p=0, int w=5 );
template void THaAnalysisObject::WriteValue<Int_t>( Int_t val, int p=0, int w=5 );
template void THaAnalysisObject::WriteValue<UInt_t>( UInt_t val, int p=0, int w=5 );

#endif

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
  if( opt && strstr(opt,"WARN") != nullptr && !fMessages.empty() ) {
    string name = GetPrefix();
    string::size_type len = name.length();
    if( len>0 && name[len-1] == '.' )
      name.erase(len-1);
    cout << "Module \"" << name << "\" encountered warnings:" << endl;
    for( const auto& fMessage : fMessages ) {
      cout << "  " << fMessage.first << ": " << fMessage.second << " times" << endl;
    }
  }
}

//_____________________________________________________________________________
void THaAnalysisObject::PrintObjects( Option_t* opt )
{
  // Print all defined analysis objects (useful for debugging)

  TIter next(fgModules);
  while( TObject* obj = next() ) {
    obj->Print(opt);
  }
}

//_____________________________________________________________________________
TString THaAnalysisObject::GetPrefixName() const
{
  // Get current prefix without the trailing ".", which turns out to be
  // frequently needed task

  TString prefix(GetPrefix());
  if( prefix.EndsWith(".") )
    prefix.Chop();
  return prefix;
}

//_____________________________________________________________________________
ClassImp(THaAnalysisObject)
