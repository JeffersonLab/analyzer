//*-- Author :    Ole Hansen   04-Dec-03

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// UserApparatus                                                        //
//                                                                      //
// Example of a new basic apparatus class. It contains one standard     //
// detector, a UserDetector called "u1".                                //
// It implements a simple Reconstruct() method that simply adds up the  //
// number of hits of all the Scintillator and UserDetectors.            //
// (This is really just an example - not a very useful calculation.)    //
// The result is exported as a global variable.                         //
//                                                                      //
// Apparatuses fall into two broad categories:                          //
// 1) Basic apparatuses, inheriting directly from THaApparatus.         //
//    These usually describe instrumentation that do NOT perform        //
//    tracking of scattered particles. Examples: beamline, calor-       //
//    imeters, luminosity monitors, etc.                                //
//    This may also include apparatus-type objects that don't represent //
//    any physical systems, such as DAQ diagnostic code and similar.    //
// 2) Spectrometer apparatuses, inheriting from THaSpectrometer.        //
//    These describe small or large acceptance "spectrometers", i.e.    //
//    apparatuses that track one or more scattered particles and        //
//    usually measure their momentum. Spectrometers implement signifi-  //
//    cantly more functionality than basic apparatuses. In particular,  //
//    there is a standard Reconstruct() algorithm and a distinction     //
//    between tracking and non-tracking detectors. Also, there are      //
//    additional requirements for detectors to be used with spectrom-   //
//    eters. Examples of spectrometers: HRS, BigBite, HMS, CLAS, OOPS.  //
//    Spectrometers do not need to be magnetic. For example, a          //
//    segmented photon or neutron calorimeter could be implemeted as    //
//    as spectrometer, even if it provides only rather coarse           //
//    particle track information. The resulting "tracks" (4-vectors)    //
//    can then be used directly in kinematics calculations.             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "UserApparatus.h"
#include "THaScintillator.h"
#include "UserDetector.h"
#include "VarDef.h"
#include "TList.h"

using namespace std;

//_____________________________________________________________________________
UserApparatus::UserApparatus( const char* name, const char* description ) :
  THaApparatus(name,description), fNtotal(0)
{
  // Constructor. Defines the _standard_ detectors for this apparatus.
  // These are the detectors that are _always_ present in the apparatus.
  // Optional detectors should be added with calls to AddDetector()
  // in the analysis macro.
  // NB: If in doubt, do not create detectors here.
  // Adding them later from a script costs nothing in terms of analysis
  // performance. Also, hardcoding detectors here means fixing their names,
  // and possibly preventing users from using customized versions of these 
  // detectors (e.g. can't use a UserScintillator if a THaScintillator is
  // hardcoded).

  AddDetector( new UserDetector("u1", "User Detector 1"));

}

//_____________________________________________________________________________
UserApparatus::~UserApparatus()
{
  // Destructor.
  // NB: the base class destructor will delete all the detectors that
  // were added with AddDetector(), so we don't need to worry about 
  // the detectors that we created in the constructor.

  RemoveVariables();
}

//_____________________________________________________________________________
void UserApparatus::Clear( Option_t* opt )
{
  // Clear this apparatus. The standard analyzer calls this function
  // before calling Decode(), so it is guaranteed to be called for
  // every physics event.

  THaApparatus::Clear(opt);
  fNtotal = 0;
}

//_____________________________________________________________________________
Int_t UserApparatus::DefineVariables( EMode mode )
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
Int_t UserApparatus::Reconstruct()
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
    } else if( UserDetector* d = dynamic_cast<UserDetector*>( theDetector )) {
      fNtotal += d->GetNhits();
    } else {
      // do nothing for all other detector types
    }
  }
  return 0;
}

//_____________________________________________________________________________
ClassImp(UserApparatus)
