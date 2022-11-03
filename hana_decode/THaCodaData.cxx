/////////////////////////////////////////////////////////////////////
//
//   THaCodaData
//   Abstract class of CODA data.
//
//   THaCodaData is an abstract class of CODA data.  Derived
//   classes will be typically either a CODA file (a disk
//   file) or a connection to the ET system.  Public methods
//   allow to open (i.e. set up), read, write, etc.
//
//   author Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaData.h"
#include "Helper.h"
#include "TMath.h"
#include "evio.h"
#include <cassert>
#include <iostream>
#include <algorithm>

using namespace std;

namespace Decoder {

//_____________________________________________________________________________
THaCodaData::THaCodaData()
  : handle{0}
  , verbose{1}
  , fIsGood{true}
{}

//_____________________________________________________________________________
Int_t THaCodaData::getCodaVersion()
{
  // Get CODA version from current data source
  int32_t EvioVersion = 0;
  int status = evIoctl(handle, (char*)"v", &EvioVersion);
  fIsGood = (status == S_SUCCESS);
  if( status != S_SUCCESS ) {
    staterr("ioctl",status);
    codaClose();
    return -1;
  }
  //TODO debug message
  //cout << "Evio file EvioVersion = "<< EvioVersion << endl;
  return (EvioVersion < 4) ? 2 : 3;
}

//_____________________________________________________________________________
void THaCodaData::staterr(const char* tried_to, Int_t status) const
{
  // staterr gives the non-expert user a reasonable clue
  // of what the status returns from evio mean.
  // Note: severe errors can cause job to exit(0)
  // and the user has to pay attention to why.
  if (status == S_SUCCESS) return;  // everything is fine.
  if (status == EOF) {
    if (verbose > 0)
      cout << endl << "Normal end of file " << filename << " encountered"
           << endl;
    return;
  }
  cerr << Form("THaCodaFile: ERROR while trying to %s %s: ",
      tried_to, filename.Data());
  switch( static_cast<UInt_t>(status) ) {
  case S_EVFILE_TRUNC :
    cerr << "Truncated event on file read. Evbuffer size is too small. "
         << endl;
    break;
  case S_EVFILE_BADBLOCK :
    cerr << "Bad block number encountered " << endl;
    break;
  case S_EVFILE_BADHANDLE :
    cerr << "Bad handle (file/stream not open) " << endl;
    break;
  case S_EVFILE_ALLOCFAIL :
    cerr << "Failed to allocate event I/O" << endl;
    break;
  case S_EVFILE_BADFILE :
    cerr << "File format error" << endl;
    break;
  case S_EVFILE_UNKOPTION :
  case S_EVFILE_BADARG :
  case S_EVFILE_BADMODE :
    cerr << "Unknown function argument or option" << endl;
    break;
  case S_EVFILE_UNXPTDEOF :
    cerr << "Unexpected end of file while reading event" << endl;
    break;
  case S_EVFILE_BADSIZEREQ :
    cerr << "Invalid buffer size request to evIoct" << endl;
    break;
#if defined(S_EVFILE_BADHEADER)
  case S_EVFILE_BADHEADER :
    cerr << "Invalid bank/segment header data" << endl;
    break;
#endif
  default:
    cerr << "Unknown error " << hex << status << dec << endl;
    break;
  }
}

//_____________________________________________________________________________
Int_t THaCodaData::ReturnCode( Int_t evio_retcode )
{
  // Convert EVIO return codes to THaCodaData codes

  switch( static_cast<UInt_t>(evio_retcode) ) {

  case S_SUCCESS:
    return CODA_OK;

  case static_cast<UInt_t>(EOF):
    return CODA_EOF;

  case S_EVFILE_UNXPTDEOF:
  case S_EVFILE_TRUNC:
#if defined(S_EVFILE_BADHEADER)
  case S_EVFILE_BADHEADER:
#endif
    return CODA_ERROR;

  case S_EVFILE_BADBLOCK:
  case S_EVFILE_BADHANDLE:
  case S_EVFILE_ALLOCFAIL:
  case S_EVFILE_BADFILE:
  case S_EVFILE_BADSIZEREQ:
    return CODA_FATAL;

  // The following indicate a programming error and so should be trapped
  case S_EVFILE_UNKOPTION:
  case S_EVFILE_BADARG:
  case S_EVFILE_BADMODE:
    assert( false );
    return CODA_FATAL;

  default:
    return CODA_ERROR;
  }
}

//=============================================================================
static constexpr UInt_t kMaxBufSize = kMaxUInt / 16; // 1 GiB sanity size limit
static constexpr UInt_t kInitBufSize = 1024;         // 4 kiB initial size

// Dynamic event buffer class
EvtBuffer::EvtBuffer( UInt_t initial_size ) :
  fBuffer(std::max(std::min(initial_size, kMaxBufSize), kInitBufSize)),
  fMaxSaved(11),  // make this odd for faster calculation of median
  fNevents(0),
  fUpdateInterval(1000),
  fChanged(false),
  fDidGrow(false)
{
  fSaved.reserve(fMaxSaved);
}

//_____________________________________________________________________________
// Record fMaxSaved largest event sizes
void EvtBuffer::recordSize()
{
  UInt_t evtsize = fBuffer[0] + 1;

  if( fNevents < fMaxSaved || fSaved.empty() ) {
    fSaved.push_back(evtsize);
    fChanged = true;
  } else {
    auto it = min_element(ALL(fSaved));
    assert(it != fSaved.end());
    if( evtsize > *it ) {
      fChanged = true;
      if( fSaved.size() < fMaxSaved )
        fSaved.push_back(evtsize);
      else
        *it = evtsize;
    }
  }

  ++fNevents;
  fDidGrow = false;
}

//_____________________________________________________________________________
// Calculate median of values in vector 'vec'. Partially reorders 'vec'
template<typename T, typename Alloc>
static T Median( vector<T, Alloc>& vec )
{
  if( vec.empty() ) {
    return T{};
  }
  auto n = vec.size() / 2;
  nth_element( vec.begin(), vec.begin()+n, vec.end() );
  auto med = vec[n];
  if( !(vec.size() & 1) ) {
    // even number of elements
    auto med2 = *max_element( vec.begin(), vec.begin()+n );
    assert(med2 <= med);
    med = (med2 + med) / 2;
  }
  return med;
}

//_____________________________________________________________________________
// Evaluate whether event buffer is too large based on event size history.
// Shrink if necessary.
void EvtBuffer::updateImpl()
{
  // Detect outliers in the group of largest event sizes. To arrive at a
  // robust estimate, use median absolute deviation (MAD), not r.m.s., as the
  // measure of dispersion. See, for example, https://arxiv.org/abs/1910.00229.

  // Find median of saved event sizes.
  UInt_t median = Median(fSaved);

  // Calculate absolute deviations from median: devs[i] = |x[i]-median|
  VectorUIntNI devs(fMaxSaved);
  transform(ALL(fSaved), devs.begin(), [median]( UInt_t elem ) -> UInt_t {
    return std::abs((Long64_t)elem - (Long64_t)median);
  });

  // Calculate MAD and scale by the conventional normalization factor
  UInt_t MAD = TMath::Nint(1.4826 * Median(devs));

  // Take largest non-outlier event size as estimate of the needed buffer size
  UInt_t max_regular_event_size = median;
  if( MAD > 0 ) {
    constexpr UInt_t lambda = 3;  // outlier boundary in units of MADs
    auto n = fSaved.size()/2;
    sort(fSaved.begin()+n, fSaved.end());
    for( auto rt = fSaved.rbegin(); rt != fSaved.rbegin()+n; ++rt ) {
      UInt_t elem = *rt;
      if( elem <= median + lambda * MAD ) {
        max_regular_event_size = elem;
        break;
      }
    }
  }
  // Provision 20% headroom
  size_t newsize = std::min(UInt_t(1.2 * max_regular_event_size), kMaxBufSize);
  fBuffer.resize(newsize);

  // Shrink buffer if it could offer substantial memory savings
  if( fBuffer.capacity() > 2 * newsize )
    fBuffer.shrink_to_fit();

  fChanged = false;
}

//_____________________________________________________________________________
// Grow event buffer.
// If newsize == 0 (default), use heuristics to guess a new size.
// If newsize > size(), grow buffer to 'newsize'. Otherwise leave buffer as is.
//
// Returns false if buffer is already at maximum size.
Bool_t EvtBuffer::grow( UInt_t newsize )
{
  if( size() == kMaxBufSize )
    return false;

  if( newsize == 0 ) {
    UInt_t maybe_newsize = 0;
    if( !fDidGrow && fNevents > fUpdateInterval ) {
      // If we have accumulated history data, try a modest increase first
      maybe_newsize = 2 * Median(fSaved);
    }
    if( maybe_newsize > size() ) {
      newsize = maybe_newsize;
    } else if( fNevents < fUpdateInterval/10 ) {
      // Grow the buffer rapidly at the beginning of the analysis.
      // If we overshoot, updateSize() will shrink it again later on.
      newsize = 4 * size();
    } else {
      // Fallback for all other cases: Double the buffer size whenever it
      // is found too small and try again.
      newsize = 2 * size();
    }
  } else if( newsize <= size() ) {
    return true;
  }
  if( newsize > kMaxBufSize)
    newsize = kMaxBufSize;

  fBuffer.resize(newsize);
  fDidGrow = true;

  return true;
}

//_____________________________________________________________________________
// Reset to starting values
void EvtBuffer::reset()
{
  fBuffer.clear();
  fBuffer.shrink_to_fit();
  fSaved.clear();
  fChanged = false;
  fDidGrow = false;
}

}  // namespace Decoder

ClassImp(Decoder::THaCodaData)
