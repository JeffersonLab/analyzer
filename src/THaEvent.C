//*-- Author :    Ole Hansen     17/05/2000

////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
// This is the base class for "event" objects, which define the 
// structure of the event data to be written to the output DST 
// via ROOT object I/O.  
//
// Any derived class must implement at least the following:
// 
//  - Define all member variables that are to appear in the output
//    for each event (a la ntuple).
//  - Set up the fDataMap array to link these member variables to 
//    global analyzer variables.
//
// Data members can be simple types (Double_t etc.) and any ROOT objects
// capable of object I/O (i.e. with a valid Streamer() class).
// For any data members other than simple types defined in fDataMap,
// the Fill() method has to be extended accordingly.
//
////////////////////////////////////////////////////////////////////////

#include "THaEvent.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TROOT.h"
#include "TMath.h"
#include "TClass.h"

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

ClassImp(THaEventHeader)
ClassImp(THaEvent)

//_____________________________________________________________________________
THaEvent::THaEvent() : fInit(kFALSE), fDataMap(NULL)
{
  // Create a THaEvent object.
  Class()->IgnoreTObjectStreamer();
}

//_____________________________________________________________________________
THaEvent::~THaEvent()
{
  // Destructor. Clean up all my objects.

  delete [] fDataMap; fDataMap = NULL;
}

//_____________________________________________________________________________
void THaEvent::Clear( Option_t* )
{
  // Reset
}

//_____________________________________________________________________________
Int_t THaEvent::Fill()
{
  // Copy global variables specified in the data map to the event structure.

  static const char* const here = "Fill()";

  // Initialize datamap if not yet done
  if( !fInit ) {
    Int_t status = Init();
    if( status )
      return status;
  }

  // Move the data into the members variables of this Event object
  //
  // For basic types, this can be reduced to a memcpy. Knowledge of the
  // type of the destination is not necessary, BUT the user needs to ensure
  // the destination type matches the source type -> potential for error!
  //
  // For object types, this is no longer that easy (without rewriting the
  // variable system); we really have to convert the data retrieved via
  // GetData() to the type of the destination.
  //
  // These complications will go away completely once this class is rewritten
  // with an intelligent streamer.
  //

  Int_t nvar = 0;
  Int_t ncopy;
  THaVar* pvar;
  if( DataMap* datamap = fDataMap ) {
    while( datamap->ncopy ) {
      if( (pvar = datamap->pvar) ) {
	if( datamap->ncopy>0 )
	  ncopy = datamap->ncopy;
	else if( datamap->ncopyvar )
	  ncopy = *datamap->ncopyvar;
	else {
	  ncopy = pvar->GetLen();
	  if( ncopy == THaVar::kInvalidInt ) 
	    continue;
	}
	Int_t       type = pvar->GetType();
	size_t      size = pvar->GetTypeSize();
	const void* src  = pvar->GetValuePointer();
	if( !src ) continue;
	if( pvar->IsBasic() ) {
	  if( type >= kDoubleP ) {
	    // Pointers to pointers - get the pointer they currently point to
	    void** loc = (void**)src;
	           src = (const void*)(*loc);
	  }
	  if( !src ) continue;
	  if( !pvar->IsPointerArray() )
	    memcpy( datamap->dest, src, ncopy*size );
	  else {
	    // For pointer arrays, we need to copy the elements one by one
	    // Type doesn't matter for memcpy, but size does ;)
	    for( int i=0; i<ncopy; i++ ) {
	      const int** psrc = (const int**)src;
	      int*        dest = static_cast<int*>( datamap->dest );
	      memcpy( dest+i, *(psrc+i), size );
	    }
	  }
	} else {
	  // Object variables: Now we have to worry about the type of
	  // the destination. We assume it is the same type as the
	  // source. Ugly and inefficient, but good enough for now I guess.
	  // Note: Function return values are either Long_t or Double_t,
	  // regardless of the actual return value of the function.
	  // This is a ROOT limitation.
	  for( int i=0, j=0; i<ncopy; i++, j++ ) {
	    Double_t val = pvar->GetValue(i);
	    if( val == THaVar::kInvalid )
	      continue;
	    switch( type ) {
	    case kDouble: case kDoubleP:
	      *(((Double_t*)datamap->dest)+j) = val;
	      break;
	    case kFloat:  case kFloatP:
	      *(((Float_t*)datamap->dest)+j)  = static_cast<Float_t>(val);
	      break;
	    case kInt:    case kIntP:
	      *(((Int_t*)datamap->dest)+j)    = static_cast<Int_t>(val);
	      break;
	    case kUInt:   case kUIntP:
	      *(((UInt_t*)datamap->dest)+j)   = static_cast<UInt_t>(val);
	      break;
	    case kShort:  case kShortP:
	      *(((Short_t*)datamap->dest)+j)  = static_cast<Short_t>(val);
	      break;
	    case kUShort: case kUShortP:
	      *(((UShort_t*)datamap->dest)+j) = static_cast<UShort_t>(val);
	      break;
	    case kLong:   case kLongP:
	      *(((Long64_t*)datamap->dest)+j) = static_cast<Long64_t>(val);
	      break;
	    case kULong:  case kULongP:
	      *(((ULong64_t*)datamap->dest)+j)= static_cast<ULong64_t>(val);
	      break;
	    case kChar:   case kCharP:
	      *(((Char_t*)datamap->dest)+j)   = static_cast<Char_t>(val);
	      break;
	    case kByte:   case kByteP:
	      *(((Byte_t*)datamap->dest)+j)   = static_cast<Byte_t>(val);
	      break;
	    default:
	      Warning( here, "Unknown type for variable %s "
		 "Not filled.", pvar->GetName() );
	      j--; nvar--;
	      break;
	    }
	  }
	}
	nvar += ncopy;
      }
      datamap++;
    }
  }

  return nvar;
}

//_____________________________________________________________________________
Int_t THaEvent::Init()
{
  // Initialize fDataMap. Called automatically by Fill() as necessary.

  if( !gHaVars ) return -2;

  if( DataMap* datamap = fDataMap ) {
    while( datamap->ncopy ) {
      if( THaVar* pvar = gHaVars->Find( datamap->name )) {
	datamap->pvar = pvar;
      } else {
	Warning("Init()", "Global variable %s not found. "
		"Will be filled with zero.", datamap->name );
	datamap->pvar = NULL;
      }
      datamap++;
    }
  }
  fInit = kTRUE;
  return 0;
}

//_____________________________________________________________________________
void THaEvent::Reset( Option_t* )
{
  // Reset pointers to global variables. Forces Init() to be executed
  // at next Fill(), which re-associates global variables with the
  // event structure and histograms.

  fInit = kFALSE;
}


