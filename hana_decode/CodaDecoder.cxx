///////////////////////////////////////////////////////////////////
//
//   CodaDecoder
//
//    Object-oriented version of decoder
//    Sept, 2014    R. Michaels
//
/////////////////////////////////////////////////////////////////////

#include "CodaDecoder.h"
#include "THaCrateMap.h"
#include "THaBenchmark.h"
#include "THaUsrstrutils.h"
#include "DAQconfig.h"
#include "Helper.h"
#include "TError.h"
#include "TList.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace std;

namespace Decoder {

static constexpr auto MAXROCSLOT_FB = MAXROC * MAXSLOT_FB;

//_____________________________________________________________________________
CodaDecoder::CodaDecoder()
  : nroc(0)
  , irn(MAXROC, 0)
  , fbfound(MAXROCSLOT_FB, false)
  , psfact(MAX_PSFACT, kMaxUInt)
  , buffmode{false}
  , synchmiss{false}
  , synchextra{false}
  , fdfirst(true)
  , chkfbstat(1)
  , blkidx(0)
  , fMultiBlockMode{false}
  , fBlockIsDone{false}
  , tsEvType{0}
  , bank_tag{0}
  , block_size{0}
{
  bankdat.reserve(32);
  // Please leave these 3 lines for me to debug if I need to.  thanks, Bob
#ifdef WANTDEBUG
  fDebugFile = new ofstream();
  fDebugFile->open("bobstuff.txt");
  *fDebugFile<< "Debug my stuff "<<endl<<endl;
#endif
  DAQInfoExtra::AddTo(fExtra);
}

//_____________________________________________________________________________
#ifndef WANTDEBUG
CodaDecoder::~CodaDecoder() = default;
#else
CodaDecoder::~CodaDecoder()
{
  delete fDebugFile; fDebugFile = nullptr;
}
#endif

//_____________________________________________________________________________
UInt_t CodaDecoder::GetPrescaleFactor( UInt_t trigger_type) const
{
  // To get the prescale factors for trigger number "trigger_type"
  // (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
    return psfact[trigger_type - 1];
  }
  if (fDebug > 0) {
    Warning( "CodaDecoder::GetPrescaleFactor", "Requested prescale factor for "
             "undefined trigger type %d", trigger_type );
  }
  return 0;
}

//_____________________________________________________________________________
Int_t CodaDecoder::Init()
{
  Int_t ret = THaEvData::Init();
  if( ret != HED_OK ) return ret;
  FindUsedSlots();
  auto* cfg = DAQInfoExtra::GetFrom(fExtra);
  if( cfg )
    cfg->clear();
  return ret;
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadEvent( const UInt_t* evbuffer )
{
  // Main engine for decoding, called by public LoadEvent() methods

  assert(evbuffer);

  if (fDebugFile) {
    *fDebugFile << "CodaDecode:: Loading event  ... " << endl
                << "evbuffer ptr " << hex << evbuffer << dec << endl;
  }

  if (fDataVersion <= 0) {
    cerr << "CODA version not set (=" << fDataVersion << "). "
         << "Do SetCodaVersion(codaData->getCodaVersion()) first" << endl;
    return HED_FATAL;
  }
  if (fDataVersion != 2 && fDataVersion != 3) {
    cerr << "Unsupported CODA version = " << fDataVersion << "). Need 2 or 3."
         << "Cannot analyze these data. Twonk."<<endl;
    return HED_FATAL;
  }

  if( DataCached() ) {
    if( evbuffer[0]+1 != event_length ) {
      throw std::logic_error("Event buffer changed while processing multiblock "
                             "data. Do not read new events while DataCached() "
                             "returns true!");
    }
    if( ++blkidx >= block_size ) {
      throw std::logic_error("Attempt to decode more events in block than "
                             "available (blkidx >= block_size). Logic error. "
                             "Call expert.");
    }

    if( fDataVersion == 3 )
      LoadTrigBankInfo(blkidx);
    ++event_num;    // _should_ be sequential
    return LoadFromMultiBlock();
  }

  buffer = evbuffer;
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_type = 0;
  data_type = 0;
  trigger_bits = 0;
  evt_time = 0;
  bankdat.clear();
  blkidx = 0;

  // Determine event type
  if (fDataVersion == 2) {
    event_type = evbuffer[1]>>16;
  } else {  // CODA version 3
    interpretCoda3(evbuffer);  // this defines event_type
  }
  if(fDebugFile) {
      *fDebugFile << "CodaDecode:: dumping "<<endl;
      dump(evbuffer);
  }

  Int_t ret = HED_OK;

  if (event_type == PRESTART_EVTYPE ) {
    // Usually prestart is the first 'event'.  Call SetRunTime() to
    // re-initialize the crate map since we now know the run time.
    // This won't happen for split files (no prestart). For such files,
    // the user should call SetRunTime() explicitly.
    SetRunTime(evbuffer[2]);
    run_num  = evbuffer[3];
    run_type = evbuffer[4];
    if (fDebugFile) {
      *fDebugFile << "Prestart Event : run_num " << run_num
                  << "  run type "   << run_type
                  << "  event_type " << event_type
                  << "  run time "   << fRunTime
                  << endl;
    }
  }

  else if( event_type == PRESCALE_EVTYPE || event_type == TS_PRESCALE_EVTYPE ) {
    if (fDataVersion == 2) {
      ret = prescale_decode_coda2(evbuffer);
    } else {
      ret = prescale_decode_coda3(evbuffer);
    }
    if (ret != HED_OK ) return ret;
  }

  else if( event_type == DAQCONFIG_FILE1 || event_type == DAQCONFIG_FILE2 ) {
    if( fDataVersion > 2 ) {
      if( (ret = daqConfigDecode(evbuffer)) != HED_OK )
        return ret;
    } else {
      cerr << "Warning: Event type " << event_type << " decoding only "
           << "supported for CODA data version 3 or higher" << endl;
    }
  }

  else if( event_type <= MAX_PHYS_EVTYPE && !PrescanModeEnabled() ) {
    if( fDataVersion == 3 &&
        (ret = trigBankDecode(evbuffer)) != HED_OK ) {
      return ret;
    }
    ret = physics_decode(evbuffer);
  }

  return ret;
}

//_____________________________________________________________________________
Int_t CodaDecoder::physics_decode( const UInt_t* evbuffer )
{
  assert(fMap || fNeedInit);
  if( first_decode || fNeedInit ) {
    Int_t ret = Init();
    if( ret != HED_OK )
      return ret;
  }
  assert(fMap);
  if( fDoBench ) fBench->Begin("clearEvent");
  for( auto i : fSlotClear )
    crateslot[i]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");

  if( fDataVersion == 3 ) {
    event_num = tbank.evtNum;
    FindRocsCoda3(evbuffer);
  } else {
    event_num = evbuffer[4];
    FindRocs(evbuffer);
  }

  if( fdfirst && fDebugFile ) {
    fdfirst = false;
    CompareRocs();
  }

  // Decode each ROC
  // From this point onwards there is no diff between CODA 2.* and CODA 3.*

  for( UInt_t i = 0; i < nroc; i++ ) {

    UInt_t iroc = irn[i];
    const RocDat_t& ROC = rocdat[iroc];
    UInt_t ipt = ROC.pos + 1;
    UInt_t iptmax = ROC.pos + ROC.len; // last word of data

    if( fMap->isFastBus(iroc) ) {  // checking that slots found = expected
      if( GetEvNum() > 200 && chkfbstat < 3 ) chkfbstat = 2;
      if( chkfbstat == 1 ) ChkFbSlot(iroc, evbuffer, ipt, iptmax);
      if( chkfbstat == 2 ) {
        ChkFbSlots();
        chkfbstat = 3;
      }
    }

    // If at least one module is in a bank, must split the banks for this roc

    Int_t status;
    if( fMap->isBankStructure(iroc) ) {
      if( fDebugFile )
        *fDebugFile << "\nCodaDecode::Calling bank_decode "
                    << iroc << "  " << ipt << "  " << iptmax
                    << endl;
      try {
        status = bank_decode(iroc, evbuffer, ipt, iptmax);
      }
      catch( const logic_error& e ) {
        Error("CodaDecoder::bank_decode", "ERROR: %s", e.what());
        return HED_ERR;
      }
      if( status != HED_OK )
        return status;
    }

    if( !fMap->isAllBanks(iroc) ) {
      if( fDebugFile )
        *fDebugFile << "\nCodaDecode::Calling roc_decode "
                    << iroc << "  " << ipt << "  " << iptmax
                    << endl;

      try {
        status = roc_decode(iroc, evbuffer, ipt, iptmax);
      }
      catch( const logic_error& e ) {
        Error("CodaDecoder::roc_decode", "ERROR: %s", e.what());
        return HED_ERR;
      }
      if( status != HED_OK )
        return status;
    }
  }
  // Print summary of discovered banks
  constexpr UInt_t bankinfo_bit = 65;
  if( !fMsgPrinted.TestBitNumber(bankinfo_bit) ) {
    if( !bankdat.empty() ) {
      PrintBankInfo();
      // Unless we are debugging, print just once at start of run
      if( fDebug < 2 )
        fMsgPrinted.SetBitNumber(bankinfo_bit);
    }
  }
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::interpretCoda3(const UInt_t* evbuffer)
{
  // Extract basic information from a CODA3 event
  tbank.Clear();
  tsEvType = 0;

  bank_tag   = (evbuffer[1] & 0xffff0000) >> 16;
  data_type  = (evbuffer[1] & 0xff00) >> 8;
  block_size = evbuffer[1] & 0xff;

  event_type = InterpretBankTag(bank_tag);

  if( bank_tag < 0xff00 ) { // User event type
    if( fDebug > 1 )    // if set, character data gets printed.
      debug_print(evbuffer);

    if( fDebugFile )
      *fDebugFile << " User defined event type " << event_type << endl;
  }
  if( fDebugFile )
    *fDebugFile << "CODA 3  Event type " << event_type << " trigger_bits "
                << trigger_bits << "  tsEvType  " << tsEvType
                << "  evt_time " << GetEvTime() << endl;

  return HED_OK;
}

//_____________________________________________________________________________
UInt_t CodaDecoder::InterpretBankTag( UInt_t tag )
{
  UInt_t evtyp{};

  if( tag >= 0xff00 ) { // CODA Reserved bank type
    switch( tag ) {
      case 0xffd1:
        evtyp = PRESTART_EVTYPE;
        break;
      case 0xffd2:
        evtyp = GO_EVTYPE;
        break;
      case 0xffd4:
        evtyp = END_EVTYPE;
        break;
      case 0xff50:
      case 0xff58:      // Physics event with sync bit
      case 0xFF78:
      case 0xff70:
        evtyp = 1;      // for CODA 3.* physics events are type 1.
        break;
      default:          // Undefined CODA 3 event type
        cerr << "CodaDecoder:: WARNING:  Undefined CODA 3 event type, tag = "
             << "0x" << hex << tag << dec << endl;
        evtyp = 0;
        //FIXME evtyp = 0 could also be a user event type ...
        // maybe throw an exception here?
    }
  } else {              // User event type
    evtyp = tag;        // ET-insertions
  }

  return evtyp;
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadTrigBankInfo( UInt_t i )
{
  // CODA3: Load tsEvType, evt_time, and trigger_bits for i-th event
  // in event block buffer. index_buffer must be < block size.

  assert(i < tbank.blksize);
  if( i >= tbank.blksize )
    return -1;
  tsEvType = tbank.evType[i];      // event type (configuration-dependent)
  if( tbank.evTS )
    evt_time = tbank.evTS[i];      // event time (4ns clock, I think)
  else if( tbank.TSROC ) {
    UInt_t struct_size = tbank.withTriggerBits() ? 3 : 2;
    evt_time = *(const uint64_t*) (tbank.TSROC + struct_size * i);
    // Only the lower 48 bits seem to contain the time
    evt_time &= 0x0000FFFFFFFFFFFF;
  }
  if( tbank.withTriggerBits() )
    // Trigger bits. Only the lower 6 bits seem to contain the actual bits
    trigger_bits = tbank.TSROC[2 + 3 * i] & 0x3F;

  return 0;
}

//_____________________________________________________________________________
Int_t CodaDecoder::trigBankDecode( const UInt_t* evbuffer )
{
  // Decode the CODA3 trigger bank. Copy relevant data to member variables.
  // This will initialize the crate map.

  const char* const here = "CodaDecoder::trigBankDecode";

  // If necessary, initialize the crate map (for TS ROC)
  assert(fMap || fNeedInit);
  if( first_decode || fNeedInit ) {
    Int_t ret = Init();
    if( ret != HED_OK )
      return ret;
  }
  assert(fMap);

  // Decode trigger bank (bank of segments at start of physics event)
  if( block_size == 0 ) {
    Error(here, "CODA 3 format error: Physics event with block size 0");
    return HED_ERR;
  }
  try {
    tbank.Fill(evbuffer + 2, block_size, fMap->getTSROC());
  }
  catch( const coda_format_error& e ) {
    Error(here, "CODA 3 format error: %s", e.what() );
    return HED_ERR;
  }

  // Copy pertinent data to member variables for faster retrieval
  LoadTrigBankInfo(0);  // Load data for first event in block

  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::BankInfo::Fill(             // NOLINT(misc-no-recursion)
  const UInt_t* evbuf, UInt_t pos, UInt_t len )
{
  if( !evbuf ) {
    return status_ = kBadArg;
  }
  if( len < 3 ) {  // Expect at least 1 bank header + 1 payload word
    return status_ = kBadArg;
  }
  // Bank length
  const auto* p = evbuf + pos;
  UInt_t blen = *p;  // number of 32-bit words following this length word
  if( blen < 2 || blen + 1 > len ) {
    len_ = blen;
    return status_ = kBadLen;
  }

  // Bank header
  ++p;
  UInt_t tag   = (*p & 0xFFFF0000) >> 16;
  UInt_t npad  = (*p & 0x0000C000) >> 14;
  UInt_t dtyp  = (*p & 0x00003F00) >> 8;
  UInt_t blksz = (*p & 0x000000FF);
  // First word of payload
  ++p;
  status_ = kOK;
  switch( dtyp ) {
    case 0x08: // float64_t (double)
    case 0x09: // int64_t
    case 0x0A: // uint64_t
    case 0x00: // 32-bit unknown
    case 0x01: // uint32_t
    case 0x02: // float32_t
    case 0x0B: // int32_t
      if( npad != 0 ) {
        npad_   = npad;
        status_ = kBadPad;
      }
      break;

    case 0x04: // int16_t
    case 0x05: // uint16_t
      if( npad != 0 && npad != 2 ) {
        npad_ = npad;
        status_ = kBadPad;
      }
      break;

    case 0x03: // int8_t   (char)
    case 0x06: // int8_t   (signed char)
    case 0x07: // uint8_t  (unsigned char)
      break;

    case 0x10: // Bank
      if( otag_ == 0 )
        otag_ = tag;
      return Fill(evbuf, pos+2, blen-1);
      break;

    default: // Segments, composite, etc. not yet supported
      dtyp_ = dtyp;
      return status_ = kUnsupType;
  }
  pos_   = pos+2;
  len_   = blen-1;
  tag_   = tag;
  dtyp_  = dtyp;
  npad_  = npad;
  blksz_ = blksz;
  return status_;
}

//_____________________________________________________________________________
const char* CodaDecoder::BankInfo::Errtxt() const
{
  switch( status_ ) {
    case kOK:
      return "OK";
    case kBadArg:
      return "Bad argument";
    case kBadLen:
      return "Invalid length";
    case kBadPad:
      return "Invalid padding size";
    case kUnsupType:
      return "Unsupported data type";
  }
  // Not reached
  assert(false);
  return "Unknown error";
}

//_____________________________________________________________________________
const char* CodaDecoder::BankInfo::Typtxt() const
{
  static const char* typtxt[] = {
    "32-bit unknown",
    "uint32_t",
    "float",
    "char",
    "int16_t",
    "uint16_t",
    "int8_t",
    "uint8_t",
    "double",
    "int64_t",
    "uint64_t",
    "int32_t",
    "TAGSEGMENT",
    "SEGMENT",
    "BANK",
    "Composite",
    "BANK"
  };
  if( dtyp_ <= 0x10 )
    return typtxt[dtyp_];
  if( dtyp_ == 0x20 )
    return "SEGMENT";
  if( dtyp_ == 0x21 )
    return "Hollerit";
  if( dtyp_ == 0x22 )
    return "N value";
  return "Unknown";
}

//_____________________________________________________________________________
CodaDecoder::BankInfo::EDataSize CodaDecoder::BankInfo::GetDataSize() const
{
  switch( dtyp_ ) {
    case 0x08: // float64_t (double)
    case 0x09: // int64_t
    case 0x0A: // uint64_t
      return k64bit;
    case 0x00: // 32-bit unknown
    case 0x01: // uint32_t
    case 0x02: // float32_t
    case 0x0B: // int32_t
      return k32bit;
    case 0x04: // int16_t
    case 0x05: // uint16_t
      return k16bit;
    case 0x03: // int8_t   (char)
    case 0x06: // int8_t   (signed char)
    case 0x07: // uint8_t  (unsigned char)
      return k8bit;
    default:
      return kUndef;
  }
}

//_____________________________________________________________________________
CodaDecoder::BankInfo::EIntFloat CodaDecoder::BankInfo::GetFloat() const
{
  switch( dtyp_ ) {
    case 0x02: // float32_t (float)
    case 0x08: // float64_t (double)
      return kFloat;
    default:
      return kInteger;
  }
}

//_____________________________________________________________________________
CodaDecoder::BankInfo::ESigned CodaDecoder::BankInfo::GetSigned() const
{
  switch( dtyp_ ) {
    case 0x02: // float32_t
    case 0x04: // int16_t
    case 0x06: // int8_t   (signed char)
    case 0x08: // float64_t (double)
    case 0x09: // int64_t
    case 0x0B: // int32_t
      return kSigned;
    case 0x01: // uint32_t
    case 0x05: // uint16_t
    case 0x07: // uint8_t  (unsigned char)
    case 0x0A: // uint64_t
      return kUnsigned;
    case 0x00: // 32-bit unknown
    case 0x03: // int8_t   (char)
    default:
      return kUnknown;
  }
}


//_____________________________________________________________________________
Int_t CodaDecoder::daqConfigDecode( const UInt_t* evbuf )
{
  // Decode DAQ configuration event. This is usually a dump of several
  // database files, one segment of type string per file.
  // Each such segment is saved as a (longish) string internally.
  // Also parses the VTP configuration file (the first of the strings)
  // into key/value pairs.

  const char* const here = "CodaDecoder::daqConfigDecode";

  if( !evbuf )
    return HED_FATAL;
  auto* cfg = DAQInfoExtra::GetFrom(fExtra);
  if( !cfg )
    return HED_ERR;
  size_t nbefore = cfg->strings.size();

#define CFGEVT1 Decoder::DAQCONFIG_FILE1
#define CFGEVT2 Decoder::DAQCONFIG_FILE2
  assert(bank_tag == CFGEVT1 || bank_tag == CFGEVT2); // else bug in LoadEvent
  if( (bank_tag != CFGEVT1 && bank_tag != CFGEVT2) ) {
    Error(here, "Invalid bank tag word %#x in event type %u",
          *(evbuf + 1), event_type);
    return HED_ERR;
  }
  UInt_t pos = 0;
  while( pos < event_length ) {
    auto bankinfo = GetBank(evbuf, pos, event_length);
    if( bankinfo.status_ != BankInfo::kOK ) {
      Error( here, "Bank decoding error %s", bankinfo.Errtxt());
      return HED_ERR;
    }
    pos = bankinfo.pos_;
    auto len = bankinfo.len_;
    assert(pos + len <= event_length);
    if( bankinfo.GetDataSize() == BankInfo::k8bit ) {
      const auto* c = reinterpret_cast<const char*>(evbuf + pos);
      cfg->strings.emplace_back(c, c + 4 * len - bankinfo.npad_);
    } else {
      Warning(here, "Unsupported data type %#x in event type %u",
              bankinfo.dtyp_, event_type);
    }
    pos += len;
  }
  // Parse first string to key/value pairs
  const size_t iparse = 0;
  if( iparse >= nbefore )
    cfg->parse(0);

  return HED_OK;
}

//_____________________________________________________________________________
void CodaDecoder::debug_print( const UInt_t* evbuffer ) const
{
  // checks of ET-inserted data
  Int_t print_it=0;

  switch( event_type ) {

  case EPICS_EVTYPE:
    cout << "EPICS data "<<endl;
    print_it=1;
    break;
  case PRESCALE_EVTYPE:
    cout << "Prescale data "<<endl;
    print_it=1;
    break;
  case DAQCONFIG_FILE1:
    cout << "DAQ config file 1 "<<endl;
    print_it=1;
    break;
  case DAQCONFIG_FILE2:
    cout << "DAQ config file 2 "<<endl;
    print_it=1;
    break;
  case SCALER_EVTYPE:
    cout << "LHRS scaler event "<<endl;
    print_it=1;
    break;
  case SBSSCALER_EVTYPE:
    cout << "SBS scaler event "<<endl;
    print_it=1;
    break;
  case HV_DATA_EVTYPE:
    cout << "High voltage data event "<<endl;
    print_it=1;
    break;
  default:
    // something else ?
    cout << endl << "--- Special event type " << event_type << endl << endl;
  }
  if(print_it) {
    char *cbuf = (char *)evbuffer; // These are character data
    size_t elen = sizeof(int)*(evbuffer[0]+1);
    cout << "Dump of event buffer .  Len = "<<elen<<endl;
    // This dump will look exactly like the text file that was inserted.
    for (size_t ii=0; ii<elen; ii++) cout << cbuf[ii];
  }
}

//_____________________________________________________________________________
void CodaDecoder::PrintBankInfo() const
{
  // Pretty-print bank info in 'bankdat'.

  // Sort banks by ROC, then position in buffer
  auto banks = bankdat;
  std::sort(ALL(banks), []( const BankDat_t& a, const BankDat_t& b ) -> bool {
    if( (a.key >> 16) < (b.key >> 16) )
      return true;
    else if( (a.key >> 16) > (b.key >> 16) )
      return false;
    return a.pos < b.pos;
  });

  UInt_t sum = 0;
  cout << "Banks found (event " << event_num << "):" << endl;
  cout << " roc  bank     pos     len" << endl;
  for( const auto& b : banks ) {
    UInt_t bank = b.key & 0xFFFF;
    UInt_t roc  = b.key >> 16;
    cout << dec << setw(4) << roc
         << setw(6) << bank
         << setw(8) << b.pos
         << setw(8) << b.len
         << endl;
    sum += b.len;
  }
  cout << "Sum" << setw(23) << sum << endl;
}

//_____________________________________________________________________________
uint32_t CodaDecoder::TBOBJ::Fill( const uint32_t* evbuffer,
                                   uint32_t blkSize, uint32_t tsroc )
{
  if( blkSize == 0 )
    throw std::invalid_argument("CODA block size must be > 0");
  start = evbuffer;
  blksize = blkSize;
  len = evbuffer[0] + 1;
  tag = (evbuffer[1] & 0xffff0000) >> 16;
  nrocs = evbuffer[1] & 0xff;

  const uint32_t* p = evbuffer + 2;
  // Segment 1:
  //  uint64_t event_number
  //  uint64_t run_info                if withRunInfo
  //  uint64_t time_stamp[blkSize]     if withTimeStamp
  {
    uint32_t slen = *p & 0xffff;
    if( slen != 2*(1 + (withRunInfo() ? 1 : 0) + (withTimeStamp() ? blkSize : 0)))
      throw coda_format_error("Invalid length for Trigger Bank seg 1");
    const auto* q = (const uint64_t*) (p + 1);
    evtNum  = *q++;
    runInfo = withRunInfo()   ? *q++ : 0;
    evTS    = withTimeStamp() ? q    : nullptr;
    p += slen + 1;
  }
  if( p-evbuffer >= len )
    throw coda_format_error("Past end of bank after Trigger Bank seg 1");

  // Segment 2:
  //  uint16_t event_type[blkSize]
  //  padded to next 32-bit boundary
  {
    uint32_t slen = *p & 0xffff;
    if( slen != (blkSize-1)/2 + 1 )
      throw coda_format_error("Invalid length for Trigger Bank seg 2");
    evType = (const uint16_t*) (p + 1);
    p += slen + 1;
  }

  // nroc ROC segments containing timestamps and optional
  // data like trigger latch bits:
  // struct {
  //   uint64_t roc_time_stamp;     // Lower 48 bits only seem to be the time.
  //   uint32_t roc_trigger_bits;   // Optional. Typically only in TSROC.
  // } roc_segment[blkSize];
  TSROC = nullptr;
  tsrocLen = 0;
  for( uint32_t i = 0; i < nrocs; ++i ) {
    if( p-evbuffer >= len )
      throw coda_format_error("Past end of bank while scanning trigger bank segments");
    uint32_t slen = *p & 0xffff;
    uint32_t rocnum = (*p & 0xff000000) >> 24;
    if( rocnum == tsroc ) {
      TSROC = p + 1;
      tsrocLen = slen;
      break;
    }
    p += slen + 1;
  }

  return len;
}

//_____________________________________________________________________________
CodaDecoder::BankDat_t* CodaDecoder::CheckForBank( UInt_t roc, UInt_t slot )
{
  // Check if roc/slot belongs to a bank. If so, check if this bank
  // has been found in the current event. Return pointer to the corresponding
  // bank data if found, otherwise nullptr.
  // Internal function used by bank_decode() and LoadFromMultiBlock().

  Int_t bank = fMap->getBank(roc, slot);
  assert(bank < MAXBANK); // bank numbers are uint16_t
  if( bank < 0 )
    return nullptr;
  UInt_t key = (roc << 16) + bank;
  auto theBank = find(ALL(bankdat), key);
  if( theBank == bankdat.end() )
    // Bank defined in crate map but not present in this event
    return nullptr;
  return &(*theBank);
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadFromMultiBlock()
{
  // LoadFromMultiBlock : This assumes some slots are in multiblock mode.
  // For modules that are in multiblock mode, the next event is loaded.
  // For other modules not in multiblock mode (e.g. scalers) or other data
  // (e.g. flags) the data remain "stale" until the next block of events.

  if( !fMultiBlockMode || fBlockIsDone ) {
    Error("CodaDecoder::LoadFromMultiBlock",
          "Not in multiblock mode or block already done. Logic error. "
          "Call expert.");
    return HED_ERR;
  }
  if( first_decode || fNeedInit ) {
    Error("CodaDecoder::LoadFromMultiBlock",
          "Uninitialized while processing event blocks. Logic error. "
          "Call expert.");
    return HED_ERR;
  }

  for( auto i : fSlotClear ) {
    auto* mod = crateslot[i]->GetModule();
    if( mod && mod->IsMultiBlockMode() )
      crateslot[i]->clearEvent();
  }

  for( UInt_t i = 0; i < nroc; i++ ) {
    UInt_t roc = irn[i];
    for( auto slot : fMap->GetUsedSlots(roc) ) {
      assert(fMap->slotUsed(roc, slot));
      // Skip modules in banks if the bank is not present in the current event
      if( fMap->getBank(roc, slot) >= 0 && !CheckForBank(roc, slot) )
        continue;
      auto* sd = crateslot[idx(roc, slot)].get();
      auto* mod = sd->GetModule();
      if( !mod )
        continue;
      // for CODA3, cross-check the block size (found in trigger bank and, separately, in modules)
      if( fDataVersion > 2 ) {
        auto module_blksz = mod->GetBlockSize();
        if( fDebugFile )
          *fDebugFile << "cross chk blk size " << roc << "  " << slot << "  "
                      << module_blksz << "   " << block_size << endl;
        if( module_blksz != block_size ) {
          cerr << "ERROR::CodaDecoder:: inconsistent block size between trig. bank and module "
               << " in roc/slot = " << roc << "/" << slot << " (\"" << mod->GetName() << "\"), "
               << "trigger bank = " << block_size << ", module = " << module_blksz
               << ". Ignoring module for this event."
               << endl;
          continue;
        }
      }
      if( mod->IsMultiBlockMode() ) {
        sd->LoadNextEvBuffer();
        // Presumes that all modules have the same global block size (see check above)
        if( sd->BlockIsDone() )
          fBlockIsDone = true;
      }
      // else {
      // Not really sure what to do with a module that isn't in multi-block mode
      // while others are ...
      // }
    }
  }
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::roc_decode( UInt_t roc, const UInt_t* evbuffer,
                               UInt_t ipt, UInt_t istop )
{
  // Decode a Readout controller
  assert( evbuffer && fMap );
  if( roc >= MAXROC ) {
    ostringstream ostr;
    ostr << "ROC number " << roc << " out of range";
    throw logic_error(ostr.str());
  }
  if( istop >= event_length )
    throw logic_error("Stop point exceeds event length (?!)");

  synchmiss = false;
  synchextra = false;
  buffmode = false;
  fBlockIsDone = false;

  if( ipt+1 >= istop )
    return HED_OK;

  if( fDoBench ) fBench->Begin("roc_decode");
  Int_t retval = HED_OK;
  try {
    UInt_t Nslot = fMap->getNslot(roc);
    if( Nslot == 0 || Nslot == kMaxUInt ) {
      // Found data for a ROC which is not defined in the crate map.
      // Just ignore it. Do debug log occurrences and warn user once about it.
      if( fDebugFile ) {
        *fDebugFile << "CodaDecode:: roc_decode:: WARNING: Undefined ROC # "
                    << dec << roc << ", event " << event_num << endl;
      }
      UInt_t ibit = roc;
      if( !fMsgPrinted.TestBitNumber(ibit) ) {
        Warning("roc_decode", "ROC %d found in data but NOT in cratemap. "
                              "Ignoring it.", roc);
        fMsgPrinted.SetBitNumber(ibit);
      }
      return HED_OK;
    }

    if( fDebugFile )
      *fDebugFile << "CodaDecode:: roc_decode:: roc#  " << dec << roc
                  << " nslot " << Nslot << endl;

    assert(fMap->GetUsedSlots(roc).size() == Nslot); // else bug in THaCrateMap

    // Build the to-do list of slots based on the contents of the crate map
    vector<pair<UInt_t,THaSlotData*>> slots_todo; slots_todo.reserve(Nslot);
    for( auto slot : fMap->GetUsedSlots(roc) ) {
      assert(fMap->slotUsed(roc, slot));   // else bug in THaCrateMap
      // ignore bank structure slots; they are decoded with bank_decode
      if( fMap->getBank(roc, slot) >= 0 )
        continue;
      slots_todo.emplace_back(slot,crateslot[idx(roc,slot)].get());
    }
    // Quit if nothing to do (all bank structure slots, decoded in bank_decode)
    if( slots_todo.empty() )
      return HED_OK;

    // higher slot # appears first in multiblock mode
    // the decoding order improves efficiency
    bool is_fastbus = fMap->isFastBus(roc);
    if( is_fastbus )
      std::reverse( ALL(slots_todo) );

    // Crawl through this ROC's data block. Each word is tested against all the
    // defined modules (slots) in the crate for a match with the expected slot
    // header. If a match is found, this word is the slot header. Zero or more
    // words following the slot header represent the data for the slot. These
    // data are loaded into the module's internal storage, and the corresponding
    // slot is removed from the search list. The search for the remaining slots
    // then resumes at the first word after the data.
    const UInt_t* p = evbuffer + ipt;    // Points to ROC ID word (1 before data)
    const UInt_t* pstop = evbuffer + istop;   // Points to last word of data
    UInt_t nextidx = 0;

    while( p++ < pstop ) {
      if( fDebugFile )
        *fDebugFile << "CodaDecode::roc_decode:: evbuff " << (p - evbuffer)
                    << "  " << hex << *p << dec << endl;

      if( is_fastbus && LoadIfFlagData(p) )
        continue;

      bool update_nextidx = true;
      for( UInt_t i = nextidx; i < slots_todo.size(); ++i ) {
        UInt_t slot = slots_todo[i].first;
        // ignore bank structure slots; they are decoded with bank_decode
        if( slot == kMaxUInt )
          continue;
        auto* sd = slots_todo[i].second;

        if( fDebugFile )
          *fDebugFile << "roc_decode:: slot logic " << roc << "  " << slot;

        // Check if data word at p belongs to the module at the current slot
        UInt_t nwords = sd->LoadIfSlot(p, pstop);

        if( sd->IsMultiBlockMode() )
          fMultiBlockMode = true;
        if( sd->BlockIsDone() )
          fBlockIsDone = true;

        if( fDebugFile )
          *fDebugFile << "CodaDecode:: roc_decode:: after LoadIfSlot "
                      << p + ((nwords > 0) ? nwords - 1 : 0) << "  " << pstop
                      << "    " << hex << *p << "  " << dec << nwords << endl;

        if( nwords > 0 ) {
          if( fDebugFile )
            *fDebugFile << "CodaDecode::  slot " << slot << "  is DONE    "
                        << nwords << endl;
          // Data for this slot found and loaded. Advance to next data block.
          p += nwords-1;
          slots_todo[i].first = kMaxUInt;  // Mark slot as done
          if( update_nextidx )
            nextidx = i+1;
          break;
        } else if( update_nextidx ) {
          nextidx = i;
          update_nextidx = false;
        }
      }
    } //end while(p++<pstop)
  }
  catch( const exception& e ) {
    cerr << e.what() << endl;
    retval = HED_ERR;
  }

  if( fDoBench ) fBench->Stop("roc_decode");
  return retval;
}

//_____________________________________________________________________________
Int_t CodaDecoder::bank_decode( UInt_t roc, const UInt_t* evbuffer,
                                UInt_t ipt, UInt_t istop )
{
  // Split a roc into banks, if using bank structure
  // Then loop over slots and decode it from a bank if the slot
  // belongs to a bank.
  assert( evbuffer && fMap );
  if( roc >= MAXROC ) {
    ostringstream ostr;
    ostr << "ROC number " << roc << " out of range";
    throw logic_error(ostr.str());
  }
  if( istop >= event_length )
    throw logic_error("Stop point exceeds event length (?!)");

  if (!fMap->isBankStructure(roc))
    return HED_OK;

  if( fDoBench ) fBench->Begin("bank_decode");
  if (fDebugFile)
    *fDebugFile << "CodaDecode:: bank_decode  ... " << roc << "   " << ipt
                << "  " << istop << endl;

  fBlockIsDone = false;
  UInt_t pos = ipt+1;  // ipt points to ROC ID word
  while (pos < istop) {
    UInt_t len = evbuffer[pos];
    UInt_t head = evbuffer[pos+1];
    UInt_t bank = head >> 16;
    if( fDebugFile )
      *fDebugFile << "bank 0x" << hex << bank << "  head 0x" << head
                  << "    len 0x" << len << dec << endl;

    UInt_t key = (roc << 16) + bank;
    // Bank numbers only appear once, else bug in CODA or corrupt input
    assert( find(ALL(bankdat), key) == bankdat.end());
    // If len == 0, bug in CODA or corrupt input
    assert( len > 0 );
    if( len > 0 )
      bankdat.emplace_back(key, pos + 2, len - 1);
    pos += len+1;
  }

  for( auto slot : fMap->GetUsedSlots(roc) ) {
    assert(fMap->slotUsed(roc,slot));
    Int_t bank = fMap->getBank(roc, slot);
    auto* theBank = CheckForBank(roc, slot);
    if( !theBank )
      continue; // skip non-bank mode modules and banks not found in present event
    if (fDebugFile)
      *fDebugFile << "CodaDecoder::bank_decode: loading bank "
                  << roc << "  " << slot << "   " << bank << "  "
                  << theBank->pos << "   " << theBank->len << endl;
    auto* sd = crateslot[idx(roc, slot)].get();
    sd->LoadBank(evbuffer, theBank->pos, theBank->len);
    if( sd->IsMultiBlockMode() )
      fMultiBlockMode = true;
    if( sd->BlockIsDone() )
      fBlockIsDone = true;
  }

  if( fDoBench ) fBench->Stop("bank_decode");
  return HED_OK;
}

//_____________________________________________________________________________
 Int_t CodaDecoder::FillBankData( UInt_t *rdat, UInt_t roc, Int_t bank,
                                  UInt_t offset, UInt_t num ) const
{
  if( fDebug > 1 )
    cout << "Into FillBankData v1 " << dec << roc << "  " << bank << "  "
         << offset << "  " << num << endl;
  if( fDebugFile )
    *fDebugFile << "Check FillBankData " << roc << "  " << bank << endl;

  if( roc >= MAXROC )
    return HED_ERR;
  UInt_t jk = (roc << 16) + bank;
  auto bankInfo = find(ALL(bankdat), jk);
  if( bankInfo == bankdat.end() ) {
    cerr << "FillBankData::ERROR:  bankdat not in current event "<<endl;
    return HED_ERR;
  }
  UInt_t pos = bankInfo->pos;
  UInt_t len = bankInfo->len;
  if( fDebug > 1 )
    cout        << "FillBankData: pos, len " << pos << "   " << len << endl;
  if( fDebugFile )
    *fDebugFile << "FillBankData  pos, len " << pos << "   " << len << endl;
  assert( pos < event_length && pos+len <= event_length ); // else bug in bank_decode
  if( offset+2 > len )
    return HED_ERR;
  if( num > len )
    num = len;
  UInt_t ilo = pos + offset;
  assert( ilo < event_length );  // else offset not correctly checked above
  UInt_t ihi = pos + offset + num;
  if( ihi > event_length )
    ihi = event_length;
  memcpy(rdat, buffer+ilo, ihi-ilo);

  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadIfFlagData(const UInt_t* evbuffer)
{
  // Need to generalize this ...  too Hall A specific
  //
  // Looks for buffer mode and synch problems.  The latter are recoverable
  // but extremely rare, so I haven't bothered to write recovery a code yet,
  // but at least this warns you.
  // Returns 0 if no flag data detected, != 0 otherwise
  assert( evbuffer );
  UInt_t word = *evbuffer;
  if (fDebugFile)
    *fDebugFile << "CodaDecode:: TestBit on :  Flag data ? "
                << hex << word << dec << endl;
  UInt_t stdslot = word >> 27;
  if( stdslot > 0 && stdslot < MAXSLOT_FB )
    return 0; // non-flag data

  Int_t ret = 1;
  UInt_t upword = word >> 16;   // upper 16 bits
  word &= 0xffff;               // lower 16 bits
  switch( upword ) {
  case 0xdc00:
    if( word == 0x00ff )        // word == 0xdc0000ff
      synchmiss = true;
    break;
  case 0xdcfe:
    synchextra = true;
    if( fDebug > 0 ) {
      UInt_t slot = word >> 11;
      UInt_t nhit = word & 0x7ff;
      cout << "CodaDecoder: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits " << nhit << endl;
    }
    break;
  case 0xfaaa:
    break;
  case 0xfabc:
    if( fDebug > 0 && (synchmiss || synchextra) ) {
      UInt_t datascan = *(evbuffer+3);
      cout << "CodaDecoder: WARNING: Synch problems !" << endl;
      cout << "Data scan word 0x" << hex << datascan << dec << endl;
    }
    break;
  case 0xfabb:
    buffmode = false;
    break;
  case 0xfafb:
    if( (word >> 8) == 0xbf ) { // word & 0xffffff00 == 0xfafbbf00
      buffmode = true;
      //    synchflag = word&0xff; // not used
    }
    break;
  default:
    break;
  }
  return ret;
}


//_____________________________________________________________________________
Int_t CodaDecoder::FindRocs(const UInt_t *evbuffer) {

// The (old) decoding of CODA 2.* event buffer to find pointers and lengths of ROCs
// ROC = ReadOut Controller, synonymous with "crate".

  assert( evbuffer );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  // The following line is not meaningful for CODA3
  if( (evbuffer[1]&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( event_type > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  for_each(ALL(rocdat), []( RocDat_t& ROC ) { ROC.clear(); });
  // Set pos to start of first ROC data bank
  UInt_t pos = evbuffer[2]+3;  // should be 7
  nroc = 0;
  while( pos+1 < event_length && nroc < MAXROC ) {
    UInt_t len  = evbuffer[pos];
    UInt_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
    if( iroc>=MAXROC ) {
#ifdef FIXME
      if(fDebug>0) {
        cout << "ERROR in EvtTypeHandler::FindRocs "<<endl;
        cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
#endif
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }

  if (fDebugFile) {
    *fDebugFile << "CodaDecode:: num rocs "<<dec<<nroc<<endl;
    for( UInt_t i = 0; i < nroc; i++ ) {
      UInt_t iroc = irn[i];
      *fDebugFile << "   CodaDecode::   roc  num "<<iroc<<"   pos "<<rocdat[iroc].pos<<"     len "<<rocdat[iroc].len<<endl;
    }
  }

  return status;

}

//_____________________________________________________________________________
Int_t CodaDecoder::FindRocsCoda3(const UInt_t *evbuffer) {

// Find the pointers and lengths of ROCs in CODA3
// ROC = ReadOut Controller, synonymous with "crate".
// Earlier we had decoded the Trigger Bank in method trigBankDecode.
// This filled the tbank structure.
// For CODA3, the ROCs start after the Trigger Bank.

  UInt_t pos = 2 + tbank.len;
  nroc=0;

  for_each(ALL(rocdat), []( RocDat_t& ROC ) { ROC.clear(); });

  while (pos+1 < event_length) {
    UInt_t len = evbuffer[pos];          /* total Length of ROC Bank data */
    UInt_t iroc = (evbuffer[pos+1]&0x0fff0000)>>16;   /* ID of ROC is 12 bits*/
    if( iroc >= MAXROC ) {
      return HED_ERR;
    }
    rocdat[iroc].len = len;
    rocdat[iroc].pos = pos;
    irn[nroc] = iroc;
    pos += len+1;
    nroc++;
  }

  /* Sanity check:  Check if number of ROCs matches */
  if(nroc != tbank.nrocs) {
      printf(" ****ERROR: Trigger and Physics Block sizes do not match (%d != %d)\n",nroc,tbank.nrocs);
// If you are reading a data file originally written with CODA 2 and then
// processed (written out) with EVIO 4, it will segfault. Do as it says below.
      printf("This might indicate a file written with EVIO 4 that was a CODA 2 file\n");
      printf("Try  analyzer->SetCodaVersion(2)  in the analyzer script.\n");
      return HED_ERR;
  }

  if (fDebugFile) {  // debug

    *fDebugFile << endl << "  FindRocsCoda3 :: Starting Event number = " << dec << tbank.evtNum;
    *fDebugFile << endl;
    *fDebugFile << "    Trigger Bank Len = "<<tbank.len<<" words "<<endl;
    *fDebugFile << "    There are "<<nroc<<"  ROCs"<<endl;
    for( UInt_t i = 0; i < nroc; i++ ) {
      *fDebugFile << "     ROC ID = "<<irn[i]<<"  pos = "<<rocdat[irn[i]].pos
          <<"  Len = "<<rocdat[irn[i]].len<<endl;
    }
    *fDebugFile << "    Trigger BANK INFO,  TAG = "<<hex<<tbank.tag<<dec<<endl;
    *fDebugFile << "    start "<<hex<<tbank.start<<"      blksize "<<dec<<tbank.blksize
        <<"  len "<<tbank.len<<"   tag "<<tbank.tag<<"   nrocs "<<tbank.nrocs<<"   evtNum "<<tbank.evtNum;
    *fDebugFile << endl;
    *fDebugFile << "         Event #       Time Stamp       Event Type"<<endl;
    for( UInt_t i = 0; i < tbank.blksize; i++ ) {
      if( tbank.evTS ) {
          *fDebugFile << "      "<<dec<<tbank.evtNum+i<<"   "<<tbank.evTS[i]<<"   "<<tbank.evType[i];
          *fDebugFile << endl;
       } else {
          *fDebugFile << "     "<<tbank.evtNum+i<<"(No Time Stamp)   "<<tbank.evType[i];
          *fDebugFile << endl;
       }
       *fDebugFile << endl<<endl;
    }
  }

  return 1;

}

//_____________________________________________________________________________
// To initialize the THaSlotData member on first call to decoder
int CodaDecoder::init_slotdata()
{
  // Initialize all slots defined in the crate map as "used".
  //
  // This defeats the clever on-demand mechanism of the original decoder,
  // which would only allocate resources for a crate/slot if data for that
  // slot were actually encountered in the data stream. Now, resources
  // are allocated for everything, whether actually active or not.
  // The advantage is that one can configure the module objects associated
  // with each crate/slot. Of course, that could be done via database
  // parameters as well. Presumably the resource waste is minor on today's
  // computers, so fine.

  if(!fMap) return HED_ERR;

  try {
    for( auto iroc : fMap->GetUsedCrates() ) {
      assert(fMap->crateUsed(iroc));
      for( auto islot : fMap->GetUsedSlots(iroc) ) {
        assert(fMap->slotUsed(iroc, islot));
        makeidx(iroc, islot);
        if( fDebugFile )
          *fDebugFile << "CodaDecode::  crate, slot " << iroc << "  " << islot
                      << "   Dev type  = " << crateslot[idx(iroc, islot)]->devType()
                      << endl;
      }
    }
  }
  catch( const exception& e ) {
    cerr << e.what() << endl;
    return HED_FATAL;
  }

  if (fDebugFile)
    *fDebugFile << "CodaDecode:: fNSlotUsed "<<fSlotUsed.size()<<endl;

  // Update lists of used/clearable slots in case crate map changed
  return THaEvData::init_slotdata();
}


//_____________________________________________________________________________
void CodaDecoder::dump(const UInt_t* evbuffer) const
{
  if( !evbuffer ) return;
  if ( !fDebugFile ) return;
  *fDebugFile << "\n\n Raw Data Dump  " << endl;
  *fDebugFile << "\n Event number  " << dec << event_num;
  *fDebugFile << "  length " << event_length << "  type " << event_type << endl;
  UInt_t ipt = 0;
  for( UInt_t j = 0; j < (event_length/5); j++ ) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for( UInt_t k = j; k < j+5; k++ ) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
  }
  if (ipt < event_length) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for( UInt_t k = ipt; k < event_length; k++ ) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
    *fDebugFile << endl;
  }
  *fDebugFile<<dec<<endl;
}

//_____________________________________________________________________________
void CodaDecoder::CompareRocs()
{
  if (!fMap || !fDebugFile) return;
  *fDebugFile<< "Comparing cratemap rocs with found rocs"<<endl;
  for( UInt_t i = 0; i < nroc; i++ ) {
    UInt_t iroc = irn[i];
    if( !fMap->crateUsed(iroc) ) {
      *fDebugFile << "ERROR  CompareRocs:: roc " << iroc << "  in data but not in map" << endl;
    }
  }
  for( UInt_t iroc1 = 0; iroc1 < MAXROC; iroc1++ ) {
    if( !fMap->crateUsed(iroc1) ) continue;
    Bool_t ifound = false;
    for( UInt_t i=0; i<nroc; i++ ) {
      UInt_t iroc2 = irn[i];
      if (iroc1 == iroc2) {
        ifound = true;
        break;
      }
    }
    if (!ifound) *fDebugFile << "ERROR: CompareRocs:  roc "<<iroc1<<" in cratemap but not found data"<<endl;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlot( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt,
                             UInt_t istop )
{
  const UInt_t* p     = evbuffer+ipt;   // Points to ROC ID word (1 before data)
  const UInt_t* pstop = evbuffer+istop; // Points to last word of data in roc
  while( p++ < pstop ) {
    UInt_t slot = *p >> 27;  // A "self-reported" slot.
    if( slot == 0 || slot >= MAXSLOT_FB )  // Used for diagnostic data words
      continue;
    UInt_t index = MAXSLOT_FB*roc + slot;
    if( slot > 0 && index < MAXROCSLOT_FB )
      fbfound[index] = true;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlots()
{
  // This checks the fastbus slots to see if slots are appearing in both the
  // data and the cratemap.  If they appear in one but not the other, a warning
  // is issued, which usually means the cratemap is wrong.
  for( UInt_t iroc = 0; iroc < MAXROC; iroc++ ) {
    if( !fMap->isFastBus(iroc) ) continue;
    for( UInt_t islot = 0; islot < MAXSLOT_FB; islot++ ) {
      UInt_t index = MAXSLOT_FB * iroc + islot;
      bool inEvent = fbfound[index], inMap = fMap->slotUsed(iroc, islot);
      if( inEvent ) {
        if( inMap ) {
          if (fDebugFile)
            *fDebugFile << "FB slot in cratemap and in data.  (good!).  "
                        << "roc = "<<iroc<<"   slot = "<<islot<<endl;
        } else {
          if (fDebugFile)
            *fDebugFile << "FB slot in data, but NOT in cratemap  (bad!).  "
                        << "roc = "<<iroc<<"   slot = "<<islot<<endl;
          Warning("ChkFbSlots", "Fastbus module in (roc,slot) = (%d,%d)  "
                                "found in data but NOT in cratemap !", iroc, islot);
        }
      } else if( inMap ) {
        if (fDebugFile)
          *fDebugFile << "FB slot NOT in data, but in cratemap  (bad!).  "
                      << "roc = "<<iroc<<"   slot = "<<islot<<endl;
        // Why do we care? If the cratemap has info about additional hardware
        // that just wasn't read out by the DAQ, so what?
        //Warning("ChkFbSlots", "Fastbus module in (roc,slot) = = (%d,%d)  "
        //               "found in cratemap but NOT in data !", iroc, islot);
      }
    }
  }
}

//_____________________________________________________________________________
void CodaDecoder::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters) for the new time.

  if( fRunTime == tloc )
    return;
  fRunTime = tloc;
  fNeedInit = true;  // force re-init
}

//_____________________________________________________________________________
Int_t CodaDecoder::prescale_decode_coda2(const UInt_t* evbuffer)
{
  // Decodes prescale factors from either
  // TS_PRESCALE_EVTYPE(default) = PS factors
  // read from Trig. Super. registers (since 11/03)
  //   - or -
  // PRESCALE_EVTYPE = PS factors from traditional
  //     "prescale.dat" file.

  assert( evbuffer );
  assert( event_type == TS_PRESCALE_EVTYPE ||
          event_type == PRESCALE_EVTYPE );
  const UInt_t HEAD_OFF1 = 2;
  const UInt_t HEAD_OFF2 = 4;
  static const char* const pstr[] = { "ps1", "ps2", "ps3", "ps4",
                                      "ps5", "ps6", "ps7", "ps8",
                                      "ps9", "ps10", "ps11", "ps12" };
  // TS registers -->
  if( event_type == TS_PRESCALE_EVTYPE) {
    // this is more authoritative
    for( UInt_t j = 0; j < 8; j++ ) {
      UInt_t k = j + HEAD_OFF1;
      UInt_t ps = 0;
      if( k < event_length ) {
        ps = evbuffer[k];
        if( psfact[j] != 0 && ps != psfact[j] ) {
          Warning("prescale_decode","Mismatch in prescale factor: "
                  "Trig %u  oldps %u   TS_PRESCALE %d. Setting to TS_PRESCALE",
                  j+1,psfact[j],ps);
        }
      }
      psfact[j]=ps;
      if (fDebug > 1)
        cout << "%% TS psfact "<<dec<<j<<"  "<<psfact[j]<<endl;
    }
  }
  // "prescale.dat" -->
  else if( event_type == PRESCALE_EVTYPE ) {
    if( event_length <= HEAD_OFF2 )
      return HED_ERR;  //oops, event too short?
    THaUsrstrutils sut;
    sut.string_from_evbuffer(evbuffer+HEAD_OFF2, event_length-HEAD_OFF2);
    for(Int_t trig=0; trig<MAX_PSFACT; trig++) {
      UInt_t ps =  sut.getint(pstr[trig]);
      UInt_t psmax = 65536; // 2^16 for trig > 3
      if (trig < 4) psmax = 16777216;  // 2^24 for 1st 4 trigs
      if (trig > 7) ps = 1;  // cannot prescale trig 9-12
      ps = ps % psmax;
      if (psfact[trig]==kMaxUInt) // not read before
        psfact[trig] = ps;
      else if (ps != psfact[trig]) {
        Warning("prescale_decode","Mismatch in prescale factor: "
                "Trig %d  oldps %d   prescale.dat %d, Keeping old value",
                trig+1,psfact[trig],ps);
      }
      if (fDebug > 1)
        cout << "** psfact[ "<<trig+1<< " ] = "<<psfact[trig]<<endl;
    }
  }

  // Ok in any case
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::prescale_decode_coda3(const UInt_t* evbuffer)
{
  // Decodes prescale factors from either
  // TS_PRESCALE_EVTYPE(default) = PS factors
  // This version is for CODA 3 data.
  //  0 means no prescaling, what we called "1" before.
  // -1 means the trigger is disabled.  "infinite" prescale
  //  otherwise the value is 2^(ps-1) + 1.
  //    ps  ..... prescale factor
  //     1 ....... 2
  //     2 ....... 3
  //     3 ....... 5
  //     5 ....... 17
  //    10 ....... 513

  static const char* const here = "prescale_decode_coda3";

  assert( evbuffer );
  assert( event_type == TS_PRESCALE_EVTYPE ||
          event_type == PRESCALE_EVTYPE );
  const UInt_t HEAD_OFF1 = 2;
  const UInt_t HEAD_OFF2 = 4;
  UInt_t super_big = 999999;
  static const char* const pstr[] = { "ps1", "ps2", "ps3", "ps4",
                                      "ps5", "ps6", "ps7", "ps8",
                                      "ps9", "ps10", "ps11", "ps12" };
  // TS registers -->
  // don't have these yet for CODA3.  hmmm... that reminds me to do it.
  if( event_type == TS_PRESCALE_EVTYPE) {
    // this is more authoritative
    for( UInt_t j = 0; j < 8; j++ ) {
      UInt_t k = j + HEAD_OFF1;
      UInt_t ps = 0;
      if( k < event_length ) {
        ps = evbuffer[k];
        if( psfact[j] != 0 && ps != psfact[j] ) {
          Warning(here,"Mismatch in prescale factor: "
                  "Trig %u  oldps %u   TS_PRESCALE %d. Setting to TS_PRESCALE",
                  j+1,psfact[j],ps);
        }
      }
      psfact[j]=ps;
      if (fDebug > 1)
        cout << "%% TS psfact "<<dec<<j<<"  "<<psfact[j]<<endl;
    }
  }
  // "prescale.dat" -->
  else if( event_type == PRESCALE_EVTYPE ) {
    if( event_length <= HEAD_OFF2 )
      return HED_ERR;  //oops, event too short?
    THaUsrstrutils sut;
    sut.string_from_evbuffer(evbuffer+HEAD_OFF2, event_length-HEAD_OFF2);

    for( Int_t trig = 0; trig < MAX_PSFACT; trig++ ) {
      long ps = sut.getSignedInt(pstr[trig]);
      if( ps == -1 ) {
        // The trigger is actually off, not just "super big" but infinite.
        ps = super_big;
      } else if( ps == 0 ) {
        ps = 1;
      } else if( ps >= 1 && ps <= 16 ) {
        // ps = 2^(val-1)+1 (sic)
        ps = 1 + (1U << (ps - 1));
      } else { // ps > 16 or ps < -1
        // ps has 4 bits in hardware, so this indicates a misconfiguration
        Error(here, "Invalid prescale = %ld in prescale.dat. "
                    "Must be between -1 and 16", ps);
        ps = -1;
      }
      if( psfact[trig] == kMaxUInt ) // not read before
        psfact[trig] = (UInt_t)ps;
      else if( ps != psfact[trig] ) {
        Warning(here, "Mismatch in prescale factor: "
                      "Trig %d  oldps %d   prescale.dat %ld, Keeping old value",
                trig + 1, (Int_t)psfact[trig], ps);
      }
      if( fDebug > 1 )
        cout << "** psfact[ " << trig + 1 << " ] = " << psfact[trig] << endl;
    }
  }

  // Ok in any case
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::SetCodaVersion( Int_t version )
{
  if (version != 2 && version != 3) {
    cout << "ERROR::CodaDecoder::SetCodaVersion version " << version;
    cout << "  must be 2 or 3 only." << endl;
    cout << "Not setting CODA version ! " << endl;
    return -1;
  }
  return (fDataVersion = version);
}

//_____________________________________________________________________________

} // namespace Decoder

ClassImp(Decoder::CodaDecoder)
