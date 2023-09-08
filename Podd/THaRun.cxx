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
#include "DAQconfig.h"
#include "THaPrintOption.h"
#include "TClass.h"
#include "TError.h"
#include "TSystem.h"
#include "TRegexp.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <stdexcept>
#include <sstream>

using namespace std;

#if __cplusplus >= 201402L
# define MKCODAFILE make_unique<Decoder::THaCodaFile>()
#else
# define MKCODAFILE unique_ptr<Decoder::THaCodaFile>(new Decoder::THaCodaFile)
#endif

static const int         fgMaxScan   = 5000;
static const char* const fgRe1       ="\\.[0-9]+$";
static const char* const fgRe2       ="\\.[0-9]+\\.[0-9]+$";

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* description )
  : THaCodaRun(description)
  , fFilename(fname)
  , fMaxScan(fgMaxScan)
  , fSegment(-1)
  , fStream(-1)
{
  // Normal & default constructor

  fCodaData = MKCODAFILE;  //Specifying the file name would open the file

  // Hall A runs normally contain all these items
  fDataRequired = kDate|kRunNumber|kRunType|kPrescales;
}

//_____________________________________________________________________________
THaRun::THaRun( const vector<TString>& pathList, const char* filename,
                const char* description )
  : THaCodaRun(description)
  , fMaxScan(fgMaxScan)
  , fSegment(-1)
  , fStream(-1)
{
  //  cout << "Looking for file:\n";
  for(const auto & path : pathList) {
    fFilename = Form("%s/%s", path.Data(), filename );
    //cout << "\t'" << fFilename << "'" << endl;

    if( !gSystem->AccessPathName(fFilename, kReadPermission) )
      break;
  }
  //cout << endl << "--> Opening file:  " << fFilename << endl;

  fCodaData = MKCODAFILE;  //Specifying the file name would open the file

  // Hall A runs normally contain all these items
  fDataRequired = kDate|kRunNumber|kRunType|kPrescales;
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs )
  : THaCodaRun(rhs)
  , fFilename(rhs.fFilename)
  , fMaxScan(rhs.fMaxScan)
  , fSegment(rhs.fSegment)
  , fStream(rhs.fStream)
{
  // Copy ctor

  fCodaData = MKCODAFILE;
}

//_____________________________________________________________________________
THaRun& THaRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator. We allow assignment to generic THaRunBase objects
  // so that we can copy run objects through a virtual operator=.
  // If the actual class of 'rhs' is different from this object's
  // (e.g. THaOnlRun - an ET run), then its special properties are lost.

  if( this != &rhs ) {
    THaCodaRun::operator=(rhs);
    fCodaData = MKCODAFILE;
    try {
      const auto& run = dynamic_cast<const THaRun&>(rhs);
      fFilename = run.fFilename;
      fMaxScan  = run.fMaxScan;
      fSegment  = run.fSegment;
      fStream   = run.fStream;
    }
    catch( const std::bad_cast& ) {
      fFilename.Clear();  // will need to call SetFilename()
      fMaxScan = fgMaxScan;
      fSegment = -1;
      fStream = -1;
    }
  }
  return *this;
}

//_____________________________________________________________________________
THaRun::~THaRun() = default;

//_____________________________________________________________________________
void THaRun::Clear( Option_t* opt )
{
  // Reset the run object.

  THaPrintOption sopt(opt);
  sopt.ToUpper();
  bool doing_init = (sopt.Contains("INIT"));

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
  const auto* rhs = dynamic_cast<const THaRunBase*>(obj);
  if( !rhs ) return -1;
  if( *this < *rhs )       return -1;
  else if( *rhs < *this )  return  1;
  const auto* rhsr = dynamic_cast<const THaRun*>(rhs);
  if( !rhsr ) return 0;
  if( fSegment < rhsr->fSegment ) return -1;
  else if( rhsr->fSegment < fSegment ) return 1;
  if( fStream < rhsr->fStream ) return -1;
  else if( rhsr->fStream < fStream ) return 1;
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
  if( !FindSegmentNumber() ) {
    Error(here, "Malformed file name or program bug. Call expert.");
    return READ_FATAL;
  }

  fOpened = false;
  Int_t st = fCodaData->codaOpen( fFilename );
  if( st == CODA_OK ) {
    // Get CODA version from data; however, if a version was set
    // explicitly by the user, use that instead
    if( GetCodaVersion() < 0 ) {
      Error( here, "Cannot determine CODA version from file. "
          "Try analyzer->SetCodaVersion(2)");
      st = CODA_ERROR;
      Close();
    }
    cout << "in THaRun::Open:  coda version "<<fDataVersion<<endl;
    if( st == CODA_OK )
      fOpened = true;
  }
  return ReturnCode( st );
}

//_____________________________________________________________________________
void THaRun::Print( Option_t* opt ) const
{
  THaPrintOption sopt(opt);
  sopt.ToUpper();
  if( sopt.Contains("NAMEDESC") ) {
    cout << "\"file://" << GetFilename() << "\"";
    if( strcmp( GetTitle(), "") != 0 )
      cout << "  \"" << GetTitle() << "\"";
    return;
  }
  THaCodaRun::Print( opt );
  cout << "CODA file:      " << fFilename << endl;
}

//_____________________________________________________________________________
Bool_t THaRun::ProvidesInitInfo()
{
  // Return true if this run /should/ contain the necessary data for
  // initialization (run date etc.)

  // Current configuration in Hall A/C:
  // - If there is no segment number, the run has init info (file not split)
  // - If there is a segment number, only segment 0 has the init info
  return (fSegment == -1 || fSegment == 0);
}

//_____________________________________________________________________________
Int_t THaRun::PrescanFile()
{
  static const char* const here = "THaRun::PrescanFile";
  // Scan at least 'minscan' number of events regardless of info required
  //FIXME: BCI: configurable member variable
  auto* ifo = DAQInfoExtra::GetExtraInfo(fExtra);
  const UInt_t minscan = ifo ? ifo->fMinScan : 50;

  unique_ptr<THaEvData> evdata{static_cast<THaEvData*>(gHaDecoder->New())};
  // Disable advanced processing
  evdata->EnableScalers(false);
  evdata->EnableHelicity(false);
  evdata->EnablePrescanMode(true);
  Int_t ver = GetCodaVersion();
  if( evdata->SetDataVersion(ver) <= 0 ) {
    Error( here, "Failed to set CODA version. Got %d, must be 2 or 3.", ver );
    return READ_FATAL;
  }
  UInt_t nev = 0;
  Int_t status = READ_OK;
  while( (nev < minscan || (nev < fMaxScan && !HasInfo(fDataRequired))) &&
         (status = ReadEvent()) == READ_OK ) {

    // Decode events. Skip bad events.
    nev++;
    status = evdata->LoadEvent(GetEvBuffer());
    if( status != THaEvData::HED_OK ) {
      if( status == THaEvData::HED_ERR || status == THaEvData::HED_FATAL ) {
        Error(here, "Error decoding event %u", nev);
        return (status == THaEvData::HED_ERR) ? READ_ERROR : READ_FATAL;
      }
      Warning(here, "Skipping event %u due to warnings", nev);
      status = READ_OK;
      continue;
    }

    // Inspect event and extract run parameters if appropriate
    Int_t st = Update(evdata.get());
    if( st < 0 )
      return READ_ERROR;
    if( st & (1<<0) )
      cout << "Prestart at " << nev << endl;
    if( st & (1<<1) )
      cout << "Prescales at " << nev << endl;
    if( st & (1<<2) )
      cout << "DAQ info at " << setw(2) << nev
           << " len " << evdata->GetEvLength() << endl;
  }//end while

  if( status != READ_OK )
    Error(here, "Failed to read CODA file at event %u", nev);
  return status;
}

//_____________________________________________________________________________
TString THaRun::GetInitInfoFileName( TString fname )
{
  // Based on the given file name, get the file name pattern that matches
  // runs containing initialization info.
  // This version simply changes the segment number to 0.

  // Extract the file base name
  Ssiz_t dot = fname.Last('.');
  if( dot != kNPOS) {
    fname.Remove(dot);
    fname.Append(".0");
  }

  return fname;
}

//_____________________________________________________________________________
TString THaRun::FindInitInfoFile( const TString& fname )
{
  // Find a file matching the name pattern returned by GetInitInfoFile().
  // The file must exist on disk and have at least read permissions.
  // Search in typical directories:
  // - First, in the same directory as the continuation segment.
  // - Then, if the filename's dirname contains dataN or workN, with N=0...9,
  //   also try looking in all other dataNs or workN. If dataN is found, ignore
  //   any workN in the file name.
  // Returns an empty string if no matching file can be found.

  TString s = GetInitInfoFileName(fname);
  if( !gSystem->AccessPathName(s, kReadPermission) )
    return s;

  TString dirn = gSystem->DirName(s);
  Ssiz_t pos = dirn.Index(TRegexp("data[0-9]"));
  if( pos == kNPOS )
    pos = dirn.Index(TRegexp("work[0-9]"));
  if( pos != kNPOS ) {
    TString sN = dirn(pos + 4, 1);
    Int_t curN = sN.Atoi();
    TString base = gSystem->BaseName(s);
    for( Int_t i = 0; i <= 9; i++ ) {
      if( i == curN )
        continue;
      dirn[pos+4] = TString::Itoa(i,10)[0];
      s = dirn + "/" + base;
      if( !gSystem->AccessPathName(s, kReadPermission) )
        return s;
    }
  }
  return {};  // empty string: not found
}

//_____________________________________________________________________________
Int_t THaRun::ReadInitInfo( Int_t level ) // NOLINT(misc-no-recursion)
{
  // Read initial info from the CODA file. This is done by prescanning
  // the run file in a local mini event loop via a local decoder.
  // 'level' is an optional recursion depth, default 0.

  static const char* const here = "ReadInitInfo";

  // If pre-scanning of the data file requested, read up to fMaxScan events
  // and extract run parameters from the data.
  if( fMaxScan == 0 )
    return READ_OK;

  Int_t status = READ_OK;

  if( ProvidesInitInfo() || level > 0 ) {
    status = PrescanFile();

    if( status != READ_OK && status != READ_EOF ) {
      Error(here, "Error %d reading CODA file %s.", status, GetFilename());
      return status;
    }

  } else {
    // If this is a continuation segment or parallel stream, try finding the
    // segment and/or stream which contains the initialization data.
    TString fname = FindInitInfoFile(fFilename);

    if( !fname.IsNull() ) {
      cout << "THaRun: Reading init info from " << fname << endl;
      unique_ptr<Decoder::THaCodaData> save_coda = std::move(fCodaData);
      fCodaData = MKCODAFILE;
      if( fCodaData->codaOpen(fname) == CODA_OK )
        status = ReadInitInfo(level+1);
      fCodaData = std::move(save_coda);
    }
  } //end if(fSegment==0)else

  return status;
}

//_____________________________________________________________________________
Int_t THaRun::SetFilename( const char* name )
{
  // Set the name of the raw data file.
  // If the name changes, the existing input, if any, will be closed.
  // Return -1 if illegal name, 1 if name not changed, 0 otherwise.

  static const char* const here = "SetFilename";

  if( !name || !*name ) {
    Error( here, "Illegal file name." );
    return -1;
  }

  if( fFilename == name )
    return 1;

  Close();

  fFilename = name;
  if( !FindSegmentNumber() ) {
    Error( here, "Illegal file name or program bug. Call expert.");
    return -1;
  }

  // Assume we have to reinitialize. A new file name generally means a new
  // run date, run number, etc.
  fIsInit = false;

  return 0;
}

//_____________________________________________________________________________
void THaRun::SetNscan( UInt_t n )
{
  // Set number of events to prescan during Init(). Default is 5000.

  fMaxScan = n;
}

//_____________________________________________________________________________
void THaRun::SetMinScan( UInt_t n )
{
  // Set minimum number of events to prescan during Init(), regardless of
  // fDataRequired. Default is 50.

  //FIXME: BCI make member variable
  auto* ifo = DAQInfoExtra::GetExtraInfo(fExtra);
  if( ifo )
    ifo->fMinScan = n;
}

//_____________________________________________________________________________
Bool_t THaRun::FindSegmentNumber()
{
  // Determine the segment number, if any. For typical CODA disk files, this is
  // the suffix of the file name, i.e. MM in name.MM, where MM can be one or
  // more digits.
  // Also set the stream number if the file name ends with .NN.MM, where
  // NN=stream, MM=segment.
  // Returns true if info successfully extracted

  fSegment = fStream = -1; // default -1 means unset
  if( fFilename.IsNull() )
    return false;

  TString dummy;
  return StdFindSegmentNumber(fFilename, dummy, fSegment, fStream);
}

//_____________________________________________________________________________
Bool_t THaRun::StdFindSegmentNumber( const TString& filename, TString& stem,
                                     Int_t& segment, Int_t& stream )
{
  try {
    TRegexp re1(fgRe1);
    TRegexp re2(fgRe2);
    if( re1.Status() != TRegexp::kOK || re2.Status() != TRegexp::kOK ) {
      ostringstream ostr;
      ostr << "Error compiling regular expressions: "
           << re1.Status() << " " << re2.Status() << endl;
      throw std::runtime_error(ostr.str());
    }
    Ssiz_t pos;
    if( (pos = filename.Index(re2)) != kNPOS ) {
      stem = filename(0, pos);
      ++pos;
      assert(pos < filename.Length());
      Ssiz_t pos2 = filename.Index(".", pos);
      assert(pos2 != kNPOS && pos2+1 < filename.Length());
      TString sStream = filename(pos, pos2 - pos);
      ++pos2;
      TString sSegmnt = filename(pos2, filename.Length() - pos2);
      stream  = sStream.Atoi();
      segment = sSegmnt.Atoi();
    } else if( (pos = filename.Index(re1)) != kNPOS ) {
      stem = filename(0, pos);
      ++pos;
      assert(pos < filename.Length());
      TString sSegmnt = filename(pos, filename.Length() - pos);
      stream = -1;
      segment = sSegmnt.Atoi();
    } else {
      // If neither expression matches, this run does not have the standard
      // segment/stream info in its file name. Leave both segment and stream
      // numbers unset and report success.
      segment = stream = -1;
    }
  }
  catch ( const exception& e ) {
    cerr << "Error parsing segment and/or stream number in run file name "
         << "\"" << filename << "\": " << e.what() << endl;
    segment = stream = -1;
    return false;
  }
  return true;
}

//_____________________________________________________________________________
ClassImp(THaRun)
