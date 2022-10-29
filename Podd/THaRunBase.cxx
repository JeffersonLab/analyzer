//*-- Author :    Ole Hansen   13/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaRunBase
//
// Base class of a description of a run.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunBase.h"
#include "THaRunParameters.h"
#include "THaEvData.h"
#include "DAQconfig.h"
#include "THaPrintOption.h"
#include "TClass.h"
#include "TError.h"
#include <iostream>
#include <algorithm>  // std::copy, std::equal
#include <iterator>   // std::begin, std::end

using namespace std;

static const int UNDEFDATE = 19950101;
static const char* NOTINIT = "uninitialized run";
static const char* DEFRUNPARAM = "THaRunParameters";
static constexpr UInt_t DEFEVTRANGE[2]{1, kMaxUInt};

//_____________________________________________________________________________
THaRunBase::THaRunBase( const char* description )
  : TNamed(NOTINIT, description )
  , fNumber(0)
  , fType(0)
  , fDate(UNDEFDATE,0)
  , fEvtRange{DEFEVTRANGE[0], DEFEVTRANGE[1]}
  , fNumAnalyzed(0)
  , fDBRead(false)
  , fIsInit(false)
  , fOpened(false)
  , fAssumeDate(false)
  , fDataSet(0)
  , fDataRead(0)
  , fDataRequired(kDate)
  , fParam(nullptr)
  , fRunParamClass(DEFRUNPARAM)
  , fDataVersion(0)
  , fExtra(nullptr)
{
  // Normal & default constructor

  ClearEventRange();
  //FIXME: BCI: should be in RunParameters
  DAQInfoExtra::AddTo(fExtra);
}

//_____________________________________________________________________________
THaRunBase::THaRunBase( const THaRunBase& rhs )
  : TNamed(rhs)
  , fNumber(rhs.fNumber)
  , fType(rhs.fType)
  , fDate(rhs.fDate)
  , fEvtRange{rhs.fEvtRange[0], rhs.fEvtRange[1]}
  , fNumAnalyzed(rhs.fNumAnalyzed)
  , fDBRead(rhs.fDBRead)
  , fIsInit(rhs.fIsInit)
  , fOpened(false)
  , fAssumeDate(rhs.fAssumeDate)
  , fDataSet(rhs.fDataSet)
  , fDataRead(rhs.fDataRead)
  , fDataRequired(rhs.fDataRequired)
  , fParam(nullptr)
  , fRunParamClass(rhs.fRunParamClass)
  , fDataVersion(rhs.fDataVersion)
  , fExtra(nullptr)
{
  // Copy ctor

  // NB: the run parameter object might inherit from THaRunParameters
  if( rhs.fParam ) {
    fParam.reset(dynamic_cast<THaRunParameters*>(rhs.fParam->Clone()));
  }
  if( rhs.fExtra )
    fExtra = rhs.fExtra->Clone();
}

//_____________________________________________________________________________
THaRunBase& THaRunBase::operator=(const THaRunBase& rhs)
{
  // Assignment operator. NB: this operator is virtual and is expected
  // to be extended by derived classes.

  if (this != &rhs) {
     TNamed::operator=(rhs);
     fNumber     = rhs.fNumber;
     fType       = rhs.fType;
     fDate       = rhs.fDate;
     copy(begin(rhs.fEvtRange), end(rhs.fEvtRange), begin(fEvtRange));
     fNumAnalyzed = rhs.fNumAnalyzed;
     fDBRead     = rhs.fDBRead;
     fIsInit     = rhs.fIsInit;
     fOpened     = false;
     fAssumeDate = rhs.fAssumeDate;
     fDataSet    = rhs.fDataSet;
     fDataRead   = rhs.fDataRead;
     fDataRequired = rhs.fDataRequired;
     if( rhs.fParam ) {
       fParam.reset(dynamic_cast<THaRunParameters*>(rhs.fParam->Clone()));
     } else
       fParam    = nullptr;
     fRunParamClass = rhs.fRunParamClass;
     fDataVersion   = rhs.fDataVersion;
     delete fExtra;
     if( rhs.fExtra )
       fExtra = rhs.fExtra->Clone();
     else
       fExtra = nullptr;
  }
  return *this;
}

//_____________________________________________________________________________
THaRunBase::~THaRunBase()
{
  // Destructor

  delete fExtra; fExtra = nullptr;
}

//_____________________________________________________________________________
Int_t THaRunBase::Update( const THaEvData* evdata )
{
  // Inspect decoded event data 'evdata' for run parameter data (e.g. prescale
  // factors) and, if any found, extract the parameters and store them here.

  static const char* const here = "THaRunBase::Update";

  if( !evdata )
    return -1;

  Int_t ret = 0;
  // Run date & number
  if( evdata->IsPrestartEvent() ) {
    fDataRead |= kDate|kRunNumber|kRunType;
    if( !fAssumeDate ) {
      fDate.Set( evdata->GetRunTime() );
      fDataSet |= kDate;
    }
    SetNumber( evdata->GetRunNum() );
    SetType( evdata->GetRunType() );
    fDataSet  |= kRunNumber|kRunType;
    ret |= (1<<0);
  }
  // Prescale factors
  if( evdata->IsPrescaleEvent() ) {
    fDataRead |= kPrescales;
    for(int i=0; i<fParam->GetPrescales().GetSize(); i++) {
      Int_t psfact = evdata->GetPrescaleFactor(i+1);
      if( psfact == -1 ) {
	Error( here, "Failed to decode prescale factor for trigger %d. "
	       "Check raw data file for format errors.", i );
	return -2;
      }
      fParam->Prescales()[i] = psfact;
    }
    fDataSet  |= kPrescales;
    ret |= (1<<1);
  }
#define CFGEVT1 Decoder::DAQCONFIG_FILE1
#define CFGEVT2 Decoder::DAQCONFIG_FILE2
  if( evdata->GetEvType() == CFGEVT1 || evdata->GetEvType() == CFGEVT2 ) {
    fDataRead |= kDAQInfo;
    auto* srcifo = DAQInfoExtra::GetFrom(evdata->GetExtra());
    if( !srcifo ) {
      Warning( here, "Failed to decode DAQ config info from event type %u",
               evdata->GetEvType() );
      return -3;
    }
    auto* ifo = DAQInfoExtra::GetFrom(fExtra);
    assert(ifo);     // else bug in constructor
    // Copy info to local run parameters. NB: This may be several MB of data.
    *ifo = *srcifo;
    fDataSet |= kDAQInfo;
    ret |= (1<<2);
  }
  return ret;
}

//_____________________________________________________________________________
bool THaRunBase::operator==( const THaRunBase& rhs ) const
{
  return (fNumber == rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRunBase::operator!=( const THaRunBase& rhs ) const
{
  return (fNumber != rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRunBase::operator<( const THaRunBase& rhs ) const
{
  return (fNumber < rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRunBase::operator>( const THaRunBase& rhs ) const
{
  return (fNumber > rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRunBase::operator<=( const THaRunBase& rhs ) const
{
  return (fNumber <= rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRunBase::operator>=( const THaRunBase& rhs ) const
{
  return (fNumber >= rhs.fNumber);
}

//_____________________________________________________________________________
void THaRunBase::Clear( Option_t* opt )
{
  // Reset the run object as if freshly constructed.
  // However, when opt=="INIT", keep an explicitly set event range and run date

  THaPrintOption sopt(opt);
  sopt.ToUpper();
  bool doing_init = (sopt.Contains("INIT"));

  Close();
  fName = NOTINIT;
  fNumber = -1;
  fType = 0;
  fNumAnalyzed = 0;
  fDBRead = fIsInit = fOpened = false;
  fDataSet = fDataRead = 0;
  fParam->Clear(opt);

  // If initializing and the date was explicitly set, keep the date
  if( doing_init && fAssumeDate )
    SetDate(fDate);
  else
    ClearDate();

  // Likewise, if initializing, keep any explicitly set parameters
  if( !doing_init ) {
    ClearEventRange();
    fDataVersion = 0;
  }
}

//_____________________________________________________________________________
void THaRunBase::ClearDate()
{
  // Reset the run date to "undefined"

  fDate.Set(UNDEFDATE,0);
  fAssumeDate = fIsInit = false;
  fDataSet &= ~kDate;
}

//_____________________________________________________________________________
void THaRunBase::ClearEventRange()
{
  // Reset the range of events to analyze

  copy(begin(DEFEVTRANGE), end(DEFEVTRANGE), begin(fEvtRange));
}

//_____________________________________________________________________________
Int_t THaRunBase::Compare( const TObject* obj ) const
{
  // Compare two THaRunBase objects via run numbers. Returns 0 when equal,
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).

  if (this == obj) return 0;
  const auto *rhs = dynamic_cast<const THaRunBase*>(obj);
  if( !rhs ) return -1;
  if      ( fNumber < rhs->fNumber ) return -1;
  else if ( fNumber > rhs->fNumber ) return  1;
  return 0;
}

//_____________________________________________________________________________
Bool_t THaRunBase::HasInfo( UInt_t bits ) const
{
  // Test if all the bits set in 'bits' are also set in fDataSet.
  // 'bits' should consist of the bits defined in EInfoType.
  return ((bits & fDataSet) == bits);
}

//_____________________________________________________________________________
Bool_t THaRunBase::HasInfoRead( UInt_t bits ) const
{
  // Test if all the bits set in 'bits' are also set in fDataRead.
  // 'bits' should consist of the bits defined in EInfoType.
  return ((bits & fDataRead) == bits);
}

//_____________________________________________________________________________
Int_t THaRunBase::Init()
{
  // Initialize the run. This reads the run database, checks
  // whether the data source can be opened, and if so, initializes the
  // run parameters (run number, time, etc.)

  static const char* const here = "THaRunBase::Init";

  // Set up the run parameter object
  fParam = nullptr;
  const char* s = fRunParamClass.Data();
  TClass* cl = TClass::GetClass(s);
  if( !cl ) {
    Error( here, "Run parameter class \"%s\" not "
	   "available. Load library or fix configuration.", s?s:"" );
    return READ_FATAL;
  }
  if( !cl->InheritsFrom( THaRunParameters::Class() )) {
    Error( here, "Class \"%s\" is not a run parameter class.", s );
    return READ_FATAL;
  }

  fParam.reset(static_cast<THaRunParameters*>( cl->New() ));

  if( !fParam ) {
    Error( here, "Unexpected error creating run parameter object "
	   "\"%s\". Call expert.", s );
    return READ_FATAL;
  }

  // Clear selected parameters (matters for re-init). This calls Close().
  Clear("INIT");

  // Open the data source.
  Int_t retval = Open();
  if( retval )
    return retval;

  // Get initial information - e.g., prescan the data source for
  // the required run date
  retval = ReadInitInfo(0);

  // Close the data source for now to avoid conflicts. It will be opened
  // again later.
  Close();

  if( retval != READ_OK && retval != READ_EOF )
    return retval;

  if( !HasInfo(fDataRequired) ) {
    vector<TString> errmsg = { "run date", "run number", "run type",
                               "prescale factors", "DAQ info" };
    TString errtxt("Missing run parameters: ");
    UInt_t i = 0, n = 0;
    for( auto& msg : errmsg ) {
      if( !HasInfo(BIT(i) & fDataRequired) ) {
	if( n>0 )
	  errtxt += ", ";
	errtxt += msg;
	n++;
      }
      i++;
    }
    errtxt += ". Run not initialized.";
    Error( here, "%s", errtxt.Data() );

    return READ_FATAL;
  }

  // Read the database to obtain additional parameters that are not set
  // by ReadInitInfo
  retval = ReadDatabase();
  if( retval )
    return retval;

  fIsInit = true;
  return READ_OK;
}

//_____________________________________________________________________________
Bool_t THaRunBase::IsOpen() const
{
  return fOpened;
}

//_____________________________________________________________________________
void THaRunBase::Print( Option_t* opt ) const
{
  // Print definition of run
  THaPrintOption sopt(opt);
  sopt.ToUpper();
  if( sopt.Contains("NAMEDESC") ) {
    cout << "\"" << GetName() << "\"";
    if( strcmp( GetTitle(), "") != 0 )
      cout << "  \"" << GetTitle() << "\"";
    return;
  }

  cout << IsA()->GetName() << "  \"" << GetName() << "\"";
  if( strcmp( GetTitle(), "") != 0 )
    cout << "  \"" << GetTitle() << "\"";
  cout << endl;
  cout << "Run number:   " << fNumber << endl;
  cout << "Run date:     " << fDate.AsString() << endl;
  cout << "Data version: " << fDataVersion << endl;
  cout << "Requested event range: ";
  if( std::equal(begin(fEvtRange), end(fEvtRange), begin(DEFEVTRANGE)) )
    cout << "all";
  else
    cout << fEvtRange[0] << "-" << fEvtRange[1];
  cout << endl;

  if( sopt.Contains("STARTINFO") )
    return;

  cout << "Analyzed events:       " << fNumAnalyzed << endl;
  cout << "Assume Date:           " << fAssumeDate << endl;
  cout << "Database read:         " << fDBRead << endl;
  cout << "Initialized:           " << fIsInit << endl;
  cout << "Opened:                " << fOpened << endl;
  cout << "Date set/read/req:     "
       << HasInfo(kDate) << " " << HasInfoRead(kDate) << " "
       << (Bool_t)((kDate & fDataRequired) == kDate) << endl;
  cout << "Run num set/read/req:  "
       << HasInfo(kRunNumber) << " " << HasInfoRead(kRunNumber) << " "
       << (Bool_t)((kRunNumber & fDataRequired) == kRunNumber) << endl;
  cout << "Run type set/read/req: "
       << HasInfo(kRunType) << " " << HasInfoRead(kRunType) << " "
       << (Bool_t)((kRunType & fDataRequired) == kRunType) << endl;
  cout << "Prescales set/rd/req:  "
       << HasInfo(kPrescales) << " " << HasInfoRead(kPrescales) << " "
       << (Bool_t)((kPrescales & fDataRequired) == kPrescales) << endl;
  cout << "DAQInfo set/rd/req:    "
       << HasInfo(kDAQInfo) << " " << HasInfoRead(kDAQInfo) << " "
       << (Bool_t)((kDAQInfo & fDataRequired) == kDAQInfo) << endl;

  if( fParam )
    fParam->Print(opt);
}

//_____________________________________________________________________________
Int_t THaRunBase::ReadDatabase()
{
  // Query the run database for the parameters of this run. The actual
  // work is done in the THaRunParameters object. Usually, the beam and target
  // parameters are read.  Internal function called by Init().
  //
  // Return 0 if success.

  static const char* const here = "ReadDatabase";

  if( !fParam ) {
    Error( here, "No run parameter object defined!?! "
	   "Something is very wrong." );
    return READ_FATAL;
  }
  TDatime undef(UNDEFDATE,0);
  if( fDate == undef ) {
    Error( here, "Run date undefined. Cannot read database without a date." );
    return READ_FATAL;
  }

  Int_t st = fParam->ReadDatabase(fDate);
  if( st ) {
    Error( here, "Failed to read run database, error = %d. "
                 "Cannot continue. Check for typos and ill-formed lines.", st);
    return READ_FATAL;
  }

  fDBRead = true;
  return READ_OK;
}

//_____________________________________________________________________________
Int_t THaRunBase::ReadInitInfo( Int_t /*level*/ )
{
  // Read initial information from the run data source.
  // Internal function called by Init(). The default version checks
  // if run date set and prints an error if not.

  // The data source can be assumed to be open at the time this
  // routine is called.

  if( !fAssumeDate )
    Error( "ReadInitInfo", "Run date not set." );

  return READ_OK;
}

//_____________________________________________________________________________
void THaRunBase::SetDate( const TDatime& date )
{
  // Set the timestamp of this run to 'date'.

  if( fDate != date ) {
    fDate = date;
    fIsInit = false;
  }
  fAssumeDate = true;
  fDataSet |= kDate;
}

//_____________________________________________________________________________
void THaRunBase::SetDate( UInt_t tloc )
{
  // Set timestamp of this run to 'tloc' which is in Unix time
  // format (number of seconds since 01 Jan 1970).

  TDatime date( tloc );
  SetDate( date );
}

//_____________________________________________________________________________
void THaRunBase::SetDataRequired( UInt_t mask )
{
  // Set bitmask of information items that must be extracted from the run
  // for Init() to succeed. Can be used to override the default value set by
  // the class for special cases. Use with caution! The default values are
  // usually sensible, and further analysis will usually fail if certain
  // info is not present.
  //
  // The 'mask' argument should contain only the bits defined in EInfoType.
  // Additional bits are ignored with a warning.
  //
  // Example: Only require the run date:
  //
  // run->SetDataRequired( THaRunBase::kDate );
  //

  UInt_t all_info = kDate | kRunNumber | kRunType | kPrescales | kDAQInfo;
  if( (mask & all_info) != mask ) {
    Warning( "THaRunBase::SetDataRequired", "Illegal bit(s) 0x%x in bitmask "
	     "argument ignored. See EInfoType.", (mask & ~all_info) );
  }
  fDataRequired = (mask & all_info);
}

//_____________________________________________________________________________
Int_t THaRunBase::SetDataVersion( Int_t version )
{
  // Set data format version (meaning is dependent on the underlying
  // data source). THaRunBase does not use this variable.

  return (fDataVersion = version);
}
//_____________________________________________________________________________
void THaRunBase::SetEventRange( UInt_t first, UInt_t last )
{
  // Set range of event numbers to analyze. The interpretation of
  // the event range depends on the analyzer. Usually, it refers
  // to the range of physics event numbers.

  fEvtRange[0] = first;
  fEvtRange[1] = last;
}

//_____________________________________________________________________________
void THaRunBase::SetFirstEvent( UInt_t n )
{
  // Set number of first event to analyze

  fEvtRange[0] = n;
}

//_____________________________________________________________________________
void THaRunBase::SetLastEvent( UInt_t n )
{
  // Set number of last event to analyze

  fEvtRange[1] = n;
}

//_____________________________________________________________________________
void THaRunBase::SetNumber( UInt_t number )
{
  // Change/set the number of the Run.

  fNumber = number;
  SetName( Form("RUN_%u", number) );
  fDataSet |= kRunNumber;
}

//_____________________________________________________________________________
void THaRunBase::SetType( UInt_t type )
{
  // Set run type
  fType = type;
  fDataSet |= kRunType;
}

//_____________________________________________________________________________
void THaRunBase::SetRunParamClass( const char* classname )
{
  // Set class of run parameters to use

  fRunParamClass = classname;
}

//_____________________________________________________________________________
size_t THaRunBase::GetNConfig() const
{
  auto* ifo = DAQInfoExtra::GetFrom(fExtra);
  if( !ifo )
    return 0;
  return ifo->strings.size();
}

//_____________________________________________________________________________
const string& THaRunBase::GetDAQConfig( size_t i ) const
{
  static const string nullstr;
  auto* ifo = DAQInfoExtra::GetFrom(fExtra);
  if( !ifo || i >= ifo->strings.size() )
    return nullstr;
  return ifo->strings[i];
}

//_____________________________________________________________________________
const string& THaRunBase::GetDAQInfo( const std::string& key ) const
{
  static const string nullstr;
  auto* ifo = DAQInfoExtra::GetFrom(fExtra);
  if( !ifo )
    return nullstr;
  const auto& keyval = ifo->keyval;
  auto it = keyval.find(key);
  if( it == keyval.end() )
    return nullstr;
  return it->second;
}

//_____________________________________________________________________________
ClassImp(THaRunBase)
