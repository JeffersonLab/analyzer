//*-- Author :    Ole Hansen   13/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
// Description of a CODA run on disk.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRun.h"
#include "THaEvData.h"
#include "THaCodaFile.h"
#include "THaGlobals.h"
#include "TClass.h"
#include "TError.h"
#include "TSystem.h"
#include "TRegexp.h"
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;
using namespace Decoder;

static const int   fgMaxScan   = 5000;
static const char* fgThisClass = "THaRun";

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* description ) :
  THaCodaRun(description), fFilename(fname), fMaxScan(fgMaxScan)
{
  // Normal & default constructor

  fCodaData = new THaCodaFile;  //Specifying the file name would open the file
  FindSegmentNumber();

  // Hall A runs normally contain all these items
  fDataRequired = kDate|kRunNumber|kRunType|kPrescales;
}

//_____________________________________________________________________________
THaRun::THaRun( const vector<TString>& pathList, const char* filename,
		const char* description )
  : THaCodaRun(description), fMaxScan(fgMaxScan)
{
  //  cout << "Looking for file:\n";
  for(vector<TString>::size_type i=0; i<pathList.size(); i++) {
    fFilename = Form( "%s/%s", pathList[i].Data(), filename );
    //cout << "\t'" << fFilename << "'" << endl;

    if( !gSystem->AccessPathName(fFilename, kReadPermission) )
      break;
  }
  //cout << endl << "--> Opening file:  " << fFilename << endl;

  fCodaData = new THaCodaFile;  //Specifying the file name would open the file
  FindSegmentNumber();

  // Hall A runs normally contain all these items
  fDataRequired = kDate|kRunNumber|kRunType|kPrescales;
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) :
  THaCodaRun(rhs), fFilename(rhs.fFilename), fMaxScan(rhs.fMaxScan)
{
  // Copy ctor

  fCodaData = new THaCodaFile;
  FindSegmentNumber();
}

//_____________________________________________________________________________
THaRun& THaRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator. We allow assignment to generic THaRun objects
  // so that we can copy run objects through a virtual operator=.
  // If 'rhs' is of different actual class (e.g. THaOnlRun - an ET run)
  // than this object, then its special properties are lost.

  if (this != &rhs) {
     THaCodaRun::operator=(rhs);
     //     delete fCodaData; //already done in THaCodaRun
     fCodaData   = new THaCodaFile;
     if( rhs.InheritsFrom(fgThisClass) ) {
       fFilename   = static_cast<const THaRun&>(rhs).fFilename;
       fMaxScan    = static_cast<const THaRun&>(rhs).fMaxScan;
       FindSegmentNumber();
     } else {
       fMaxScan    = fgMaxScan;
       fSegment    = 0;
     }
  }
  return *this;
}

//_____________________________________________________________________________
THaRun::~THaRun()
{
  // Destructor.

}

//_____________________________________________________________________________
void THaRun::Clear( Option_t* opt )
{
  // Reset the run object.

  TString sopt(opt);
  bool doing_init = (sopt == "INIT");

  THaCodaRun::Clear(opt);

  // If initializing, keep explicitly set fMaxScan
  if( !doing_init )
    fMaxScan = fgMaxScan;
}

//_____________________________________________________________________________
Int_t THaRun::Compare( const TObject* obj ) const
{
  // Compare a THaRun object to another run. Returns 0 when equal,
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).
  // Used by ROOT containers.

  if (this == obj) return 0;
  const THaRunBase* rhs = dynamic_cast<const THaRunBase*>(obj);
  if( !rhs ) return -1;
  if( *this < *rhs )       return -1;
  else if( *rhs < *this )  return  1;
  const THaRun* rhsr = dynamic_cast<const THaRun*>(rhs);
  if( !rhsr ) return 0;
  if( fSegment < rhsr->fSegment ) return -1;
  else if( rhsr->fSegment < fSegment ) return 1;
  return 0;
}

//_____________________________________________________________________________
Int_t THaRun::Open()
{
  // Open CODA file for read-only access.

  static const char* const here = "Open";

  if( fFilename.IsNull() ) {
    Error( here, "CODA file name not set. Cannot open the run." );
    return READ_FATAL;  // filename not set
  }

  Int_t st = fCodaData->codaOpen( fFilename );
// Get fCodaVersion from data; however, if it was already defined
// by the analyzer script use that instead.
  if (fCodaVersion == 0) fCodaVersion = fCodaData->getCodaVersion();
  cout << "in THaRun::Open:  coda version "<<fCodaVersion<<endl;
  if( st == 0 )
    fOpened = kTRUE;
  return ReturnCode( st );
}

//_____________________________________________________________________________
void THaRun::Print( Option_t* opt ) const
{
  THaCodaRun::Print( opt );
  cout << "Max # scan:     " << fMaxScan  << endl;
  cout << "CODA file:      " << fFilename << endl;
  cout << "Segment number: " << fSegment  << endl;
}

//_____________________________________________________________________________
Int_t THaRun::ReadInitInfo()
{
  // Read initial info from the CODA file. This is done by prescanning
  // the run file in a local mini event loop via a local decoder.

  static const char* const here = "ReadInitInfo";

  // If pre-scanning of the data file requested, read up to fMaxScan events
  // and extract run parameters from the data.
  Int_t status = READ_OK;
  if( fMaxScan > 0 ) {
    if( fSegment == 0 ) {
      THaEvData* evdata = static_cast<THaEvData*>(gHaDecoder->New());
      // Disable advanced processing
      evdata->EnableScalers(kFALSE);
      evdata->EnableHelicity(kFALSE);
      evdata->SetCodaVersion(fCodaVersion);
      UInt_t nev = 0;
      while( nev<fMaxScan && !HasInfo(fDataRequired) &&
	     (status = ReadEvent()) == READ_OK ) {

	// Decode events. Skip bad events.
	nev++;
	status = evdata->LoadEvent( GetEvBuffer());
	if( status != THaEvData::HED_OK ) {
	  if( status == THaEvData::HED_ERR ||
	      status == THaEvData::HED_FATAL ) {
	    Error( here, "Error decoding event %u", nev );
	    status = READ_ERROR;
	    break;
	  }
	  Warning( here, "Skipping event %u due to warnings", nev );
	  status = READ_OK;
	  continue;
	}

	// Inspect event and extract run parameters if appropriate
	Int_t st = Update( evdata );
	//FIXME: debug
	if( st == 1 )
	  cout << "Prestart at " << nev << endl;
	else if( st == 2 )
	  cout << "Prescales at " << nev << endl;
	else if ( st < 0 ) {
	  status = READ_ERROR;
	  break;
	}
      }//end while
      delete evdata;

      if( status != READ_OK && status != READ_EOF ) {
	Error( here, "Error %d reading CODA file %s. Check file type & "
	       "permissions.", status, GetFilename());
	return status;
      }

    } else {
      // If this is a continuation segment, try finding segment 0
      // Since this class is Hall A-specific, look in typical directories.
      // First look in the same directory as the continuation segment.
      // If the filename's dirname contains dataN, with N=1...9, also look in
      // all other dataN's.
      Ssiz_t dot = fFilename.Last('.');
      assert( dot != kNPOS );  // if fSegment>0, there must be a dot
      TString s = fFilename(0,dot);
      s.Append(".0");
      vector<TString> fnames;
      fnames.push_back(s);
      TString dirn = gSystem->DirName(s);
      Ssiz_t pos = dirn.Index( TRegexp("data[1-9]") );
      if( pos != kNPOS ) {
	fnames.reserve(10);
	TString sN = dirn(pos+4,1);
	Int_t curN = atoi(sN.Data());
	TString base = gSystem->BaseName(s);
	for( Int_t i = 1; i <= 9; i++ ) {
	  if( i == curN )
	    continue;
	  dirn.Replace( pos, 5, Form("data%d",i) );
	  s = dirn + "/" + base;
	  fnames.push_back(s);
	}
      }
      for( vector<TString>::size_type i = 0; i < fnames.size(); ++i ) {
	s = fnames[i];
	if( !gSystem->AccessPathName(s, kReadPermission) ) {
	  THaCodaData* save_coda = fCodaData;
	  Int_t        save_seg  = fSegment;
	  fCodaData = new THaCodaFile;
	  fSegment  = 0;
	  if( fCodaData->codaOpen(s) == 0 )
	    status = ReadInitInfo();
          fCodaVersion = fCodaData->getCodaVersion();
	  delete fCodaData;
	  fSegment  = save_seg;
	  fCodaData = save_coda;
	  break;
	}
      }
    } //end if(fSegment==0)else
  } //end if(fMaxScan>0)

  return status;
}

//_____________________________________________________________________________
Int_t THaRun::SetFilename( const char* name )
{
  // Set the name of the raw data file.
  // If the name changes, the existing input, if any, will be closed.
  // Return -1 if illegal name, 1 if name not changed, 0 otherwise.

  static const char* const here = "SetFilename";

  if( !name ) {
    Error( here, "Illegal file name." );
    return -1;
  }

  if( fFilename == name )
    return 1;

  Close();
  fFilename = name;
  FindSegmentNumber();

  // The run becomes uninitialized only if this is not a continuation segment
  if( fSegment == 0 )
    fIsInit = kFALSE;

  return 0;
}

//_____________________________________________________________________________
void THaRun::SetNscan( UInt_t n )
{
  // Set number of events to prescan during Init(). Default is 5000.

  fMaxScan = n;
}

//_____________________________________________________________________________
Int_t THaRun::FindSegmentNumber()
{
  // Determine the segment number, if any. For Hall A CODA disk files, we can
  // safely assume that the suffix of the file name will tell us.
  // Internal function.

  Ssiz_t dot = fFilename.Last('.');
  if( dot != kNPOS ) {
    TString s = fFilename(dot+1,fFilename.Length()-dot-1);
    fSegment = atoi(s.Data());
  } else
    fSegment = 0;

  return fSegment;
}

//_____________________________________________________________________________
ClassImp(THaRun)
