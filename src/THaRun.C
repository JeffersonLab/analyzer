//*-- Author :    Ole Hansen   13/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
// Description of a run.  This is essentially a TNamed object with
// additional support for a run number, filename, CODA file IO, 
// and run statistics.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRun.h"
#include "THaRunParameters.h"
#include "THaEvData.h"
#include "THaCodaData.h"
#include "THaCodaFile.h"
#include "THaGlobals.h"
#include "TMath.h"
#include "TClass.h"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <climits>
#if ROOT_VERSION_CODE < ROOT_VERSION(3,1,6)
#include <ctime>
#endif

using namespace std;

static const int MAXSCAN = 5000;
static const int UNDEFDATE = 19950101;
static const char* NOTINIT = "uninitialized run";

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* descr ) : 
  TNamed(NOTINIT, strlen(descr) ? descr : fname), 
  fNumber(-1), fType(0), fFilename(fname), fDate(UNDEFDATE,0), fNumAnalyzed(0),
  fDBRead(kFALSE), fIsInit(kFALSE), fAssumeDate(kFALSE), fMaxScan(MAXSCAN),
  fDataSet(0), fDataRead(0), fDataRequired(kDate), fParam(0)
{
  // Normal & default constructor

  fCodaData = new THaCodaFile;  //Specifying the file name would open the file
  ClearEventRange();
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) : 
  TNamed( rhs ), fNumber(rhs.fNumber), fType(rhs.fType), 
  fFilename(rhs.fFilename), fDate(rhs.fDate), fNumAnalyzed(rhs.fNumAnalyzed),
  fDBRead(rhs.fDBRead), fIsInit(rhs.fIsInit), fAssumeDate(rhs.fAssumeDate),
  fMaxScan(rhs.fMaxScan), fDataSet(rhs.fDataSet), fDataRead(rhs.fDataRead), 
  fDataRequired(rhs.fDataRequired)
{
  // Copy ctor

  fEvtRange[0] = rhs.fEvtRange[0];
  fEvtRange[1] = rhs.fEvtRange[1];
  fCodaData = new THaCodaFile;
  // NB: the run parameter object might inherit from THaRunParameters
  if( rhs.fParam ) {
    fParam = static_cast<THaRunParameters*>(rhs.fParam->IsA()->New());
    *fParam = *rhs.fParam;
  } else
    fParam    = 0;
}

//_____________________________________________________________________________
THaRun& THaRun::operator=(const THaRun& rhs)
{
  // Assignment operator.

  if (this != &rhs) {
     TNamed::operator=(rhs);
     fNumber     = rhs.fNumber;
     fType       = rhs.fType;
     fFilename   = rhs.fFilename;
     fDate       = rhs.fDate;
     fEvtRange[0] = rhs.fEvtRange[0];
     fEvtRange[1] = rhs.fEvtRange[1];
     fNumAnalyzed = rhs.fNumAnalyzed;
     fDBRead     = rhs.fDBRead;
     fIsInit     = rhs.fIsInit;
     fAssumeDate = rhs.fAssumeDate;
     fMaxScan    = rhs.fMaxScan;
     fDataSet    = rhs.fDataSet;
     fDataRead   = rhs.fDataRead;
     fDataRequired = rhs.fDataRequired;
     delete fCodaData;
     fCodaData   = new THaCodaFile;
     if( rhs.fParam ) {
       fParam = static_cast<THaRunParameters*>(rhs.fParam->IsA()->New());
       *fParam = *rhs.fParam;
     } else
       fParam    = 0;
  }
  return *this;
}

//_____________________________________________________________________________
THaRun::~THaRun()
{
  // Destroy the Run. The CODA file will be closed by the THaCodaFile 
  // destructor if necessary.

  delete fParam   ; fParam    = 0;
  delete fCodaData; fCodaData = 0;
}

//_____________________________________________________________________________
Bool_t THaRun::HasInfo( UInt_t bits ) const
{
  // Test if all the bits set in 'bits' are also set in fDataSet. 
  // 'bits' should consist of the bits defined in EInfoType.
  return ((bits & fDataSet) == bits);
}

//_____________________________________________________________________________
Bool_t THaRun::HasInfoRead( UInt_t bits ) const
{
  // Test if all the bits set in 'bits' are also set in fDataRead. 
  // 'bits' should consist of the bits defined in EInfoType.
  return ((bits & fDataRead) == bits);
}

//_____________________________________________________________________________
Int_t THaRun::Init()
{
  // Initialize the run. This reads the run database, checks 
  // whether the data source can be opened, and if so, initializes the
  // run parameters (run number, time, etc.)

  static const char* const here = "Init";

  // Make sure we have a run parameter structure. This object will 
  // allocate THaRunParameters automatically. If a derived class needs
  // a different structure, then its Init() method must allocate it
  // before calling this Init().
  if( !fParam ) {
    fParam = new THaRunParameters;
    // THaEvData uses a fixed number of prescale factors, so we can
    // just set the size of the array here once and for all
    fParam->Prescales().Set(THaEvData::MAX_PSFACT);
    // Default all prescale factors to -1 (not set)
    for(int i=0; i<fParam->GetPrescales().GetSize(); i++)
      fParam->Prescales()[i] = -1;
  }

  // Open the data source. This will fail if the CODA file doesn't exist,
  // a common source of problems.
  Int_t retval = 0;
  if( !IsOpen() ) {
    retval = OpenFile();
    if( retval ) {
      Error( here, "Cannot open CODA file %s. Make sure the file exists.",
	     GetFilename() );
      return retval;
    }
  }

  // Clear selected parameters (matters for re-init)
  Clear("INIT");

  // If pre-scanning of the data file requested, read up to fMaxScan events
  // and extract run parameters from the data.
  if( fMaxScan > 0 ) {
    UInt_t wanted_info = kDate|kRunNumber|kRunType|kPrescales;
    THaEvData* evdata = static_cast<THaEvData*>(gHaDecoder->New());
    Int_t nev = 0;
    Int_t status = S_SUCCESS;
    while( nev<fMaxScan && !HasInfo(wanted_info) && 
	   (status = ReadEvent()) == S_SUCCESS ) {

      // Decode events. Skip bad events.
      nev++;
      if( evdata->LoadEvent( GetEvBuffer()) != THaEvData::HED_OK ) {
	Warning( here, "Error decoding event buffer at event %d", nev );
	continue;
      }

      // Inspect event and extract run parameters if appropriate
      Int_t st = Update( evdata );
      //FIXME: debug
      if( st == 1 )
	cout << "Prestart at " << nev << endl;
      else if( st == 2 )
	cout << "Prescales at " << nev << endl;

    }//end while
    delete evdata;

    if( status != S_SUCCESS && status != EOF ) {
      Error( here, "Error %d reading CODA file %s. Check file permissions.",
	     status, GetFilename());
      return status;
    }
  } //end if(fMaxScan>0)

  if( !HasInfo(fDataRequired) ) {
    Error( here, "Not all required run parameters are set.\n"
	   "Set them explicitly or increase scan range and ensure "
	   "that the data file is not a continuation segment." );
    return 255;
  }

  // Close the data source for now to avoid conflicts. It will be opened
  // again later.
  CloseFile();

  // Extract additional run parameters (beam energy etc.) from the database
  retval = ReadDatabase();
  if( retval ) {
    Error( here, "Error reading run database for run number %d, file %s, "
	   "run date %s.\nMake sure the database file exists and contains "
	   "all required parameters.",
	   GetNumber(), GetFilename(), GetDate().AsString() );
    return retval;
  }

  fIsInit = kTRUE;
  return 0;
}

//_____________________________________________________________________________
Int_t THaRun::Update( const THaEvData* evdata )
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
    fDataSet  |= kRunNumber|kRunType;
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
bool THaRun::operator==( const THaRun& rhs ) const
{
  return (fNumber == rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRun::operator!=( const THaRun& rhs ) const
{
  return (fNumber != rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRun::operator<( const THaRun& rhs ) const
{
  return (fNumber < rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRun::operator>( const THaRun& rhs ) const
{
  return (fNumber > rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRun::operator<=( const THaRun& rhs ) const
{
  return (fNumber <= rhs.fNumber);
}

//_____________________________________________________________________________
bool THaRun::operator>=( const THaRun& rhs ) const
{
  return (fNumber >= rhs.fNumber);
}

//_____________________________________________________________________________
Int_t THaRun::Compare( const TObject* obj ) const
{
  // Compare two THaRun objects via run numbers. Returns 0 when equal, 
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).

   if (this == obj) return 0;
   if      ( fNumber < static_cast<const THaRun*>(obj)->fNumber ) return -1;
   else if ( fNumber > static_cast<const THaRun*>(obj)->fNumber ) return  1;
   return 0;
}

//_____________________________________________________________________________
const Int_t* THaRun::GetEvBuffer() const
{ 
  // Return address of the eventbuffer allocated and filled 
  // by fCodaData->codaRead()

  return fCodaData ? fCodaData->getEvBuffer() : 0; 
}

//_____________________________________________________________________________
Int_t THaRun::CloseFile()
{
  return fCodaData ? fCodaData->codaClose() : 0;
}

//_____________________________________________________________________________
bool THaRun::IsOpen() const
{
  return fCodaData ? fCodaData->isOpen() : false;
}

//_____________________________________________________________________________
Int_t THaRun::OpenFile()
{
  // Open CODA file for read-only access.

  if( fFilename.IsNull() )
    return -2;  // filename not set

  return fCodaData ? fCodaData->codaOpen( fFilename ) : -3;
}

//_____________________________________________________________________________
Int_t THaRun::OpenFile( const char* filename )
{
  // Set the filename and then open CODA file.

  SetFilename( filename );
  return OpenFile();
}

//_____________________________________________________________________________
Int_t THaRun::ReadEvent()
{
  // Read one event from CODA file.

  return fCodaData ? fCodaData->codaRead() : -255;
}

//_____________________________________________________________________________
void THaRun::Print( Option_t* opt ) const
{
  // Print definition of run
  TNamed::Print( opt );
  cout << "File name:  " << fFilename.Data() << endl;
  cout << "Run number: " << fNumber << endl;
  cout << "Run date:   " << fDate.AsString() << endl;
  cout << "Requested event range: " << fEvtRange[0] << "-"
       << fEvtRange[1] << endl;

  if( !strcmp(opt,"STARTINFO"))
    return;

  cout << "Analyzed events:       " << fNumAnalyzed << endl;
  cout << "Max events for scan:   " << fMaxScan << endl;
  cout << "Assume Date:           " << fAssumeDate << endl;
  cout << "Database read:         " << fDBRead << endl;
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
Int_t THaRun::ReadDatabase()
{
  // Query the run database for the parameters of this run. The actual
  // work is done in the THaRunParameters object. Usually, the beam and target 
  // parameters are read.  Internal function called by Init().
  //
  // Return 0 if success, <0 if file error, >0 if not all required data found.

  if( !fParam )
    return -1;
  Int_t st = fParam->ReadDatabase(fDate);
  if( st )
    return st;

  fDBRead = true;
  return 0;
}
  
//_____________________________________________________________________________
void THaRun::Clear( const Option_t* opt )
{
  // Reset the run object.

  bool doing_init = !strcmp(opt,"INIT");

  fNumber = -1;
  fType = 0;
  fNumAnalyzed = 0;
  fDBRead = fIsInit = kFALSE;
  fDataSet = fDataRead = 0;
  fParam->Clear(opt);
  if ( fAssumeDate )
    SetDate(fDate);
  
  if( doing_init ) {
    if( !fAssumeDate )
      ClearDate();
  } else {
    fName = NOTINIT;
    fMaxScan = MAXSCAN;
    ClearEventRange();
    CloseFile();
  }
}

//_____________________________________________________________________________
void THaRun::ClearDate()
{
  // Reset the run date to "undefined"

  fDate.Set(UNDEFDATE,0);
  fAssumeDate = fIsInit = kFALSE;
  fDataSet &= ~kDate;
}

//_____________________________________________________________________________
void THaRun::SetDate( const TDatime& date )
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
void THaRun::SetDate( UInt_t tloc )
{
  // Set timestamp of this run to 'tloc' which is in Unix time
  // format (number of seconds since 01 Jan 1970).

#if ROOT_VERSION_CODE >= ROOT_VERSION(3,1,6)
  TDatime date( tloc );
#else
  time_t t = tloc;
  struct tm* tp = localtime(&t);
  date.Set( tp->tm_year, tp->tm_mon+1, tp->tm_mday,
	    tp->tm_hour, tp->tm_min, tp->tm_sec );
#endif
  SetDate( date );
}

//_____________________________________________________________________________
void THaRun::SetNumber( Int_t number )
{
  // Change/set the number of the Run.

  fNumber = number;
  char str[16];
  sprintf( str, "RUN_%u", number);
  SetName( str );
  fDataSet |= kRunNumber;
}

//_____________________________________________________________________________
void THaRun::SetType( Int_t type )
{
  // Set run type
  fType = type;
  fDataSet |= kRunType;
}

//_____________________________________________________________________________
void THaRun::SetFilename( const char* name )
{
  // Set the name of the raw data file.
  // If the name changes, the run will become uninitialized.

  if( !name || fFilename != name )
    fIsInit = kFALSE;

  fFilename = name;
}

//_____________________________________________________________________________
void THaRun::ClearEventRange()
{
  // Reset the range of events to analyze

  fEvtRange[0] = 1;
  fEvtRange[1] = UINT_MAX;
}

//_____________________________________________________________________________
ClassImp(THaRun)
