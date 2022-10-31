////////////////////////////////////////////////////////////////////
//
//   PipeliningModule
//   A JLab PipeLining Module.  See header file for more comments.
//   R. Michaels, Oct 2016
//
/////////////////////////////////////////////////////////////////////

#include "PipeliningModule.h"
#include "THaSlotData.h"
#include "Helper.h"
#include <iostream>
#include <cstring>     // for memcpy
#include <algorithm>   // for std::transform
#include <utility>     // for std::swap

using namespace std;

//_____________________________________________________________________________
// Helper function for debugging
static void PrintBlock( const uint32_t* codabuffer, uint32_t pos, uint32_t len )
{
  size_t idx = pos;
  while( idx < pos+len ) {
    while( idx < pos+len && !TESTBIT(codabuffer[idx], 31) )
      ++idx;
    if( idx == pos+len )
      break;
    uint32_t data = codabuffer[idx];
    uint32_t type = (data >> 27) & 0xF;
    switch( type ) {
      case 0:
        cout << "Block header"
             << " idx = " << idx
             << " slot = " << ((data >> 22) & 0x1F)
             << " blksz = " << ((data >> 0) & 0xFF)
             << " iblkn = " << ((data >> 8) & 0x3FF);
        break;
      case 1:
        cout << "Block trailer"
             << " idx = " << idx
             << " slot = " << ((data >> 22) & 0x1F)
             << " nwords = " << (data & 0x3FFFFF);
        break;
      case 2:
        cout << " Event header"
             << " idx = " << idx
             << " slot = " << ((data >> 22) & 0x1F)
//             << " nevt = " << (data & 0x3FFFFF);
             << " time = " << ((data >>12) & 0x3FF)
             << " trignum = " << (data & 0xFFF);
        break;
      default:
        cout << "  Type = " << type
             << " idx = " << idx;
        break;
    }
    cout << endl;
    ++idx;
  }
}

//_____________________________________________________________________________
namespace Decoder {

//_____________________________________________________________________________
PipeliningModule::PipeliningModule( UInt_t crate, UInt_t slot )
  : VmeModule(crate, slot),
    fBlockHeader(0),
    data_type_def(15),  // initialize to FILLER WORD
    index_buffer(0)
{
}

//_____________________________________________________________________________
void PipeliningModule::Init( const char* configstr )
{
  // Set debug level of this module via optional configuration string in
  // db_cratemap.dat.
  //
  // Example:
  // ==== Crate 30 type vme
  // # slot   model   bank   configuration string
  //   10      250    2501   cfg: debug=1

  Init();  // standard Init

  UInt_t debug = 0;
  vector<ConfigStrReq> req = { { "debug", debug } };
  ParseConfigStr(configstr, req);

  fDebug = static_cast<Int_t>(debug);
}

//_____________________________________________________________________________
void PipeliningModule::Clear( Option_t* opt )
{
  VmeModule::Clear(opt);
  evtblk.clear();
  index_buffer = 0;
}

//_____________________________________________________________________________
UInt_t PipeliningModule::LoadBlock( THaSlotData* sldat,
                                    const UInt_t* evbuffer,
                                    const UInt_t* pstop )
{
  // Load event block. If multi-block data, split the buffer into individual
  // physics events.
  // This routine is called for legacy-type buffers without bank structure.
  // It currently just forwards to the bank-structure version, assuming the
  // data format within the block is the same in both cases.

  //TODO: is multiblock mode supported for non-bank data?
  return LoadBank(sldat, evbuffer, 0, pstop+1-evbuffer);
}

//_____________________________________________________________________________
Long64_t PipeliningModule::VerifyBlockTrailer(
  const UInt_t* evbuffer, UInt_t pos, UInt_t len, Long64_t ibeg,
  Long64_t iend ) const
{
  if( iend > 0 ) {
    // Verify that the word count reported in the block trailer agrees with
    // the trailer's position in the buffer.
    UInt_t nwords_inblock = evbuffer[iend] & 0x3FFFFF;
    if( ibeg + nwords_inblock == iend + 1 )
      // All good
      return iend;
  } else {
    // Block header without matching block trailer, should not happen
    goto notfound;
  }
  // Apparent misidentification: keep searching until hitting the buffer end
  if( ++iend < pos+len ) {
    cerr << "WARNING: Block trailer misidentification, slot " << fSlot
         << ", roc " << fCrate << ", data 0x" << hex << evbuffer[iend-1] << dec
         << ". Attempting recovery." << endl;
    return -iend;
  } else {
 notfound:
    cerr << "ERROR: Block trailer NOT found, slot " << fSlot
         << ", roc " << fCrate << ". Corrupt data. Giving up." << endl;
    cout << "Block data:" << endl;
    PrintBlock(evbuffer, pos, len);
    return 0;
  }
}

//_____________________________________________________________________________
UInt_t PipeliningModule::LoadBank( THaSlotData* sldat,
                                   const UInt_t* evbuffer,
                                   UInt_t pos, UInt_t len )
{
  // Load event block. If multi-block data, split the buffer into individual
  // physics events.
  // This routine is called for buffers with bank structure.

  if( fDebug > 1 )  // Set fDebug via module config string in db_cratemap.dat
    PrintBlock(evbuffer,pos,len);

  // Find block for this module's slot
  auto ibeg = FindIDWord(evbuffer, pos, len, kBlockHeader, fSlot);
  if( ibeg == -1 )
    // Slot not present in buffer (OK)
    return 0;
  fBlockHeader = evbuffer[ibeg];  // save for convenience

  // Multi-block event?
  block_size = (evbuffer[ibeg] & 0xFF);  // Number of events in block
  fMultiBlockMode = ( block_size > 1 );

  if( fMultiBlockMode ) {
    // Multi-block: Find all the event headers (there should be block_size
    // of them) between here and the block trailer, save their positions,
    // then proceed with decoding the first block
    evtblk.reserve(block_size + 1);
    Long64_t iend = ibeg+1;
    while( true ) {
      iend = FindEventsInBlock(evbuffer, iend, len+pos-iend,
                               kEventHeader, kBlockTrailer, evtblk, fSlot);
      if( (iend = VerifyBlockTrailer(evbuffer, pos, len, ibeg, iend)) > 0 )
        break;
      if( iend == 0 )
        return 0;
      iend = -iend;
    }
    assert( ibeg >= pos && iend > ibeg && iend < pos+len ); // trivially

    if( evtblk.empty() )
      //TODO missing event headers, should not happen
      evtblk.push_back(ibeg+1);
    // evtblk should have exactly block_size elements now
    evtblk.push_back(iend + 1); // include block trailer

    // Because our module decoders expect a block header at the start of every
    // event block, we must unfortunately copy the event block here so that we
    // have a writable buffer where we can prepend the block header.
    // This could probably be avoided, but we'd have to rewrite all decoders.
    size_t blklen = iend + 1 - ibeg;
    fBuffer.resize(blklen);
    memcpy(fBuffer.data(), evbuffer+ibeg, blklen * sizeof(fBuffer[0]));
    // Adjust the saved event header locations such that they reference the
    // corresponding locations in fBuffer
    transform(ALL(evtblk), evtblk.begin(),
              [ibeg]( UInt_t pos ) { return pos-ibeg; });

    index_buffer = 0;
    return LoadNextEvBuffer(sldat);

  } else {
    // Single block: Find end of block and let the module decode the event
    Long64_t iend = ibeg+1;
    while( true ) {
      iend = FindIDWord(evbuffer, iend, len+pos-iend,
                        kBlockTrailer, fSlot);
      if( (iend = VerifyBlockTrailer(evbuffer, pos, len, ibeg, iend)) > 0 )
        break;
      if( iend == 0 )
        return 0;
      iend = -iend;
    }
    assert( ibeg >= pos && iend > ibeg && iend < pos+len ); // trivially

    return LoadSlot(sldat, evbuffer, ibeg, iend+1-ibeg);
  }
  // not reached
}

//_____________________________________________________________________________
UInt_t PipeliningModule::LoadNextEvBuffer( THaSlotData* sldat )
{
  // In multi-block mode, load the next event from the current block

  UInt_t ii = index_buffer;
  assert( ii+1 < evtblk.size() );

  // ibeg = event header, iend = one past last word of this event ( = next
  // event header if more events pending)
  auto ibeg = evtblk[ii], iend = evtblk[ii+1];
  assert(ibeg > 0 && iend > ibeg && static_cast<size_t>(iend) <= fBuffer.size());

  // Let ibeg point to the block header, or one before event header
  if( ii == 0 )
    ibeg = 0;
  else {
    --ibeg;
    // Ensure the buffer starts with the block header word
    std::swap(fBlockHeader, fBuffer[ibeg]);
  }

  // Load slot starting with block header at ibeg
  try {
    ii = LoadSlot(sldat, fBuffer.data(), ibeg, iend-ibeg);
  }

  catch( ... ) {
    // In case the calling code wants to continue, put the buffer back in a
    // consistent state
    if( ii != 0 ) std::swap(fBlockHeader, fBuffer[ibeg]);
    throw;
  }
  if( ii != 0 ) std::swap(fBlockHeader, fBuffer[ibeg]);

  // Next cached buffer. Set flag if we've exhausted the cache.
  ++index_buffer;
  if( index_buffer+1 >= evtblk.size() )
    fBlockIsDone = true;

  return ii;
}

//_____________________________________________________________________________
} //namespace Decoder

ClassImp(Decoder::PipeliningModule)
