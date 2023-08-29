#ifndef Podd_PipeliningModule_h_
#define Podd_PipeliningModule_h_

/////////////////////////////////////////////////////////////////////
//
//   PipeliningModule
//   R. Michaels, Oct 2016
//
//   This class splits the "CODA event buffer" into actual events so that the
//   Podd analyzer's event loop works as before.
//   An "event" is a particle hitting a target and making hits in detectors, etc.
//   A "CODA event buffer" can contain many "events"; the events are stored in
//   each module of this type (piplelining).
//
//   All of the Jlab pipeline modules have the same data format with respect to
//   specific bits indicating data types.
//   All produce block headers, block trailers, and event headers.
//   All encode the slot number the same way in these headers and trailers.
//
//   While we're at it, we also split by slot number
//   so that each module doesn't need to consider another module's data.
//   Note, one module belongs to one slot.
//
//   The first event buffer will have the block header
//   the last event buffer will have the block trailer
//   and all event buffers will have an event header
//
/////////////////////////////////////////////////////////////////////

#include "VmeModule.h"
#include "CustomAlloc.h"
#include <vector>
#include <cstdint>
#include <cassert>

class THaSlotData;

namespace Decoder {

class PipeliningModule : public VmeModule  {

public:

   PipeliningModule() : PipeliningModule(0,0) {}
   PipeliningModule( UInt_t crate, UInt_t slot );
   PipeliningModule(const PipeliningModule &fh) = delete;
   PipeliningModule& operator=(const PipeliningModule &fh) = delete;
   virtual ~PipeliningModule() = default;

   using VmeModule::Init;
   virtual void Init( const char* configstr );
   virtual void Clear( Option_t *opt="" );

  // Wrappers for LoadSlot methods to allow buffer preprocessing
  virtual UInt_t LoadBlock( THaSlotData* sldat, const UInt_t* evbuffer,
                            const UInt_t* pstop );
  virtual UInt_t LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                           UInt_t pos, UInt_t len );

protected:

   virtual UInt_t LoadNextEvBuffer( THaSlotData *sldat );
   UInt_t fBlockHeader;    // Copy of block header word
   UInt_t data_type_def;   // Data type indicated by most recent header word

   // Support for multi-block mode
   VectorUIntNI fBuffer;   // Copy of this module's chunk of the event buffer
   std::vector<Long64_t> evtblk;  // Event header positions
   UInt_t index_buffer;    // Index of next block to be decoded

   enum { kBlockHeader = 0, kBlockTrailer = 1, kEventHeader = 2 };
   static Long64_t FindIDWord( const uint32_t* buf, size_t start, size_t len,
                               uint32_t type );
   static Long64_t FindIDWord( const uint32_t* buf, size_t start, size_t len,
                               uint32_t type, uint32_t slot );
   static Long64_t FindEventsInBlock(
     const uint32_t* buf, size_t start, size_t len, uint32_t evthdr,
     uint32_t blktrl, std::vector<Long64_t>& evtpos, uint32_t slot );

   Long64_t VerifyBlockTrailer( const UInt_t* evbuffer, UInt_t pos, UInt_t len,
                                Long64_t ibeg, Long64_t iend ) const;

   ClassDef(Decoder::PipeliningModule,0)  // A pipelining module
};

//_____________________________________________________________________________
inline
Long64_t PipeliningModule::FindIDWord(
  const uint32_t* buf, size_t start, size_t len, uint32_t type )
{
  // Search 'buf' for identifier word for 'type'.
  // The format is (T=type)
  //    1TTT TXXX XXXX XXXX XXXX XXXX XXXX XXXX
  // bit  28   24   20   16   12    8    4    0
  //
  // The buffer is searched between [start,start+len)
  // Returns the offset into 'buf' containing the word, or -1 if not found.

  const uint32_t ID = BIT(31) | (type & 0x0F) << 27;
  const auto* p = buf + start;
  const auto* q = p + len;
  while( p != q && (*p & 0xF8000000) != ID )
    ++p;
  return (p != q) ? p - buf : -1;
}

//_____________________________________________________________________________
inline
Long64_t PipeliningModule::FindIDWord(
  const uint32_t* buf, size_t start, size_t len, uint32_t type, uint32_t slot )
{
  // Search 'buf' for data type identifier word encoding 'type' and 'slot'.
  // The format is (T=type, S=slot)
  //    1TTT TSSS SSXX XXXX XXXX XXXX XXXX XXXX
  // bit  28   24   20   16   12    8    4    0
  //
  // The buffer is searched between [start,start+len)
  // Returns the offset into 'buf' containing the word, or -1 if not found.

  const uint32_t ID = BIT(31) | (type & 0x0F) << 27 | (slot & 0x1F) << 22;
  const auto* p = buf + start;
  const auto* q = p + len;
  while( p != q && (*p & 0xFFC00000) != ID )
    ++p;
  return (p != q) ? p - buf : -1;
}

//_____________________________________________________________________________
inline
Long64_t PipeliningModule::FindEventsInBlock(
  const uint32_t* buf, size_t start, size_t len, uint32_t evthdr,
  uint32_t blktrl, std::vector<Long64_t>& evtpos, uint32_t slot )
{
  // Search 'buf' for block trailer (identified by type = 'blktrl' and
  // slot = 'slot', see FindIDWord for the bit format).
  // While scanning, save any event headers (identified by type = 'evthdr'
  // in vector 'evtpos'.
  // The buffer is searched between [start,start+len).
  // Returns the offset into 'buf' of the trailer word, or -1 if not found.

  const uint32_t HDR = BIT(31) | (slot & 0x1F) << 22;
  const uint32_t EVT = HDR | (evthdr & 0x0F) << 27;
  const uint32_t TRL = HDR | (blktrl & 0x0F) << 27;
  const auto* p = buf + start;
  const auto* q = p + len;
  while( p != q && (*p & 0xFFC00000) != TRL ) {
    // Ignore the slot number in event headers since, anecdotally,
    // it is not reliably present.
    // Change the bit mask below to 0xFFC00000 to include the slot number.
    if( (*p & 0xF8000000) == (EVT & 0xF8000000) )
      evtpos.push_back(p - buf);
    ++p;
  }
  return (p != q) ? p - buf : -1;
}
} //namespace Decoder

#endif
