//*-- Author :    Ole Hansen   13-May-00

//////////////////////////////////////////////////////////////////////////
//
// THaApparatus
//
// ABC for a generic apparatus (collection of detectors).
// Defines a standard Decode() method that loops over all detectors 
// defined for this object.
//
// A Reconstruct() method has to be implemented by the derived classes.
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "THaDetector.h"
#include "TClass.h"
#include "TList.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

using namespace std;

//_____________________________________________________________________________
THaApparatus::THaApparatus( const char* name, const char* description ) : 
  THaAnalysisObject(name,description)
{
  // Constructor
  
  fDetectors = new TList;
}

//_____________________________________________________________________________
THaApparatus::THaApparatus( ) : fDetectors(NULL)
{
  // only for ROOT I/O
}
  
//_____________________________________________________________________________
THaApparatus::~THaApparatus()
{
  // Destructor. Delete all detectors currently associated with this
  // apparatus, including any that were added via AddDetector.

  if (fDetectors) {
    fDetectors->Delete();
    delete fDetectors; fDetectors=0;
  }
}

//_____________________________________________________________________________
Int_t THaApparatus::AddDetector( THaDetector* pdet )
{
  // Add a detector to this apparatus. This is the standard way to 
  // configure an apparatus for data analysis.
  //
  // The name of each detector is important as it defines the names
  // of all related global variables, output variables, cuts, database
  // file names and entries, etc. Consequently, duplicate detector 
  // names are not allowed. Note that some apparatuses require 
  // specific names for certain standard detectors, e.g. "s1" for the
  // first scintillator plane in the HRS, etc.
  //
  // The detector object must be allocated by the caller, but will be
  // deleted by the apparatus.

  
  THaDetector* pfound = 
    static_cast<THaDetector*>( fDetectors->FindObject( pdet->GetName() ));
  if( pfound ) {
    Error("THaApparatus", "Detector with name %s already exists for this"
	  " apparatus. Not added.", pdet->GetName() );
    return -1;
  }
  pdet->SetApparatus(this);
  fDetectors->AddLast( pdet );
  return 0;
}

//_____________________________________________________________________________
Int_t THaApparatus::Begin( THaRunBase* run )
{
  // Default Begin() for an apparatus: Begin() all our detectors

  TIter next(fDetectors);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(next()) ) {
    obj->Begin(run);
  }
  return 0;
}

//_____________________________________________________________________________
void THaApparatus::Clear( Option_t* opt )
{
  // Call the Clear() method for all detectors defined for this apparatus.

  TIter next(fDetectors);
  while( THaDetector* theDetector = static_cast<THaDetector*>( next() )) {
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Clearing " << theDetector->GetName()
			<< "... " << flush;
#endif
    theDetector->Clear(opt);
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n" << flush;
#endif
  }
}

//_____________________________________________________________________________
Int_t THaApparatus::Decode( const THaEvData& evdata )
{
  // Call the Decode() method for all detectors defined for this apparatus.

  TIter next(fDetectors);
  while( THaDetector* theDetector = static_cast<THaDetector*>( next() )) {
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Decoding " << theDetector->GetName()
			<< "... " << flush;
#endif
    theDetector->Decode( evdata );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n" << flush;
#endif
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaApparatus::End( THaRunBase* run )
{
  // Default End() for an apparatus: End() all our detectors

  TIter next(fDetectors);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(next()) ) {
    obj->End(run);
  }
  return 0;
}

//_____________________________________________________________________________
THaDetector* THaApparatus::GetDetector( const char* name )
{
  // Find the named detector and return a pointer to it. 
  // This is useful for specialized processing.

  return static_cast<THaDetector*>( fDetectors->FindObject( name ));
}

//_____________________________________________________________________________
Int_t THaApparatus::GetNumDets() const
{ 
  // Return number of detectors of the apparatus

  return fDetectors->GetSize();
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaApparatus::Init( const TDatime& run_time )
{
  // Default method for initializing an apparatus. 
  // First, call default THaAnalysisObject::Init(), which
  //  - Creates the fPrefix for this apparatus.
  //  - Opens database file and reads database for this apparatus, if custom 
  //    ReadDatabase() defined.
  //  - Sets up global variables for this apparatus via DefineVariables().
  // Then, call Init() for all defined detectors in turn. Pass run_time argument.
  // Return kOK if all detectors initialized correctly, or kInitError
  // if any detector failed to initialize. Init() will be called for
  // all detectors, even if one or more detectors fail.

  if( THaAnalysisObject::Init( run_time ) )
    return fStatus;

  TIter next(fDetectors);
  TObject* obj;

  while( (obj = next()) ) {
    if( !obj->IsA()->InheritsFrom("THaDetector")) {
      Error( Here("Init()"), "Detector %s (\"%s\") is not a THaDetector. "
	     "Initialization of apparatus %s (\"%s\") failed.", 
	     obj->GetName(), obj->GetTitle(), GetName(), GetTitle());
      fStatus = kInitError;
    } else {
      THaDetector* theDetector = static_cast<THaDetector*>( obj );
#ifdef WITH_DEBUG
      if( fDebug>0 ) cout << "Initializing " 
			  << theDetector->GetName() << "... "
			  << flush;
#endif
      theDetector->Init( run_time );
#ifdef WITH_DEBUG
      if( fDebug>0 ) cout << "done.\n" << flush;
#endif
      if( !theDetector->IsOK() ) {
	Error( Here("Init()"), "While initializing apparatus %s (\"%s\") "
	      "got error %d from detector %s (\"%s\")", 
	      GetName(), GetTitle(), theDetector->Status(),
	      theDetector->GetName(), theDetector->GetTitle());
	fStatus = kInitError;
      }
    }
  }

  return fStatus;
}

//_____________________________________________________________________________
void THaApparatus::Print( Option_t* opt ) const
{ 
  // Print info about the apparatus and all its detectors

  THaAnalysisObject::Print(opt);
  fDetectors->Print(opt);
}

//_____________________________________________________________________________
void THaApparatus::SetDebugAll( Int_t level )
{
  // Set debug level of this apparatus as well as all its detectors

  SetDebug( level );

  TIter next(fDetectors);
  while( THaDetector* theDetector = static_cast<THaDetector*>( next() )) {
    theDetector->SetDebug( level );
  }
}

//_____________________________________________________________________________
ClassImp(THaApparatus)
