//*-- Author :    Ole Hansen   04-Dec-03

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// FbusApparatus                                                    //
//                                                                      //
//   Fastbus Test Stand                                                 // 
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "FbusApparatus.h"
#include "THaScintillator.h"
#include "FbusDetector.h"
#include "VarDef.h"
#include "TList.h"

using namespace std;

//_____________________________________________________________________________
FbusApparatus::FbusApparatus( const char* name, const char* description ) : 
  THaApparatus(name,description)
{
  // Constructor. Defines the _standard_ detectors for this apparatus.
  // These are the detectors that are _always_ present in the apparatus.
  // Optional detectors should be added with calls to AddDetector()
  // in the analysis macro. 

  AddDetector( new FbusDetector("fbus", "Fbus Detector"));

}

//_____________________________________________________________________________
FbusApparatus::~FbusApparatus()
{
  // Destructor.
  // NB: the base class destructor will delete all the detectors that
  // were added with AddDetector(), so we don't need to worry about 
  // the detectors that we created in the constructor.

  RemoveVariables();
}

//_____________________________________________________________________________
void FbusApparatus::Clear( Option_t* opt )
{
  // Clear this apparatus. The standard analyzer calls this function
  // before calling Decode(), so it is guaranteed to be called for
  // every physics event.

  fNtotal = 0;
}

//_____________________________________________________________________________
Int_t FbusApparatus::DefineVariables( EMode mode )
{
  // Define/delete the global variables for this apparatus.
  // Typically these are results computed in Reconstruct().

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "ntot", "Total number of hits", "fNtotal" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t FbusApparatus::Reconstruct()
{
  // "Reconstruct" the apparatus. Here any processing should be done
  // that requires that data from several detectors be combined.
  //
  // As an example, we calculate the total number of hits of
  // all the detectors. This illustrates how to handle detectors that
  // do not share a common API.

  TIter next(fDetectors);
  while( TObject* theDetector = next()) {
    // NB: dynamic_cast returns NULL if the cast object does not 
    // inherit from the requested type. 
    // One could also use TClass::InheritsFrom, but dynamic_cast is faster.
    if( THaScintillator* d = dynamic_cast<THaScintillator*>( theDetector )) {
      fNtotal += d->GetNHits();
    } else if( FbusDetector* d = dynamic_cast<FbusDetector*>( theDetector )) {
      fNtotal += d->GetNhits();
    } else {
      // do nothing for all other detector types
    }
  }
  return 0;
}

//_____________________________________________________________________________
ClassImp(FbusApparatus)
