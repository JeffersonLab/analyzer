//*-- Author :    Ole Hansen   01-Dec-03
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserDetector                                                              //
//                                                                           //
// Example of a user-defined detector class. This class implements a         //
// completely new detector from scratch. Only Decode() performs non-trivial  //
// work. CoarseProcess/FineProcess should be implemented as needed.          //
//                                                                           //
// See UserScintillator for an example of a detector class where the         //
// behavior of an existing detector (scintillator) is modified and extended. //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UserDetector.h"
#include "VarDef.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "TMath.h"
#include "Helper.h"
// Needed in CoarseProcess/FineProcess if implemented
#include "THaTrack.h"
#include "TClonesArray.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Podd;

// Constructors should initialize all basic-type member variables to safe
// default values. In particular, all pointers should be either zeroed or
// assigned valid data.
//_____________________________________________________________________________
UserDetector::UserDetector( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus)
{
  // Constructor
}

//_____________________________________________________________________________
UserDetector::~UserDetector()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t UserDetector::ReadDatabase( const TDatime& date )
{
  // Read the database for this detector.
  // This function is called once at the beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.
  //
  // This routine is written for the simple file-based database scheme
  // of Analyzer 1.x. The format of the database file is completely up to the
  // author of the detector class. Here, we use tag/value pairs to allow
  // for a free file format.

  const char* const here = "ReadDatabase";

  // Open the database file
  // NB: OpenFile() is in THaAnalysisObject. It will try to open a database
  // file named db_<fPrefix><fName>.dat (fPrefix is in THaAnalysisObject
  // and is typically the name of the apparatus containing this detector;
  // fName is the name of this object in TNamed).
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Storage and default values for data from database. To prevent memory
  // leaks when re-initializing, all dynamically allocated memory should
  // be deleted here, and the corresponding variables should be zeroed.
  // All vectors should be cleared unless they are required database
  // parameters. (If an optional parameter is not found in the database,
  // its value remains unchanged.)
  Int_t nelem = 0;
  Double_t angle = 0.0;
  fPed.clear();
  fGain.clear();

  // The detector map.
  // This is a matrix of numerical values. Multiple lines are allowed.
  // The format is:
  //  detmap  <crate1> <slot1> <first channel1> <last channel1> <first logical ch1>
  //          <crate2> <slot2> <first channel2> <last channel2> <first logical ch2>
  //  etc.
  // The "first logical channel" is the one assigned to the first hardware channel.
  vector<Int_t> detmap;

  // Read the database. This may throw exceptions, so we put it in a try block.
  Int_t err = 0;
  try {
    // Set up an array of database requests. See VarDef.h for details.
    const DBRequest request[] = {
      // Required items
      { "detmap",    &detmap, kIntV },         // Detector map
      { "nelem",     &nelem,  kInt, 0, 0, -1}, // Number of elements (e.g. PMTs)
      // Optional items
      { "angle",     &angle,  kDouble, 0, 1 }, // Rotation angle about y (deg)
      //== CAUTION: kFloatV here must match the type of Data_t defined in the header!
      { "pedestals", &fPed,   kFloatV, 0, 1 }, // Pedestals
      { "gains",     &fGain,  kFloatV, 0, 1 }, // Gains
      { 0 }                                    // Last element must be NULL
    };

    // Read the requested values
    err = LoadDB( file, date, request );

    // If no error, parse the detector map. See THaDetMap::Fill for details.
    if( err == kOK ) {
      if( FillDetMap( detmap, THaDetMap::kFillLogicalChannel, here ) <= 0 ) {
	err = kInitError;
      }
    }
  }
  // Catch exceptions here so that we can close the file and clean up
  catch(...) {
    fclose(file);
    throw;
  }

  // Normal end of reading the database
  fclose(file);
  if( err != kOK )
    return err;

  // Check all configuration values for sanity. This is the more tedious, but
  // nevertheless very important part of reading the database. In addition to
  // simple checking, this part may also include computation of derived
  // configuration parameters, for instance an allowed hitpattern or
  // geometry limits. See the call to DefineAxes below as an example.
  if( nelem <= 0 ) {
    Error( Here(here), "Cannot have zero or negative number of elements. "
	   "Fix database." );
    return kInitError;
  }
  if( nelem > 100 ) {
    Error( Here(here), "Illegal number of elements = %d. Must be <= 100. "
	   "Fix database.", nelem );
    return kInitError;
  }

  // If the detector is already initialized, the number of elements must not
  // change. This prevents problems with global variables that
  // point to memory that is dynamically allocated.
  // Resizing a detector is hardly ever necessary for a given analysis,
  // so this catches database errors and simplifies the design of this class.
  if( fIsInit && nelem != fNelem ) {
    ostringstream ostr;
    ostr << "Cannot re-initalize with different number of elements. "
         << "(was: " << fNelem << ", now: " << nelem << "). "
         << "Detector not re-initialized.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }

  // All ok - store nelem
  fNelem = nelem;

  // Check detector map size. We assume that each detector element (e.g. paddle)
  // has exactly one hardware channel. This could be different, e.g. if each PMT
  // is read by an ADC and a TDC, etc. Modify as appropriate.
  Int_t nchan = fDetMap->GetTotNumChan();
  if( nchan != nelem ) {
    ostringstream ostr;
    ostr << "Incorrect number of detector map channels = " << nchan
         << ". Must equal nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }

  // Check if the sizes of the arrays we read make sense.
  // NB: If we have an empty vector here, there was no database entry for it.
  if( !fPed.empty() && fPed.size() != static_cast<Vec_t::size_type>(nelem)) {
    ostringstream ostr;
    ostr << "Incorrect number of pedestal values = " << fPed.size()
         << ". Must match nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }
  if( !fGain.empty() && fGain.size() != static_cast<Vec_t::size_type>(nelem)) {
    ostringstream ostr;
    ostr << "Incorrect number of gain values = " << fGain.size()
         << ". Must match nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }

  // If any of the arrays were not given in the database, set them to defaults:
  // Pedestals default to zero, gains to 1.0.
  if( fPed.empty() )
    fPed.assign( nelem, 0.0f );
  if( fGain.empty() )
    fGain.assign( nelem, 1.0f );

  // Use the rotation angle to set the axes vectors
  // (required for display functions, for instance)
  // See THaDetectorBase::DefineAxes for details.
  const Double_t degrad = TMath::Pi()/180.0;
  DefineAxes(angle*degrad);

  fIsInit = true;

  return kOK;
}

//_____________________________________________________________________________
Int_t UserDetector::DefineVariables( EMode mode )
{
  // Define (or delete) global variables of the detector

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nhit",  "Number of hits",  "GetNhits()" },
    { "chan",  "Raw ADC values",  "fEvtData.fChannel" },
    { "adc",   "Raw ADC values",  "fEvtData.fRawADC" },
    { "adc_c", "Corrected ADCs",  "fEvtData.fCorADC" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void UserDetector::Clear( Option_t* opt )
{
  // Reset per-event data. This function is called before Decode() for
  // every event.

  THaNonTrackingDetector::Clear(opt);
  fEvtData.clear();
}

//_____________________________________________________________________________
Int_t UserDetector::Decode( const THaEvData& evdata )
{
  // Decode data.
  // Read all channels with hits specified in  the detector map.
  // Assume single-hit hardware, i.e. only one hit per channel.
  // Store results in fRawADC array. Compute corrected ADC values
  // (pedestal-subtracted, scaled by gain factor) here as well since
  // all the required data is available.

  // Loop over all modules defined in the detector map
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;   // Not one of my channels

      // Get the data. This detector is assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = chan - d->lo + d->first;

      // Always ensure that index values are sane
      if( k<0 || k>=fNelem ) {
#ifdef WITH_DEBUG
	// Indicates bad database
	Warning( Here("Decode()"), "Illegal detector channel: %d", k );
#endif
	continue;
      }
      fEvtData.push_back( EventData(k,data,(data-fPed[k])*fGain[k]) );
    }
  }
  return fEvtData.size();
}

//_____________________________________________________________________________
Int_t UserDetector::CoarseProcess( TClonesArray& /* tracks */ )
{
  // Coarse processing. 'tracks' contains coarse tracks.

  return 0;
}

//_____________________________________________________________________________
Int_t UserDetector::FineProcess( TClonesArray& /* tracks */ )
{
  // Fine processing. 'tracks' contains final tracking results.

  return 0;
}

//_____________________________________________________________________________
// Helper macro to print a single field of a structure in a std::vector.
// Could be done more elegantly in C++11/14
#define PrintArrayField(txt,var,field)          \
  cout << txt << " = ";                         \
  {                                             \
     size_t size = var.size();                  \
     if( size == 0 )                            \
       cout << "(empty)";                       \
     else {                                     \
       for( size_t i = 0; i < size; ++i ) {     \
         cout << var[i].field;                  \
         if( i+1 != size ) cout << ", ";        \
       }                                        \
     }                                          \
  } cout << endl;

//_____________________________________________________________________________
void UserDetector::Print( Option_t* opt ) const
{
  // Print current configuration

  // It is always a good idea to have a Print method, if only for debugging

  THaDetector::Print(opt);
  cout << "detmap = "; fDetMap->Print();
  cout << "nelem = " << fNelem << endl;
  cout << "pedestals = "; PrintArray( fPed );
  cout << "gains = ";     PrintArray( fGain );
  cout << "nhits = " << fEvtData.size() << endl;
  PrintArrayField("channel",fEvtData,fChannel);
  PrintArrayField("rawadc",fEvtData,fRawADC);
  PrintArrayField("coradc",fEvtData,fCorADC);
}

////////////////////////////////////////////////////////////////////////////////

ClassImp(UserDetector)
