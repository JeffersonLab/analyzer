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
#include "THaCodaFile.h"
#include "TMath.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>

ClassImp(THaRun)

//_____________________________________________________________________________
THaRun::THaRun() : TNamed(), fNumber(0), fDBRead(false),
  fBeamE(0.0), fBeamP(0.0), fBeamM(0.0), fBeamQ(0), fBeamdE(0.0), fTarget(0)
{
  // Default constructor

  fCodaFile = 0;
  ClearEventRange();
}

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* descr ) : 
  TNamed("", strlen(descr) ? descr : fname), fNumber(0), fFilename(fname),
  fDBRead(false),
  fBeamE(0.0), fBeamP(0.0), fBeamM(0.0), fBeamQ(0), fBeamdE(0.0), fTarget(0)
{
  // Normal constructor

  fCodaFile = new THaCodaFile;  //Do not open the file yet
  ClearEventRange();
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) : TNamed( rhs )
{
  // Copy ctor

  fNumber     = rhs.fNumber;
  fFilename   = rhs.fFilename;
  fDate       = rhs.fDate;
  fFirstEvent = rhs.fFirstEvent;
  fLastEvent  = rhs.fLastEvent;
  fCodaFile   = new THaCodaFile;
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
     fFilename   = rhs.fFilename;
     fFirstEvent = rhs.fFirstEvent;
     fLastEvent  = rhs.fLastEvent;
     fDBRead     = rhs.fDBRead;
     delete fCodaFile;
     fCodaFile   = new THaCodaFile;
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

  delete fCodaFile;
}

//_____________________________________________________________________________
Int_t THaRun::CloseFile()
{
  return fCodaFile ? fCodaFile->codaClose() : 0;
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
  // by fCodaFile->codaRead()

  return fCodaFile ? fCodaFile->getEvBuffer() : 0; 
}

//_____________________________________________________________________________
Int_t THaRun::OpenFile()
{
  // Open CODA file for read-only access.

  if( fFilename.IsNull() )
    return -2;  // filename not set

  if( !fCodaFile ) fCodaFile = new THaCodaFile;
  if( !fCodaFile ) return -3;
  return fCodaFile->codaOpen( fFilename );
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

  return fCodaFile ? fCodaFile->codaRead() : -1;
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
void THaRun::SetDate( UInt_t tloc )
{
  // Set timestamp of this run to 'tloc' which is in Unix time
  // format (number of seconds since 01 Jan 1970).

  // fDate.Set(tloc) only supported by ROOT>=3.01/06, so we 
  // code it by hand for now.
  // FIXME: use preprocessor ifdef? This is Unix-specific!
  time_t t = tloc;
  struct tm* tp = localtime(&t);
  fDate.Set( tp->tm_year, tp->tm_mon+1, tp->tm_mday, 
	     tp->tm_hour, tp->tm_min, tp->tm_sec );
}

//_____________________________________________________________________________
void THaRun::SetNumber( UInt_t number )
{
  // Change/set the number of the Run.

  fNumber = number;
  char str[16];
  sprintf( str, "RUN_%u", number);
  SetName( str );
}
