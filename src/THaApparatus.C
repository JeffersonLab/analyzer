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
#include "TDatime.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

class THaEvData;

ClassImp(THaApparatus)

//_____________________________________________________________________________
THaApparatus::THaApparatus( const char* name, const char* description ) : 
  TNamed(name,description), fDebug(0), fNmydets(0), fMydets(0), 
  fStatus(kNotinit)
{
  // Constructor
  
  fDetectors = new TList;
}

//_____________________________________________________________________________
THaApparatus::~THaApparatus()
{
  // Destructor. Delete all detectors that I have defined.

  delete fDetectors;   //Does not delete any user detectors

  for( int i=0; i<fNmydets; i++ ) {
    delete fMydets[i];
  }
  delete [] fMydets;
}

//_____________________________________________________________________________
Int_t THaApparatus::AddDetector( THaDetector* pdet )
{
  // Add a detector to the internal list.  This method is useful for
  // quick testing of a new detector class that one doesn't want to
  // include permanently in an Apparatus yet.
  // The detector object must be allocated and deleted by the caller.
  // Duplicate detector names are not allowed.

  
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
Int_t THaApparatus::Decode( const THaEvData& evdata )
{
  // Call the Decode() method for all detectors defined for this apparatus.

  TIter next(fDetectors);
  while( THaDetector* theDetector = static_cast<THaDetector*>( next() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Decoding " << theDetector->GetName()
			<< "... " << flush;
#endif
    theDetector->Decode( evdata );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n" << flush;
#endif
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaApparatus::Init()
{
  // Initialize apparatus for current time. See Init(run_time) below.

  TDatime now;
  return Init(now);
}

//_____________________________________________________________________________
Int_t THaApparatus::Init( const TDatime& run_time )
{
  // Default method for initializing an apparatus. Call Init() for all
  // defined detectors in turn. Pass run_time argument.
  // Return kOK if all detectors initialized correctly, or kInitError
  // if any detector failed to initialize. Init() will be called for
  // all detectors, even if one or more detectors fail.
  // 

  static const char* const here = "Init()";
  fStatus = kOK;
  TIter next(fDetectors);
  TObject* obj;

  while( (obj = next()) ) {
    if( !obj->IsA()->InheritsFrom("THaDetector")) {
      Error( here, "Detector %s (\"%s\") is not a THaDetector. "
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
	Error( here, "While initializing apparatus %s (\"%s\") "
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
void THaApparatus::SetDebugAll( Int_t level )
{
  // Set debug level of this apparatus as well as all its detectors

  SetDebug( level );

  TIter next(fDetectors);
  while( THaDetector* theDetector = static_cast<THaDetector*>( next() )) {
    theDetector->SetDebug( level );
  }
}

