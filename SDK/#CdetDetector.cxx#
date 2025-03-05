//*-- Author :    Bob Michaels, Jan 2017
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Cdet Detector							     //
//                                                                           //
// Example of a user-defined detector class. This class implements a         //
// completely new detector from scratch. Only Decode() performs non-trivial  //
// work. CoarseProcess/FineProcess should be implemented as needed.          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#define LEADING_EDGE 0
#define TRAILING_EDGE 1
#define EDGEMASK 0x10000
#define EDGESHIFT 16
#define TDC_UPPER_LIMIT 1000
#define TDC_LOWER_LIMIT 800

#include "CdetDetector.h"
#include "VarDef.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "TMath.h"
// Needed in CoarseProcess/FineProcess if implemented
#include "THaTrack.h"
#include "TClonesArray.h"

#include <cassert>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace std;

// Constructors should initialize all basic-type member variables to safe
// default values. In particular, all pointers should be either zeroed or
// assigned valid data.
//_____________________________________________________________________________
CdetDetector::CdetDetector( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus),
    fOff(0), fPed(0), fGain(0), // Database values
    fTNhit(0), fRawLTDC(0), fRawTTDC(0), fCorLTDC(0), fCorTTDC(0),fANhit(0), fRawADC(0), fCorADC(0), // Data
    fNhit(0) // Other calculated quantities 
{
  // Constructor
}

//_____________________________________________________________________________
CdetDetector::CdetDetector()
  : THaNonTrackingDetector(),
    fOff(0), fPed(0), fGain(0), fAdcMIP(0), // Database values 
    fRawLTDC(0), fRawTTDC(0), fCorLTDC(0), fCorTTDC(0), fRawADC(0), fCorADC(0) // Data
{
  // Default Constructor (for ROOT I/O)
}

THaAnalysisObject::EStatus CdetDetector::Init( const TDatime& date )
{
  // Extra initialization for scintillators: set up DataDest map
  
  if( THaNonTrackingDetector::Init( date ) )
    return fStatus;
  
  const DataDest tmp[NDEST] = {
    { &fTNhit, fRawLTDC, fRawTTDC, fCorLTDC, fCorTTDC, &fANhit, fRawADC, fCorADC, fOff, fPed, fGain }
  };
  memcpy( fDataDest, tmp, NDEST*sizeof(DataDest) );
  
  return fStatus = kOK;
}
  
//_____________________________________________________________________________
CdetDetector::~CdetDetector()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
  delete [] fRawLTDC;
  delete [] fRawTTDC;
  delete [] fCorLTDC; 
  delete [] fCorTTDC; 
  delete [] fRawADC;
  delete [] fCorADC;

  delete [] fOff;
  delete [] fPed;
  delete [] fGain;

}

//_____________________________________________________________________________
Int_t CdetDetector::ReadDatabase( const TDatime& date )
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
  
  //fPed.clear();
  //fGain.clear();

  // The detector map.
  // This is a matrix of numerical values. Multiple lines are allowed.
  // The format is:
  //  detmap  <crate1> <slot1> <first channel1> <last channel1> <first logical ch1>
  //          <crate2> <slot2> <first channel2> <last channel2> <first logical ch2>
  //  etc.
  // The "first logical channel" is the one assigned to the first hardware channel.
  vector<Int_t> detmap;
  Int_t nelem = 0;
  Double_t angle = 0.0;

  // Read the database. This may throw exceptions, so we put it in a try block.
  Int_t err = 0;
  try {
    // Set up an array of database requests. See VarDef.h for details.
    const DBRequest request[] = {
      // Required items
      { "detmap",    &detmap, kIntV },         // Detector map
      { "nelem",     &nelem,  kInt },          // Number of elements (e.g. PMTs)
      { "angle",     &angle,  kDouble, 0, 1 }, // Rotation angle about y (deg)
      { 0 }                                    // Last element must be NULL
    };

    // Read the requested values
    err = LoadDB( file, date, request );

    // Sanity checks
    if( !err && nelem <= 0 ) {
      Error( Here(here), "Invalid number of paddles: %d", nelem );
      err = kInitError;
    }

    // Reinitialization only possible for same basic configuration
    if( !err ) {
      if( fIsInit && nelem != fNelem ) {
        Error( Here(here), "Cannot re-initalize with different number of paddles. "
               "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
        err = kInitError;
      } else
        fNelem = nelem;
    }

    // If no error, parse the detector map. See THaDetMap::Fill for details.
    if( err == kOK ) {
      if( FillDetMap( detmap, THaDetMap::kFillLogicalChannel, here ) <= 0 ) {
	err = kInitError;
      }
    }

    if( !err && (nelem = fDetMap->GetTotNumChan()) != 2*fNelem ) {
      Error( Here(here), "Number of detector map channels (%d) "
             "inconsistent with 2*number of paddles (%d)", nelem, 2*fNelem );
      err = kInitError;
    }

  }
  // Catch exceptions here so that we can close the file and clean up
  catch(...) {
    fclose(file);
    throw;
  }

  DefineAxes( angle*TMath::DegToRad() );

  UInt_t nval = fNelem;
  if( !fIsInit ) {

  	fOff = new Double_t[ nval ];
  	fPed = new Double_t[ nval ];
  	fGain = new Double_t[ nval ];

  	fRawLTDC = new Double_t [nval];
  	fRawTTDC = new Double_t [nval];
  	fCorLTDC = new Double_t [nval];
  	fCorTTDC = new Double_t [nval];
  	fRawADC = new Double_t [nval];
  	fCorADC = new Double_t [nval];

	fIsInit = true;
  }

  // Set DEFAULT values here
  // TDC resolution (s/channel)
  fTdc2T = 0.1e-9; // second/channel
  // Speed of light in the scintillator material
  fCn = 1.7e+8; // meters/second
  // Time-walk correction parameters
  fAdcMIP = 1.e10;

  memset( fOff, 0, nval*sizeof(fOff[0]) );
  memset( fPed, 0, nval*sizeof(fPed[0]) );
  for( UInt_t i=0; i<nval; ++i ) { fGain[i] = 1.0; }

  DBRequest calib_request[] = {
    { "off",            fOff,         kDouble, nval, 1 },
    { "ped",            fPed,         kDouble, nval, 1 },
    { "gain",           fGain,        kDouble, nval, 1 },
    { "tdc.res",          &fTdc2T,       kDouble },
    { "Cn",               &fCn,          kDouble },
    { "MIP",              &fAdcMIP,      kDouble, 0, 1 },
    { 0 }
  };
  err = LoadDB( file, date, calib_request, fPrefix );

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
  if( nelem > 448 ) {
    Error( Here(here), "Illegal number of elements = %d. Must be <= 448. "
	   "Fix database.", nelem );
    return kInitError;
  }

  // If the detector is already initialized, the number of elements must not
  // change. This prevents problems with global variables that
  // point to memory that is dynamically allocated.
  // Resizing a detector is hardly ever necessary for a given analysis,
  // so this catches database errors and simplifies the design of this class.
  //if( fIsInit && nelem != fNelem ) {
  //  Error( Here(here), "Cannot re-initalize with different number of elements. "
  //	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
  //  return kInitError;
  //}

  // All ok - store nelem
  //fNelem = nelem;

  // Check detector map size. We assume that each detector element (e.g. paddle)
  // has exactly one hardware channel. This could be different, e.g. if each PMT
  // is read by an ADC and a TDC, etc. Modify as appropriate.
  Int_t nchan = fDetMap->GetTotNumChan();
  if( nchan != nelem ) {
    Error( Here(here), "Incorrect number of detector map channels = %d. Must "
	   "equal nelem = %d. Fix database.", nchan, nelem );
    return kInitError;
  }

  if ( fDebug > -1 ) {
    cout << '\n' << GetPrefix() << " calibration parameters: " << endl;;
    for ( DBRequest *li = calib_request; li->name; li++ ) {
      cout << "  " << li->name;
      UInt_t maxc = li->nelem;
      if (maxc==0)maxc=1;
      for (UInt_t i=0; i<maxc; i++) {
        if (li->type==kDouble) cout << "  " << ((Double_t*)li->var)[i];
        if (li->type==kInt) cout << "  " << ((Int_t*)li->var)[i];
      }
      cout << endl;
    }
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t CdetDetector::DefineVariables( EMode mode )
{
  // Define (or delete) global variables of the detector

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nthit", "Number of paddles TDC times",  "fTNhit" },
    { "nahit", "Number of paddles ADCs amps",  "fANhit" },
    { "tdcl",     "Leading Edge TDC values",              "fRawLTDC" },
    { "tdct",     "Trailing Edge TDC values",              "fRawTTDC" },
    { "tdcl_c",   "Corrected Leading Edge times",         "fCorLTDC" },
    { "tdct_c",   "Corrected Trailing Edge times",         "fCorTTDC" },
    { "adc",     "ADC values left side",              "fRawADC" },
    { "adc_c",   "Corrected ADC values left side",    "fCorADC" },
    { "nhit",  "Number of hits",  "fNhit" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void CdetDetector::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaNonTrackingDetector::Clear(opt);

  fNhit = 0;
  fTNhit = 0;
  fANhit = 0;
  if( fIsInit ) {
    assert( fRawADC != 0 && fCorADC != 0 && fRawLTDC !=0 && fRawTTDC !=0 && fCorLTDC !=0 && fCorTTDC !=0);
    memset( fRawADC, 0, fNelem*sizeof(Double_t) );
    memset( fCorADC, 0, fNelem*sizeof(Double_t) );
    memset( fRawLTDC, 0, fNelem*sizeof(Double_t) );
    memset( fRawTTDC, 0, fNelem*sizeof(Double_t) );
    memset( fCorLTDC, 0, fNelem*sizeof(Double_t) );
    memset( fCorTTDC, 0, fNelem*sizeof(Double_t) );
  }
}

//_____________________________________________________________________________
Int_t CdetDetector::Decode( const THaEvData& evdata )
{
  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.
  // This implementation makes the following assumptions about the detector map:
  // - The first half of the map entries corresponds to ADCs,
  //   the second half, to TDCs.
  //   
  // Loop over all modules defined for this detector
  //
  // Only record FIRST hit for a particular TDC channel
  //

  // Clear event-by-event data
  Clear();
 
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = ( d->model ? fDetMap->IsADC(d) : (i < fDetMap->GetSize()/2) );

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
#ifdef WITH_DEBUG
      if( adc && nhit > 1 )
        Warning( Here("Decode"), "%d hits on %s channel %d/%d/%d",
                 nhit, adc ? "ADC" : "TDC", d->crate, d->slot, chan );
#endif
      // Get the data. Scintillators are assumed to have only one 
      // ADC hit (hit=0), but can have multiple TDC hits ... thus only
      // record the first leading edge TDC hit.
      Int_t data=0;
      Int_t datal=0;
      Int_t datat=0;
      Int_t edgetype=-1;
      bool first_trailing_edge = false;
      bool leading_edge_match = false;
      bool trailing_edge_match = false;
      Int_t olddatal = 0;
      Int_t olddatat = 0;
      Int_t oldolddatal = 0;
      Int_t oldolddatat = 0;
      if (adc){	
      	data = evdata.GetData( d->crate, d->slot, chan, 0 );
      }else{
	if (nhit>1){
         //Warning( Here("Decode"), "Number of Hits = %d", nhit );
	 for (Int_t ihitcounter=0;ihitcounter<nhit;ihitcounter++){
	   Int_t tempdata = evdata.GetData(d->crate,d->slot,chan,ihitcounter);
	   Int_t tempdataword = evdata.GetRawData(d->crate,d->slot,chan,ihitcounter);
	   edgetype = (tempdataword&EDGEMASK)>>EDGESHIFT;
	   if (tempdata > TDC_LOWER_LIMIT && tempdata < TDC_UPPER_LIMIT){
           	//Warning( Here("Decode"), "Hit %d: Data = %d: Edgetype = %d",
                // 	ihitcounter, tempdata, edgetype );
	  	if (edgetype == TRAILING_EDGE){
			olddatat = tempdata;
		        first_trailing_edge = true;
			trailing_edge_match = true;
	   	}else if (edgetype == LEADING_EDGE && first_trailing_edge){
			olddatal = tempdata;
			leading_edge_match = true;
			if (leading_edge_match && trailing_edge_match){
				oldolddatal = olddatal;
				oldolddatat = olddatat;
				olddatal = 0;
				olddatat = 0;
				leading_edge_match = false;
				trailing_edge_match = false;
			}
	   	}else{
        		if(edgetype != LEADING_EDGE && edgetype != TRAILING_EDGE) Warning( Here("Decode"), "Edge Problem: edgetype = %d",edgetype);
	   	}
	   }	
	 }
         //Warning( Here("Decode"), "oldolddatal = %d: oldolddatat = %d",
         //        	oldolddatal, oldolddatat );
	 datal = oldolddatal;
	 datat = oldolddatat;
	}else{
#ifdef WITH_DEBUG
         //Warning( Here("Decode"), "%d hits on %s channel %d/%d/%d",
         //        nhit, adc ? "ADC" : "TDC", d->crate, d->slot, chan );
#endif
         datal=0;
	 datat=0;
        }
      }

      // Get the detector channel number, starting at 0
      Int_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) - 1;

#ifdef WITH_DEBUG
      if( k<0 || k>NDEST*fNelem ) {
        // Indicates bad database
        Warning( Here("Decode()"), "Illegal detector channel: %d", k );
        continue;
      }
//        cout << "adc,j,k = " <<adc<<","<<j<< ","<<k<< endl;
#endif
      // Copy the data to the local variables.
      DataDest* dest = fDataDest + k/fNelem;
      k = k % fNelem;
      if( adc ) {
	dest->adc[k]   = static_cast<Double_t>( data );
        dest->adc_c[k] = (data - dest->ped[k])*dest->gain[k];
        (*dest->nahit)++;
      } else {
        dest->tdcl[k]   = static_cast<Double_t>( datal );
        dest->tdcl_c[k] = (datal - dest->offset[k])*fTdc2T;
        dest->tdct[k]   = static_cast<Double_t>( datat );
        dest->tdct_c[k] = (datat - dest->offset[k])*fTdc2T;
        (*dest->nthit)++;
      }
    }
  }
  if ( fDebug > 3 ) {
    printf("\n\nEvent %d   Trigger %d Scintillator %s\n:",
           evdata.GetEvNum(), evdata.GetEvType(), GetPrefix() );
    printf("   paddle  Left(TDC    ADC   ADC_p)   Right(TDC   ADC   ADC_p)\n");
    for ( int i=0; i<fNelem; i++ ) {
      printf("     %2d  %5.0f %5.0f %5.0f %5.0f %5.0f %5.0f\n",
             i+1,fRawLTDC[i],fRawTTDC[i],fCorLTDC[i],fCorTTDC[i],fRawADC[i],fCorADC[i]);
    }
  }

  return fTNhit;
}      

//_____________________________________________________________________________
Int_t CdetDetector::CoarseProcess( TClonesArray& tracks )
{
  // Coarse processing. 'tracks' contains coarse tracks.

  return 0;
}

//_____________________________________________________________________________
Int_t CdetDetector::FineProcess( TClonesArray& tracks )
{
  // Fine processing. 'tracks' contains final tracking results.

  return 0;
}

//_____________________________________________________________________________
static void PrintArray( const Double_t* arr, Int_t size )
{
  // Helper function for Print()

  if( size <= 0 ) {
    cout << "(empty)";
  } else {
    for( Int_t i = 0; i < size; ++i ) {
      cout << arr[i];
      if( i+1 != size )  cout << ", ";
    }
  }
  cout << endl;
}

//_____________________________________________________________________________
void CdetDetector::Print( Option_t* opt ) const
{
  // Print current configuration

  // It is always a good idea to have a Print method, if only for debugging

  THaDetector::Print(opt);
  cout << "detmap = "; fDetMap->Print();
  cout << "nelem = " << fNelem << endl;
  cout << "offsets = "; PrintArray( &fOff[0],  fNelem );
  cout << "pedestals = "; PrintArray( &fPed[0],  fNelem );
  cout << "gains = ";     PrintArray( &fGain[0], fNelem );
  cout << "nhits = " << fNhit << endl;
  cout << "nahits = " << fANhit << endl;
  cout << "rawadc = ";    PrintArray( fRawADC,   fNelem );
  cout << "coradc = ";    PrintArray( fCorADC,   fNelem );
  cout << "nthits = " << fTNhit << endl;
  cout << "rawltdc = ";    PrintArray( fRawLTDC,   fNelem );
  cout << "rawttdc = ";    PrintArray( fRawTTDC,   fNelem );
  cout << "corltdc = ";    PrintArray( fCorLTDC,   fNelem );
  cout << "corttdc = ";    PrintArray( fCorTTDC,   fNelem );
}

////////////////////////////////////////////////////////////////////////////////

ClassImp(CdetDetector)
