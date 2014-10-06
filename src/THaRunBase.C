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
#include "TClass.h"
#include <iostream>
#if ROOT_VERSION_CODE < ROOT_VERSION(3,1,6)
#include <ctime>
#endif

using namespace std;

static const int UNDEFDATE = 19950101;
static const char* NOTINIT = "uninitialized run";

//_____________________________________________________________________________
THaRunBase::THaRunBase( const char* description ) : 
  TNamed(NOTINIT, description ), 
  fNumber(-1), fType(0), fDate(UNDEFDATE,0), fNumAnalyzed(0),
  fDBRead(kFALSE), fIsInit(kFALSE), fOpened(kFALSE), fAssumeDate(kFALSE), 
  fDataSet(0), fDataRead(0), fDataRequired(kDate), fParam(0)
{
  // Normal & default constructor

  ClearEventRange();
}

//_____________________________________________________________________________
THaRunBase::THaRunBase( const THaRunBase& rhs ) : 
  TNamed( rhs ), fNumber(rhs.fNumber), fType(rhs.fType), 
  fDate(rhs.fDate), fNumAnalyzed(rhs.fNumAnalyzed), fDBRead(rhs.fDBRead), 
  fIsInit(rhs.fIsInit), fOpened(kFALSE), fAssumeDate(rhs.fAssumeDate),
  fDataSet(rhs.fDataSet), fDataRead(rhs.fDataRead), 
  fDataRequired(rhs.fDataRequired)
{
  // Copy ctor

  fEvtRange[0] = rhs.fEvtRange[0];
  fEvtRange[1] = rhs.fEvtRange[1];
  // NB: the run parameter object might inherit from THaRunParameters
  if( rhs.fParam ) {
    fParam = static_cast<THaRunParameters*>(rhs.fParam->IsA()->New());
    *fParam = *rhs.fParam;
  } else
    fParam    = 0;
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
     fEvtRange[0] = rhs.fEvtRange[0];
     fEvtRange[1] = rhs.fEvtRange[1];
     fNumAnalyzed = rhs.fNumAnalyzed;
     fDBRead     = rhs.fDBRead;
     fIsInit     = rhs.fIsInit;
     fOpened     = kFALSE;
     fAssumeDate = rhs.fAssumeDate;
     fDataSet    = rhs.fDataSet;
     fDataRead   = rhs.fDataRead;
     fDataRequired = rhs.fDataRequired;
     if( rhs.fParam ) {
       fParam = static_cast<THaRunParameters*>(rhs.fParam->IsA()->New());
       *fParam = *rhs.fParam;
     } else
       fParam    = 0;
  }
  return *this;
}

//_____________________________________________________________________________
THaRunBase::~THaRunBase()
{
  // Destructor

  delete fParam   ; fParam    = 0;
}

//_____________________________________________________________________________
Int_t THaRunBase::Update( const THaEvData* evdata )
{
  // Inspect decoded event data 'evdata' for run parameter data (e.g. prescale
  // factors) and, if any found, extract the parameters and store them here.

  if( !evdata )
    return -1;

  Int_t ret = 0;
  // Run date & number
  if( evdata->IsPrestartEvent() ) {
    if( !fAssumeDate ) {
      fDate.Set( evdata->GetRunTime() );
      fDataSet |= kDate;
    }
    SetNumber( evdata->GetRunNum() );
    SetType( evdata->GetRunType() );
    fDataRead |= kDate|kRunNumber|kRunType;
    ret = 1;
  } 
  // Prescale factors
  if( evdata->IsPrescaleEvent() ) {
    for(int i=0; i<fParam->GetPrescales().GetSize(); i++)
      fParam->Prescales()[i] = evdata->GetPrescaleFactor(i+1);
    fDataSet  |= kPrescales;
    fDataRead |= kPrescales;
    ret = 2;
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
void THaRunBase::Clear( const Option_t* opt )
{
  // Reset the run object as if freshly constructed.
  // However, when opt=="INIT", keep an explicitly set event range and run date

  TString sopt(opt);
  bool doing_init = (sopt == "INIT");

  Close();
  fName = NOTINIT;
  fNumber = -1;
  fType = 0;
  fNumAnalyzed = 0;
  fDBRead = fIsInit = fOpened = kFALSE;
  fDataSet = fDataRead = 0;
  fParam->Clear(opt);

  // If initializing and the date was explicitly set, keep the date
  if( doing_init && fAssumeDate )
    SetDate(fDate);
  else
    ClearDate();

  // Likewise, if initializing, keep any explicitly set event range
  if( !doing_init ) {
    ClearEventRange();
  }
}

//_____________________________________________________________________________
void THaRunBase::ClearDate()
{
  // Reset the run date to "undefined"

  fDate.Set(UNDEFDATE,0);
  fAssumeDate = fIsInit = kFALSE;
  fDataSet &= ~kDate;
}

//_____________________________________________________________________________
void THaRunBase::ClearEventRange()
{
  // Reset the range of events to analyze

  fEvtRange[0] = 1;
  fEvtRange[1] = kMaxUInt;
}

//_____________________________________________________________________________
Int_t THaRunBase::Compare( const TObject* obj ) const
{
  // Compare two THaRunBase objects via run numbers. Returns 0 when equal, 
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).

  if (this == obj) return 0;
  const THaRunBase* rhs = dynamic_cast<const THaRunBase*>(obj);
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

  static const char* const here = "Init";

  // Make sure we have a run parameter structure. This object will 
  // allocate THaRunParameters automatically. If a derived class needs
  // a different structure, then its Init() method must allocate it
  // before calling this Init().
  if( !fParam )
    fParam = new THaRunParameters;

  // Clear selected parameters (matters for re-init)
  Clear("INIT");

  // Open the data source.
  Int_t retval = 0;
  if( !IsOpen() ) {
    retval = Open();
    if( retval )
      return retval;
  }

  // Get initial information - e.g., prescan the data source for
  // the required run date
  retval = ReadInitInfo();

  // Close the data source for now to avoid conflicts. It will be opened
  // again later.
  Close();

  if( retval != READ_OK && retval != READ_EOF )
    return retval;

  if( !HasInfo(fDataRequired) ) {
    const char* errmsg[] = { "run date", "run number", "run type", 
			     "prescale factors", 0 };
    TString errtxt("Missing run parameters: ");
    UInt_t i = 0, n = 0;
    const char** msg = errmsg;
    while( *msg ) {
      if( !HasInfo(BIT(i)&fDataRequired) ) {
	if( n>0 )
	  errtxt += ", ";
	errtxt += *msg;
	n++;
      }
      i++;
      msg++;
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

  fIsInit = kTRUE;
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
  TNamed::Print( opt );
  cout << "Run number: " << fNumber << endl;
  cout << "Run date:   " << fDate.AsString() << endl;
  cout << "Requested event range: " << fEvtRange[0] << "-"
       << fEvtRange[1] << endl;

  TString sopt(opt);
  if( sopt == "STARTINFO" )
    return;

  cout << "Analyzed events:       " << fNumAnalyzed << endl;
  cout << "Assume Date:           " << fAssumeDate << endl;
  cout << "Database read:         " << fDBRead << endl;
  cout << "Initialized  :         " << fIsInit << endl;
  cout << "Opened       :         " << fOpened << endl;
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
  if( st )
    return st;

  fDBRead = true;
  return READ_OK;
}
  
//_____________________________________________________________________________
Int_t THaRunBase::ReadInitInfo()
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
    fIsInit = kFALSE;
  }
  fAssumeDate = kTRUE;
  fDataSet |= kDate;
}

//_____________________________________________________________________________
void THaRunBase::SetDate( UInt_t tloc )
{
  // Set timestamp of this run to 'tloc' which is in Unix time
  // format (number of seconds since 01 Jan 1970).

#if ROOT_VERSION_CODE >= ROOT_VERSION(3,1,6)
  TDatime date( tloc );
#else
  time_t t = tloc;
  struct tm* tp = localtime(&t);
  TDatime date;
  date.Set( tp->tm_year, tp->tm_mon+1, tp->tm_mday,
	    tp->tm_hour, tp->tm_min, tp->tm_sec );
#endif
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
  
  UInt_t all_info = kDate | kRunNumber | kRunType | kPrescales;
  if( (mask & all_info) != mask ) {
    Warning( "THaRunBase::SetDataRequired", "Illegal bit(s) 0x%x in bitmask "
	     "argument ignored. See EInfoType.", (mask & ~all_info) );
  }
  fDataRequired = (mask & all_info);
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
void THaRunBase::SetNumber( Int_t number )
{
  // Change/set the number of the Run.

  fNumber = number;
  SetName( Form("RUN_%u", number) );
  fDataSet |= kRunNumber;
}

//_____________________________________________________________________________
void THaRunBase::SetType( Int_t type )
{
  // Set run type
  fType = type;
  fDataSet |= kRunType;
}

//_____________________________________________________________________________
ClassImp(THaRunBase)
