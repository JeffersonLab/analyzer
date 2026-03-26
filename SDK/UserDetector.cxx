//*-- Author :    Ole Hansen   01-Dec-03
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserDetector                                                              //
//                                                                           //
// Example of a user-defined detector class. This class implements a         //
// completely new detector from scratch. Only ReadDatabase() and StoreHit()  //
// perform slightly non-trivial work. CoarseProcess/FineProcess must be      //
// implemented as needed.                                                    //
//                                                                           //
// See UserScintillator for an example of a detector class where the         //
// behavior of an existing detector (scintillator) is modified and extended. //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UserDetector.h"
#include "VarDef.h"
#include "THaDetMap.h"
#include "TMath.h"
#include "Helper.h"
// Needed in CoarseProcess/FineProcess if implemented
#include "THaTrack.h"
#include "TClonesArray.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <utility>

using namespace std;
using namespace Podd;

// Example of a hardcoded parameter (arbitrary value, see ReadDatabase).
// Generally not recommended. The day will come when someone needs more than this.
constexpr int MAXCHAN = 100;

// Constructors should initialize all basic-type member variables to safe
// default values. In particular, all pointers should be either zeroed or
// assigned valid data.
//_____________________________________________________________________________
UserDetector::UserDetector( const char* name, const char* description,
                            THaApparatus* apparatus )
  : THaNonTrackingDetector(name, description, apparatus)
{
  // Constructor
}

//_____________________________________________________________________________
UserDetector::~UserDetector()
{
  // Destructor. Remove variables from global list.

  // Unfortunately, we can't rely on one of the base classes to remove our
  // variables because of the way virtual functions are resolved in the scope
  // of destructors.
  // The following calls THaAnalysisObject::DefineVariablesWrapper(), and
  // the virtual function call to DefineVariables(kDelete) within that function
  // resolves to the DefineVariables implementation in this class, which would
  // not be the case if called from THaAnalysisObject's destructor.
  // Hence, RemoveVariables() should be called from the destructor of
  // every class that defines its own global variables through the standard
  // DefineVariables() mechanism, like this one does.
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
  // of Analyzer 1.x. The format of the database file is up to the
  // author of the detector class. Here, we use key/value pairs to allow
  // for a free file format.

  const char* const here = "ReadDatabase";

  // Open the database file
  // NB: OpenFile() is in THaAnalysisObject. It will try to open a database
  // file named db_<fPrefix><fName>.dat (fPrefix is in THaAnalysisObject
  // and is typically the name of the apparatus containing this detector;
  // fName is the name of this object in TNamed).
  CFile file = OpenFile( date );  // closes file upon exiting this function
  if( !file ) return kFileError;

  // Read the detector map. The result will be in fDetMap.
  // In the database, this is a matrix of numerical values with format:
  //  detmap = <crate1> <slot1> <first channel1> <last channel1> <first logical ch1>
  //           <crate2> <slot2> <first channel2> <last channel2> <first logical ch2>
  //  etc.
  // The "first logical channel" is the one assigned to the first hardware channel.
  //
  // Alternatively, and perhaps better, use the moderinzed format (see
  // the documentation for THaDetMap for details):
  //  detector_map = <crate1> <slot1> <first channel1> <last channel1> <first logical ch1>
  // etc.
  // ReadDetMap supports both formats. The flags (3rd argument) are ignored
  // with the modernized format.
  Int_t err = ReadDetMap(file, date, THaDetMap::kFillLogicalChannel);
  if( err )
    return err;

  // All vectors should be cleared and all conditions variables should be set
  // to default values here.
  // NB: fPed and fGain will be set to default values below in case there are
  // no values for them in the database.
  Int_t nelem = 0;
  Double_t angle = 0.0;
  fPed.clear();
  fGain.clear();

  // Read the database
  // Automatically determine which data type constants correspond to Data_t.
  // This can be ignored/removed if the member variables are hardcoded as
  // Float_t or Double_t.
  //  VarType kDataType  = std::is_same_v<Data_t, Float_t> ? kFloat  : kDouble;
  VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;

  // Set up an array of database requests. See VarDef.h for details.
  // If an optional parameter is not found in the database, the value of its
  // associated variable remains unchanged.
  const vector<DBRequest> request = {
    // Required items. Type kDouble is the default and can be omitted.
    { .name = "nelem",     .var = &nelem,  .type = kInt,       .search = -1     }, // Number of elements (e.g. PMTs)
    // Optional items
    { .name = "angle",     .var = &angle,/*.type = kDouble,*/  .optional = true }, // Rotation angle about y (deg)
    { .name = "pedestals", .var = &fPed,   .type = kDataTypeV, .optional = true }, // Pedestals (optional)
    { .name = "gains",     .var = &fGain,  .type = kDataTypeV, .optional = true }, // Gains (optional)
  };

  // Read the requested values
  err = LoadDB(file, date, request);
  if( err )
    return err;

  // Check all configuration values for sanity. This is the more tedious, but
  // nevertheless very important part of reading the database. In addition to
  // simple checking, this part may also include computation of derived
  // configuration parameters, for instance an allowed hit pattern or
  // geometry parameters. See the call to DefineAxes below as an example.
  if( nelem <= 0 ) {
    Error( Here(here), "Cannot have zero or negative number of elements. "
	   "Fix database." );
    return kInitError;
  }
  if( nelem > MAXCHAN ) {
    Error( Here(here), "Illegal number of elements = %d. Must be <= %d. "
	   "Fix database.", nelem, MAXCHAN );
    return kInitError;
  }

  // If the detector is already initialized, the number of elements must not
  // change. This prevents problems with global variables that
  // point to memory that is dynamically allocated.
  // Resizing a detector is hardly ever necessary for a given analysis,
  // so this catches database errors and simplifies the design of this class.
  if( fIsInit && nelem != fNelem ) {
    ostringstream ostr;
    ostr << "Cannot re-initialize with different number of elements. "
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
  UInt_t nchan = fDetMap->GetTotNumChan();
  if( std::cmp_not_equal(nchan, nelem) ) { // type-safe integer comparison
    ostringstream ostr;
    ostr << "Incorrect number of detector map channels = " << nchan
         << ". Must equal nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }

  // Check if the sizes of the arrays we read make sense.
  // NB: If we have an empty vector here, there was no database entry for it.
  if( !fPed.empty() && std::ssize(fPed) != nelem) {
    ostringstream ostr;
    ostr << "Incorrect number of pedestal values = " << fPed.size()
         << ". Must match nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }
  if( !fGain.empty() && std::ssize(fGain) != nelem) {
    ostringstream ostr;
    ostr << "Incorrect number of gain values = " << fGain.size()
         << ". Must match nelem = " << nelem << ". Fix database.";
    Error( Here(here), "%s", ostr.str().c_str() );
    return kInitError;
  }

  // If any of the arrays were not given in the database, set them to defaults:
  // Pedestals default to zero, gains to 1.0.
  if( fPed.empty() )
    fPed.assign( nelem, 0.0 );
  if( fGain.empty() )
    fGain.assign( nelem, 1.0 );

  // Use the rotation angle to set the axes vectors
  // (required for display functions, for instance)
  // See THaDetectorBase::DefineAxes for details.
  constexpr Double_t degrad = TMath::Pi()/180.0;
  DefineAxes(angle*degrad);

  fIsInit = true;

  return kOK;
}

//_____________________________________________________________________________
Int_t UserDetector::DefineVariables( EMode mode )
{
  // Define (or delete) global variables of this detector

  const vector<RVarDef> vars = {
    { .name = "nhit",  .desc = "Number of hits",  .def = "GetNhits()"          },
    { .name = "chan",  .desc = "Channel number",  .def = "fEventData.fChannel" },
    { .name = "adc",   .desc = "Raw ADC value",   .def = "fEventData.fRawADC"  },
    { .name = "adc_c", .desc = "Calibrated ADC",  .def = "fEventData.fCalADC"  },
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void UserDetector::Clear( Option_t* opt )
{
  // Reset per-event data. This function is called before Decode() for
  // every event.

  THaNonTrackingDetector::Clear(opt);
  fEventData.clear();
}

//_____________________________________________________________________________
Int_t UserDetector::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Simple example method for storing decoded data.
  // NB: All detectors use THaDetectorBase::Decode as the default Decode method.
  //
  // This is called from THaDetectorBase::Decode for every hit belonging to this
  // detector in the current event. The THaDetectorBase::Decode method assumes
  // single-hit hardware, i.e. only one hit per channel. If multiple hits per
  // channel need to be analyzed, the detector class needs to override Decode.
  //
  // As an alternative to writing a StoreHit method, one can add ChannelData
  // objects to the fChannelData vector (which is provided in THaDetectorBase).
  // This would be done during initialization, typically in ReadDatabase.

  // Use the logical detector channel to index the data. The logical channel
  // is calculated in THaDetMap::Module::ConvertToLogicalChannel as
  // d->first + hardware_channel - d->lo, where 'd' refers to the detector
  // map "Module" (= line in the database) corresponding to the frontend
  // where the hit was registered. It typically runs from 0 to nelem-1.
  Int_t chan = hitinfo.lchan;

  // Check if 'chan' is in range here as a bugcheck of the detector map logic.
  // Like asserts, such bug ªchecks may be skipped for better performance when
  // compiling production code with -DNDEBUG. Or you may keep them for more
  // safety, especially when some value depends on unpredictable input data.
#ifndef NDEBUG
  if( chan < 0 || chan >= fNelem )
    throw std::logic_error("UserDetector::StoreHit: invalid logical channel");
#endif

  // Copy the data to its destination. Here, we use a custom data structure
  // to store the logical channel number, raw data and calibrated data. This
  // provides a variable size vector of data for each event, holding only
  // the channels with data.
  fEventData.emplace_back(chan, data, (data - fPed[chan]) * fGain[chan]);

  // The return value is currently ignored by THeDetectorBase::Decode.
  return 0;
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
void UserDetector::Print( Option_t* opt ) const
{
  // Print current configuration

  // It is always a good idea to have a Print method, if only for debugging

  THaDetector::Print(opt);
  cout << "detmap = "; fDetMap->Print();
  cout << "nelem = " << fNelem << endl;
  PrintArray("pedestals", fPed);
  PrintArray("gains", fGain);
  cout << "nhits = " << GetNhits() << endl;
  PrintArrayField("channel", fEventData, &EventData::fChannel);
  PrintArrayField("rawadc", fEventData, &EventData::fRawADC);
  PrintArrayField("coradc", fEventData, &EventData::fCalADC);
}

////////////////////////////////////////////////////////////////////////////////

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(UserDetector)
#endif
