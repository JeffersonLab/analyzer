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
#include "THaAnalysisObject.h"
#include "THaEvData.h"
#include "THaCodaData.h"
#include "THaCodaFile.h"
#include "TMath.h"
#include <iostream>
#include <cstring>
#include <cstdio>

using namespace std;

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* descr ) : 
  TNamed("", strlen(descr) ? descr : fname), fNumber(0), fFilename(fname),
  fDate(19950101,0), fAssumeDate(kFALSE), fDBRead(kFALSE), fIsInit(kFALSE),
  fBeamE(0.0), fBeamP(0.0), fBeamM(0.0), fBeamQ(0), fBeamdE(0.0), fTarget(0)
{
  // Normal & default constructor

  fCodaData = new THaCodaFile;  //Do not open the file yet
  ClearEventRange();
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) : TNamed( rhs )
{
  // Copy ctor

  fNumber     = rhs.fNumber;
  fFilename   = rhs.fFilename;
  fDate       = rhs.fDate;
  fAssumeDate = rhs.fAssumeDate;
  fFirstEvent = rhs.fFirstEvent;
  fLastEvent  = rhs.fLastEvent;
  fCodaData   = new THaCodaFile;
  fDBRead     = rhs.fDBRead;
  fBeamE      = rhs.fBeamE;
  fBeamP      = rhs.fBeamP;
  fBeamM      = rhs.fBeamM;
  fBeamQ      = rhs.fBeamQ;
  fBeamdE     = rhs.fBeamdE;
  fTarget     = rhs.fTarget;  // Target object managed externally
}

//_____________________________________________________________________________
THaRun& THaRun::operator=(const THaRun& rhs)
{
  // THaRun assignment operator.

  if (this != &rhs) {
     TNamed::operator=(rhs);
     fNumber     = rhs.fNumber;
     fDate       = rhs.fDate;
     fAssumeDate = rhs.fAssumeDate;
     fFilename   = rhs.fFilename;
     fFirstEvent = rhs.fFirstEvent;
     fLastEvent  = rhs.fLastEvent;
     fDBRead     = rhs.fDBRead;
     delete fCodaData;
     fCodaData   = new THaCodaFile;
     fBeamE      = rhs.fBeamE;
     fBeamP      = rhs.fBeamP;
     fBeamM      = rhs.fBeamM;
     fBeamQ      = rhs.fBeamQ;
     fBeamdE     = rhs.fBeamdE;
     fTarget     = rhs.fTarget;
  }
  return *this;
}

//_____________________________________________________________________________
THaRun::~THaRun()
{
  // Destroy the Run. The CODA file will be closed by the THaCodaFile 
  // destructor if necessary.

  delete fCodaData; fCodaData = 0;
}

//_____________________________________________________________________________
Int_t THaRun::CloseFile()
{
  return fCodaData ? fCodaData->codaClose() : 0;
}

//_____________________________________________________________________________
Int_t THaRun::Init()
{
  // Initialize the run. This reads the run database, checks 
  // whether the data source can be opened, and if so, initializes the
  // run parameters (run number, time, etc.)

  static const char* const here = "Init()";

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

  // Determine the date of this run
  // If the user gave us a date via SetDate(), we're done; otherwise we
  // need to search for an event with a timestamp. 
  // For CODA files, we need a prestart event. We search for a maximum of 
  // 1000 events, then give up.
  if( !fAssumeDate ) {
    THaEvData evdata;
    const Int_t MAXEV = 1000;
    Int_t nev = 0;
    Int_t status = 0;
    while (status != EOF && status != CODA_ERROR) {

      status = ReadEvent();
      if( status == S_EVFILE_TRUNC || status == CODA_ERROR ) {
	Error( here, "Error %d reading CODA file %s. Check file permissions.",
	       status, GetFilename());
	return status;
      }

      // Decode the event
      nev++;
      evdata.LoadEvent( GetEvBuffer() );
      if( evdata.IsPrestartEvent())
	// Got a good event! Get out of here.
	break;
      if( nev < MAXEV )
	continue;
      else {
	Error( here, "Cannot find a timestamp within the first %d events "
	       "in CODA file %s.\nMake sure the file is not a continuation "
	       " segment.", MAXEV, GetFilename());
	return 255;
      }
    }
    fDate.Set( evdata.GetRunTime() );
    SetNumber( evdata.GetRunNum() );
  }

  // Close the data source to avoid conflicts
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
bool THaRun::IsOpen() const
{
  return fCodaData ? fCodaData->isOpen() : false;
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
Int_t THaRun::OpenFile()
{
  // Open CODA file for read-only access.

  if( fFilename.IsNull() )
    return -2;  // filename not set

  if( !fCodaData ) fCodaData = new THaCodaFile;
  if( !fCodaData ) return -3;
  return fCodaData->codaOpen( fFilename );
}

//_____________________________________________________________________________
Int_t THaRun::OpenFile( const char* filename )
{
  // Set the filename and then open CODA file.

  SetFilename( filename );
  return OpenFile();
}

//_____________________________________________________________________________
void THaRun::Print( Option_t* opt ) const
{
  // Print definition of run
  TNamed::Print( opt );
  cout << "File name:  " << fFilename.Data() << endl;
  cout << "Run number: " << fNumber << endl;
  cout << "Run date:   " << fDate.AsString() << endl;
}

//_____________________________________________________________________________
Int_t THaRun::ReadDatabase()
{
  // Qeury the run database for the beam and target parameters for the 
  // date/time of this run.  The run date should have been set, otherwise
  // the time when the run object was created (usually shortly before the 
  // current time) will be used, which is often not meaningful.
  //
  // Return 0 if success, <0 if file error, >0 if not all required data found.

#define OPEN THaAnalysisObject::OpenFile
#define READ THaAnalysisObject::LoadDBvalue

  FILE* f = OPEN( "run", fDate, "THaRun::ReadDatabase()" );
  if( !f ) return -1;
  Int_t iq, st;
  Double_t E, M = 0.511e-3, Q = -1.0, dE = 0.0;

  if( (st = READ( f, fDate, "Ebeam", E )) ) return st; // Beam energy required
  READ( f, fDate, "mbeam", M );
  READ( f, fDate, "qbeam", Q );
  READ( f, fDate, "dEbeam", dE );
  iq = int(Q);
  SetBeam( E, M, iq, dE );

  // FIXME: Read target parameters and create structure.
  fTarget = NULL;
  fDBRead = true;

  fclose(f);
  return 0;
}
  
//_____________________________________________________________________________
Int_t THaRun::ReadEvent()
{
  // Read one event from CODA file.

  return fCodaData ? fCodaData->codaRead() : -1;
}

//_____________________________________________________________________________
void THaRun::SetBeam( Double_t E, Double_t M, Int_t Q, Double_t dE )
{
  // Set beam parameters.
  fBeamE = E;
  fBeamM = M;
  fBeamQ = Q;
  fBeamP = (E>M) ? TMath::Sqrt(E*E-M*M) : 0.0;
  if( fBeamP == 0.0 )
    Warning( "SetBeam()", "Beam momentum = 0 ??");
  fBeamdE = dE;
}

//_____________________________________________________________________________
void THaRun::ClearDate()
{
  // Reset the run date to "undefined" (1.1.1995)

  fDate.Set(19950101,0);
  fAssumeDate = fIsInit = kFALSE;
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
}

//_____________________________________________________________________________
void THaRun::SetDate( UInt_t tloc )
{
  // Set timestamp of this run to 'tloc' which is in Unix time
  // format (number of seconds since 01 Jan 1970).

  // NB: only supported by ROOT>=3.01/06
  TDatime date( tloc );
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
}

//_____________________________________________________________________________
void THaRun::SetFilename( const char* name )
{
  // Set the name of the run. If the name changes, the run will become
  // uninitialized.

  if( name && fName != name )
    fIsInit = kFALSE;

  fName = name;
}

//_____________________________________________________________________________
ClassImp(THaRun)
