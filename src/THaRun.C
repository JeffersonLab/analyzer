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
#include "THaCodaFile.h"
#include "TMath.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>

ClassImp(THaRun)

//_____________________________________________________________________________
THaRun::THaRun() : TNamed(), fNumber(0), 
  fBeamE(0.0), fBeamP(0.0), fBeamM(0.0), fBeamQ(0), fBeamdE(0.0), fTarget(0)
{
  // Default constructor

  fCodaFile = 0;
  ClearEventRange();
}

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* descr ) : 
  TNamed("", strlen(descr) ? descr : fname), fNumber(0), fFilename(fname),
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
void THaRun::Copy( TObject& rhs )
{
  // Copy this THaRun to 'run'

  THaRun& run = static_cast<THaRun&>(rhs);
  run = *this;
}

//_____________________________________________________________________________
//void THaRun::FillBuffer( char*& buffer )
//  {
//    // Encode THaRun into output buffer.

//    TNamed::FillBuffer( buffer );
//    fFilename.FillBuffer( buffer );
//  }

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
  // FIXME: use preprocessor ifdef
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
