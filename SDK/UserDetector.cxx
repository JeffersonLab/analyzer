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
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TMath.h"
#include "TString.h"

// stringstreams are a bit messy due to varying standards compliance 
// of compilers. The following is needed only for ReadDatabase.
#if defined(HAS_SSTREAM) || (defined(__GNUC__)&&(__GNUC__ >= 3))
#include <sstream>
#define ISTR istringstream
#else
#include <strstream>
#define ISTR istrstream
#endif
// end of mess

using namespace std;

//_____________________________________________________________________________
UserDetector::UserDetector( const char* name, const char* description,
				  THaApparatus* apparatus ) :
  THaNonTrackingDetector(name,description,apparatus)
{
  // Constructor
}

//_____________________________________________________________________________
UserDetector::~UserDetector()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
  if( fIsInit )
    DeleteArrays();
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

  static const char* const here = "ReadDatabase";

  // Open the database file
  // NB: OpenFile() is in THaAnalysisObject. It will try to open a database
  // file named db_<fPrefix><fName>.dat (fPrefix is in THaAnalysisObject
  // and is typically the name of the apparatus containing this detector;
  // fName is the name of this object in TNamed).
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // Storage and default values for data from database
  // Note: These must be Double_t
  Double_t nelem = 0.0, angle = 0.0;

  // Set up a table of tags to read and locations to store values.
  // If the optional third element of a line is non-zero, the item is required.
  // This method works for scalar numerical values only. 
  // See below for how to read arrays and strings.
  const TagDef tags[] = { 
    { "nelem", &nelem, 1 },       // Number of elements (e.g. PMTs)
    { "angle", &angle },          // Rotation angle about y (degrees)
    { 0 }                         // Last element must be NULL
  };

  // Read all the tag/value pairs. 
  // NB: LoadDB() is in THaAnalysisObject
  Int_t err = LoadDB( fi, date, tags );

  // Complain and quit if any required values missing.
  if( err ) {    
    if( err>0 )
      Error( Here(here), "Required tag %s%s missing in the "
	     "database.\nDetector initialization failed.",
	     fPrefix, tags[err-1].name );
    fclose(fi);
    return kInitError;
  }
  
  // Check nelem for sanity
  if( nelem <= 0.0 ) {
    Error( Here(here), "Cannot have zero or negative number of elements. "
	   "Detector initialization failed." );
    fclose(fi);
    return kInitError;
  }
  // If the detector is already initialized, the number of elements must not
  // change. This prevents problems with global variables that
  // point to memory that is dynamically allocated.
  // Resizing a detector is hardly ever necessary for a given analysis,
  // so this catches database errors and simplifies the design of this class.
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of paddles. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    fclose(fi);
    return kInitError;
  }

  // All ok - store nelem
  fNelem = int(nelem);
  
  // Use the rotation angle to set the axes vectors 
  // (required for display functions, for instance)
  // NB: DefineAxes is in THaSpectrometerDetector
  const Float_t degrad = TMath::Pi()/180.0;
  DefineAxes(angle*degrad);

  // Read arrays (e.g. calibration data). 
  // First, allocate memory (if not already done)
  if( !fIsInit ) {
    // Calibrations
    fPed   = new Double_t[ fNelem ];
    fGain  = new Double_t[ fNelem ];

    // Per-event data
    fRawADC = new Double_t[ fNelem ];
    fCorADC = new Double_t[ fNelem ];

    fIsInit = true;
  }

  // To read arrays, we need to read the data as text, then scan the text.
  // This is kludgy; in the future, there will be full support for arrays.
  TString line, tag;
  Int_t retval = kOK;
  int n;

  // Now read the array data.
  // We define a structure with the list of the arrays we want to read.
  // This looks complicated, but allows for easy expansion.
  struct ArrayInputDef_t {
    const char* tag;
    Double_t*   dest;
    Int_t       nval;
  };
  const ArrayInputDef_t adef[] = {
    { "pedestals", fPed,  fNelem },
    { "gains"    , fGain, fNelem },
    { 0 }
  };
  const ArrayInputDef_t* item = adef;
  while( item->tag ) {
    tag = item->tag;
    // NB: LoadDBvalue is in THaAnalysisObject
    if( LoadDBvalue( fi, date, tag, line ) != 0 ) 
      goto bad_data;
    ISTR inp(line.Data());
    for(int i=0; i<item->nval; i++) {
      inp >> item->dest[i];
      if( !inp ) 
	goto bad_data;
    }
    item++;
  }

  // Read detector map
  // Multiple lines are allowed. The format is:
  //  detmap_1  <crate> <slot> <first channel> <last channel> <first logical ch>
  //  detmap_2   dto.
  //  etc.
  // The "first logical channel" is the one assigned to the 
  // first hardware channel, as usual.
  fDetMap->Clear();
  n = 1;
  while(1) {
    Int_t crate, slot, lo, hi, first;
    tag = Form("detmap_%d", n);
    if( LoadDBvalue( fi, date, tag, line ) != 0 )
      break;
    ISTR inp(line.Data());
    inp >> crate >> slot >> lo >> hi >> first;
    if( !inp )
      goto bad_data;
    if( fDetMap->AddModule( crate, slot, lo, hi, first ) < 0 )
      goto bad_data;
    n++;
  }
  if(n>1)
    goto done;

 bad_data:    
  Error( Here(here), "Problem reading database entry \"%s\". "
	 "Detector initialization failed.", tag.Data() );
  retval = kInitError;

 done:
  fclose(fi);
  return retval;
}

//_____________________________________________________________________________
Int_t UserDetector::DefineVariables( EMode mode )
{
  // Define (or delete) global variables of the detector

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nhit",  "Number of hits",  "fNhit" },
    { "adc",   "Raw ADC values",  "fRawADC" },
    { "adc_c", "Corrected ADCs",  "fCorADC" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void UserDetector::DeleteArrays()
{
  // Delete dynamically allocated arrays. Used by destructor.

  delete [] fRawADC; fRawADC = NULL;
  delete [] fCorADC; fCorADC = NULL;
}

//_____________________________________________________________________________
void UserDetector::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaNonTrackingDetector::Clear(opt);

  const int asiz = fNelem*sizeof(Double_t);
  fNhit = 0;
  memset( fRawADC, 0, asiz );
  memset( fCorADC, 0, asiz );
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

  // Clear event-by-event data
  Clear();

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

#ifdef WITH_DEBUG      
      if( k<0 || k>=fNelem ) {
	// Indicates bad database
	Warning( Here("Decode()"), "Illegal detector channel: %d", k );
	continue;
      }
#endif
      fRawADC[k] = static_cast<Double_t>( data );
      fCorADC[k] = (fRawADC[k] - fPed[k]) * fGain[k];
      fNhit++;
    }
  }
  return fNhit;
}

//_____________________________________________________________________________
Int_t UserDetector::CoarseProcess( TClonesArray& tracks )
{
  // Coarse processing. 'tracks' contains coarse tracks.

  return 0;
}

//_____________________________________________________________________________
Int_t UserDetector::FineProcess( TClonesArray& tracks )
{
  // Fine processing. 'tracks' contains final tracking results.

  return 0;
}


////////////////////////////////////////////////////////////////////////////////

ClassImp(UserDetector)
