//*-- Author :    Ole Hansen   16/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// The standard detector map for a Hall A detector.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetMap.h"
#include "Decoder.h"      // for EModuleType
#include "Helper.h"       // for ToInt, ALL
#include "Module.h"       // for Module
#include "TError.h"       // for Error
#include "THaAnalyzer.h"  // for THaAnalyzer
#include "THaCrateMap.h"  // for THaCrateMap
#include "THaEvData.h"    // for THaEvData
#include "TString.h"      // for TString
#include "Textvars.h"     // for Tokenize
#include <algorithm>      // for min, copy_n, max, find_if, sort
#include <cassert>        // for assert
#include <cctype>         // for isalpha, isdigit
#include <charconv>       // for from_chars
#include <iomanip>        // for setw
#include <iostream>       // for cout, endl, dec, right
#include <iterator>       // for size
#include <memory>         // for unique_ptr, make_unique
#include <set>            // for operator!=
#include <sstream>        // for ostringstream
#include <stdexcept>      // for logic_error, out_of_range, invalid_argument
#include <system_error>   // for errc
#include <type_traits>    // for is_integral_v
#include <utility>        // for cmp_greater_equal, operator<=>, move, operator==
#include <vector>         // for vector

using namespace std;
using namespace Decoder;
using namespace Podd;

static unique_ptr<THaCrateMap> fgCrateMap = nullptr;

//_____________________________________________________________________________
Int_t THaDetMap::InitCmap( Long64_t tloc )
{
  if( fgCrateMap && fgCrateMap->GetInitTime() == tloc )
    return THaCrateMap::CM_OK;
  if( !fgCrateMap ) {
    TString cmap_name;
    if( auto* analyzer = THaAnalyzer::GetInstance() ) {
      if( auto* decoder = analyzer->GetDecoder() )
        cmap_name = decoder->GetCrateMapName();
    }
    if( cmap_name.IsNull() )
      cmap_name = THaEvData::GetDefaultCrateMapName();
    assert(!cmap_name.IsNull());
    fgCrateMap = make_unique<THaCrateMap>(cmap_name);
  }
  return fgCrateMap->init(tloc);
}

//_____________________________________________________________________________
THaCrateMap* THaDetMap::GetCrateMap()
{
  return fgCrateMap.get();
}

//_____________________________________________________________________________
void THaDetMap::Module::SetModel( Int_t mod )
{
  model = mod;
  // Find the model number in the module registry
  const auto& modtypes = Decoder::Module::fgModuleTypes();
  if( auto it = modtypes.find(mod); it != modtypes.end() ) {
    auto module = *it;
    type = module.fType;
    //TODO other parameters?
  } else
    type = EModuleType::kUndefined;
}

//_____________________________________________________________________________
void THaDetMap::Module::SetTDCMode( Bool_t cstart )
{
  cmnstart = cstart;
  if( cstart && type == EModuleType::kCommonStopTDC )
    type = EModuleType::kCommonStartTDC;
  else if( !cstart && type == EModuleType::kCommonStartTDC )
    type = EModuleType::kCommonStopTDC;
}

//_____________________________________________________________________________
void THaDetMap::Module::MakeTDC()
{
  if( IsCommonStart() )
    type = EModuleType::kCommonStartTDC;
  else
    type = EModuleType::kCommonStopTDC;
}

//_____________________________________________________________________________
void THaDetMap::Module::MakeADC()
{
  type = EModuleType::kADC;
}

//_____________________________________________________________________________
void THaDetMap::CopyMap( const ModuleVec_t& map )
{
  // Deep-copy the vector of module pointers
  fMap.reserve(map.capacity());
  for( const auto& m : map ) {
    fMap.emplace_back(std::make_unique<Module>(*m));
  }
}

//_____________________________________________________________________________
THaDetMap::THaDetMap( const THaDetMap& rhs )
  : fStartAtZero(rhs.fStartAtZero), fHasTags(rhs.fHasTags)
{
  // Copy constructor

  CopyMap(rhs.fMap);
}

//_____________________________________________________________________________
THaDetMap& THaDetMap::operator=( const THaDetMap& rhs )
{
  // THaDetMap assignment operator

  if( this != &rhs ) {
    fMap.clear();
    CopyMap(rhs.fMap);
    fStartAtZero = rhs.fStartAtZero;
    fHasTags = rhs.fHasTags;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaDetMap::AddModule( UInt_t crate, UInt_t slot,
                            UInt_t chan_lo, UInt_t chan_hi,
                            UInt_t first, Int_t model, Int_t refindex,
                            Int_t refchan, UInt_t plane, UInt_t signal,
                            const string& tag )
{
  // Add a module to the map.

  // Logical channels can run either "with" or "against" the physical channel
  // numbers:
  // lo<=hi :    lo -> first
  //             hi -> first+hi-lo
  //
  // lo>hi  :    hi -> first
  //             lo -> first+lo-hi
  //
  // To indicate the second case, The flag "reverse" is set to true in the
  // module. The fields lo and hi are reversed so that lo<=hi always.
  //
  bool reverse = (chan_lo > chan_hi);

  auto pm = make_unique<Module>();
  Module& m = *pm;
  m.crate = crate;
  m.slot  = slot;
  if( reverse ) {
    m.lo = chan_hi;
    m.hi = chan_lo;
  } else {
    m.lo = chan_lo;
    m.hi = chan_hi;
  }
  m.first = first;
  m.model = model;
  m.refindex = refindex;
  m.refchan  = refchan;
  m.plane = plane;
  m.signal = signal;

  // Assign tag. Only up to 4 characters to keep the structure small.
  auto sz = std::min(tag.size(), sizeof(m.tag) - 1);
  std::copy_n(tag.begin(), sz, m.tag);
  m.tag[sz] = '\0'; // ensure m.tag[] is a C-string

  m.reverse = reverse;
  m.cmnstart = false;

  // Set the module type using our lookup table of known model numbers.
  // If the model is not predefined, this can be done manually later with
  // calls to MakeADC()/MakeTDC().
  if( model != 0 ) {
    m.SetModel( model );
      //      return -1;  // Unknown module TODO do when module_list is in a database
  } else
    m.type = EModuleType::kUndefined;

  fMap.push_back(std::move(pm));
  return ToInt(GetSize());
}

//_____________________________________________________________________________
THaDetMap::Module* THaDetMap::Find( UInt_t crate, UInt_t slot,
                                    UInt_t chan )
{
  // Return the module containing crate, slot, and channel chan.
  // If several matching modules exist (which mean the map is misconfigured),
  // only the first one is returned. If none match, return nullptr.
  // Since the map is usually small and not necessarily sorted, a simple
  // linear search is done.

  auto found = ranges::find_if( fMap,
    [crate,slot,chan](const unique_ptr<Module>& d)
    { return ( d->crate == crate && d->slot == slot &&
               d->lo <= chan && chan <= d->hi );
    });

  return (found != fMap.end()) ? found->get() : nullptr;
}

//_____________________________________________________________________________
Int_t THaDetMap::Fill( const vector<Int_t>& values, UInt_t flags )
{
  // Fill the map with 'values'. Depending on 'flags', the values vector
  // is interpreted as a 4-, 5-, 6- or 7-tuple:
  //
  // The first 4 values are interpreted as (crate,slot,start_chan,end_chan)
  // Each of the following flags causes one more value to be used as part of
  // the tuple for each module:
  //
  // kFillLogicalChannel - Logical channel number for 'start_chan'.
  // kFillModel          - The module's hardware model number (see AddModule())
  // kFillRefChan        - Reference channel (for pipeline TDCs etc.)
  // kFillRefIndex       - Reference index (for pipeline TDCs etc.)
  // kFillPlane          - Which plane in detector (for Hall C)
  // kFillSignal         - Which signal type (for Hall C)
  //
  // If more than one flag is present, the numbers will be interpreted
  // in the order the flags are listed above.
  // Example:
  //      flags = kFillModel | kFillRefChan
  // ==>
  //      the vector is interpreted as a series of 6-tuples in the order
  //      (crate,slot,start_chan,end_chan,model,refchan).
  //
  // If kFillLogicalChannel is not set then the first logical channel numbers
  // are automatically calculated for each module, assuming the numbers are
  // sequential.
  //
  // By default, an existing map is overwritten. If the flag kDoNotClear
  // is present, then the data are appended.
  //
  // The return value is the number of modules successfully added,
  // or negative if an error occurred.

  typedef vector<Int_t>::size_type vsiz_t;

  if( (flags & kFillLogicalChannel) && (flags & kSkipLogicalChannel) )
    // Bad combination of flags
    return -128;

  if( (flags & kDoNotClear) == 0 )
    Clear();

  vsiz_t tuple_size = 4;
  if( flags & kFillLogicalChannel || flags & kSkipLogicalChannel )
    tuple_size++;
  if( flags & kFillModel )
    tuple_size++;
  if( flags & kFillRefChan )
    tuple_size++;
  if( flags & kFillRefIndex )
    tuple_size++;
  if( flags & kFillPlane )
    tuple_size++;
  if( flags & kFillSignal )
    tuple_size++;

  // For historical reasons, logical channel numbers start at 1
  UInt_t prev_first = 1, prev_nchan = 0;
  // Defaults for optional values
  UInt_t first = 0;
  Int_t  plane = 0, signal = 0, model = 0, rchan = -1, ref = -1;

  Int_t ret = 0;
  for( vsiz_t i = 0; i < values.size(); i += tuple_size ) {
    // For compatibility with older maps, crate < 0 means end of data
    if( values[i] < 0 )
      break;
    // Now we require a full tuple
    if( i + tuple_size > values.size() ) {
      ret = -127;
      break;
    }

    vsiz_t k = 4;
    if( flags & kFillLogicalChannel ) {
      first = values[i + k++];
      if( first == 0 )
        // This map appears to start counting channels at zero
        // This is actually the exception. For historical reasons (Fortran),
        // most maps start counting at 1.
        // This flag is used by the hit iterators only; existing Decode()
        // methods will not be affected.
        fStartAtZero = true;
    } else {
      if( flags & kSkipLogicalChannel ) {
        ++k;
      }
      first = prev_first + prev_nchan;
    }
    if( flags & kFillModel )
      model = values[i + k++];
    if( flags & kFillRefChan )
      rchan = values[i + k++];
    if( flags & kFillRefIndex )
      ref = values[i + k++];
    if( flags & kFillPlane )
      plane = values[i + k++];
    if( flags & kFillSignal )
      signal = values[i + k++];

    auto crate = static_cast<UInt_t>(values[i]);
    auto slot  = static_cast<UInt_t>(values[i+1]);
    auto ch_lo = static_cast<UInt_t>(values[i+2]);
    auto ch_hi = static_cast<UInt_t>(values[i+3]);

    // If no model given, try to retrieve it from the crate map
    // FIXME: This sort-of assumes that the crate map was read successfully
    //  but for now we'll let the caller deal with that.
    if( model == 0 && fgCrateMap )
      model = fgCrateMap->getModel(crate, slot);

    ret = AddModule(crate, slot, ch_lo, ch_hi,
                    first, model, ref, rchan, plane, signal);
    if( ret <= 0 )
      break;
    prev_first = first;
    prev_nchan = GetNchan(ret - 1);
  }

  return ret;
}

//_____________________________________________________________________________
namespace {
template<typename T> requires std::is_integral_v<T>
Int_t ParseInt( const string_view str, T& val )
{
  constexpr auto noerr = std::errc{}; // NOLINT(*-invalid-enum-default-initialization)
  auto [ptr, ec] = std::from_chars(ALL(str), val);
  return ptr != str.end() || ec != noerr;
}
}

//_____________________________________________________________________________
THaDetMap::FillResult
THaDetMap::FillImpl( string_view dbtxt, UInt_t flags )
{
  // Parser for v2 detector map definitions

  Int_t ret = 0;
  // For historical reasons, logical channel numbers start at 1
  UInt_t first = 0, prev_first = 1, prev_nchan = 0;

  if( (flags & kDoNotClear) == 0 )
    Clear();

  vector<string_view> lines;
  Tokenize(dbtxt,",",lines);
  for( const auto& line: lines ) {
    vector<string_view> tok;
    Tokenize(line," \t\n\r\v\f",tok);
    assert(!tok.empty());  // else bug in LoadDBvalue or Tokenize

    // Parse optional "tag". If present, it must start with a letter
    std::size_t idx = 0;
    string tag;
    auto c = tok.at(0).at(0);
    if( isalpha(c) ) {
      tag = tok[idx++];
      fHasTags = true;
    }

    // Must have at least 4 numbers after the optional tag
    if( tok.size() < 4U + !tag.empty() )
      return {.err = kNotEnoughData, .ret = ret, .field = string(dbtxt)};

    // Parse crate, slot, chan_lo, chan_hi
    Int_t tup[4];
    for( std::size_t i = 0; i < std::size(tup); ++i, ++idx ) {
      if( ParseInt(tok.at(idx), tup[i]) )
        return {.err = kConversionError, .ret = ret, .field = string(tok[idx])};
      // For compatibility with old maps, crate < 0 (actually any of
      // crate/slot/lo/hi < 0) means end of data
      if( tup[i] < 0 )
        return {.err = kOK, .ret = ret};
    }

    // Logical channel: optional 5th number
    if( idx < tok.size() && isdigit(tok[idx].at(0)) ) {
      if( ParseInt(tok[idx], first) )
        return {.err = kConversionError, .ret = ret, .field = string(tok[idx])};
      ++idx;
      if( first == 0 )
        fStartAtZero = true;
    } else
      first = prev_first + prev_nchan;

    // Optional tagged parameters: "M" (model), "R" (ref channel),
    // "I" (ref index), "P" (plane), "S" (signal). Defaults follow:
    Int_t plane = 0, signal = 0, model = 0, rchan = -1, ref = -1;
    for( ; idx < tok.size(); ++idx ) {
      const auto& t = tok.at(idx);
      c = t.at(0);
      if( t.size() < 3 || !isalpha(c) || t.at(1) != ':' )
        return {.err = kFormatError, .ret = ret, .field = string(t)};
      Int_t val;
      if( ParseInt(t.substr(2), val) )
        return {.err = kConversionError, .ret = ret, .field = string(t)};
      switch( c ) {
        case 'M':
          model = val;
          break;
        case 'R':
          rchan = val;
          break;
        case 'I':
          ref = val;
          break;
        case 'P':
          if( val < 0 )
            return {.err = kInvalidNumber, .ret = ret, .field = string(t)};
          plane = val;
          break;
        case 'S':
          if( val < 0 )
            return {.err = kInvalidNumber, .ret = ret, .field = string(t)};
          signal = val;
          break;
        default:
          // unsupported token
          return {.err = kUnknownTag, .ret = ret, .field = string(t)};
      }
    }
    auto [crate , slot, ch_lo, ch_hi] = tup; // for readability
    //TODO handle case when the model number here disagrees with the crate map
    if( model == 0 && fgCrateMap )
      model = fgCrateMap->getModel(crate, slot);

    ret = AddModule(crate, slot, ch_lo, ch_hi,
                    first, model, ref, rchan, plane, signal, tag);
    if( ret <= 0 ) {
      return {.err = kUnknownModel, .ret = ret, .field = to_string(model)};
    }
    prev_first = first;
    prev_nchan = GetNchan(ret - 1);
  }

  return {.err = kOK, .ret = ret };
}

//_____________________________________________________________________________
Int_t THaDetMap::Fill( string_view dbtxt, UInt_t flags )
{
  FillResult st{};
  Int_t prev_size = (flags & kDoNotClear) ? ToInt(GetSize()) : 0;
  try {
    st = FillImpl(dbtxt, flags);
  }
  catch( const std::out_of_range& e ) {
    st.err = kIndexOutOfBounds; // should never happen
    st.field = e.what();
  }
  if( st.err != kOK ) {
    string errtxt; errtxt.reserve(64);
    switch( st.err ) {
      case kNotEnoughData:
        errtxt = "not enough values";
        break;
      case kConversionError:
        errtxt = "conversion error";
        break;
      case kInvalidNumber:
        errtxt = "number must be positive";
        break;
      case kFormatError:
        errtxt = "format error";
        break;
      case kUnknownModel:
        errtxt = "unknown model";
        break;
      case kUnknownTag:
        errtxt = "unknown tag";
        break;
      case kIndexOutOfBounds:
        errtxt = "index out of bounds";
        break;
      default:
        errtxt = "unknown error code "; // not reached
        errtxt += to_string(st.err);
        break;
    }
    errtxt += ": \"";
    errtxt += st.field;
    errtxt += "\"";
    Error("THaDetMap::Fill<string>", "Fill failed: %s", errtxt.c_str());
    if( st.ret > prev_size ) {
      // Put the map back in the previous state (usually empty)
      Int_t n_add = st.ret - prev_size;
      auto to_erase = std::min(n_add, ToInt(fMap.size()));
      fMap.erase(fMap.end() - to_erase, fMap.end());
    }
    st.ret = -st.err;
  }
  return st.ret;  // Current map size
}

//_____________________________________________________________________________
UInt_t THaDetMap::GetTotNumChan() const
{
  // Get sum of the number of channels of all modules in the map. This is
  // typically the total number of hardware channels used by the detector.

  UInt_t sum = 0;
  for( const auto & m : fMap )
    sum += m->GetNchan();

  return sum;
}


//_____________________________________________________________________________
void THaDetMap::GetMinMaxChan( UInt_t& min, UInt_t& max, ECountMode mode ) const
{
  // Put the minimum and maximum logical or reference channel numbers
  // into min and max. If refidx is true, check refindex, else check logical
  // channel numbers.

  min = kMaxInt;
  max = kMinInt;
  bool do_ref = (mode == kRefIndex);
  for( const auto& m : fMap ) {
    UInt_t m_min = do_ref ? m->refindex : m->first;
    UInt_t m_max = do_ref ? m->refindex : m->first + m->hi - m->lo;
    min = std::min(m_min, min);
    max = std::max(m_max, max);
  }
}


//_____________________________________________________________________________
void THaDetMap::Print( Option_t* ) const
{
  // Print the contents of the map

  cout << "Size: " << GetSize() << endl;
  for( const auto& m : fMap ) {
    cout << " "
         << setw(5) << m->crate
         << setw(5) << m->slot
         << setw(5) << m->lo
         << setw(5) << m->hi
         << setw(5) << m->first
         << setw(5) << m->model;
    if( m->IsADC() )
      cout << setw(4) << " ADC";
    if( m->IsTDC() )
      cout << setw(4) << " TDC";
    cout << setw(5) << m->refchan
         << setw(5) << m->refindex
         << setw(5) << m->plane
         << setw(5) << m->signal
         << endl;
  }
}

//_____________________________________________________________________________
void THaDetMap::Clear()
{
  // Clear the map and reset the array size to zero.

  fMap.clear();
  fMap.shrink_to_fit(); // FWIW
  fStartAtZero = false;
  fHasTags = false;
}

//_____________________________________________________________________________
void THaDetMap::Sort()
{
  // Sort the map by crate/slot/low channel

  ranges::sort(fMap, []( const auto& a, const auto& b ) {
    if( a->crate < b->crate ) return true;
    if( a->crate > b->crate ) return false;
    if( a->slot  < b->slot )  return true;
    if( a->slot  > b->slot )  return false;
    return (a->lo < b->lo);
  });

}

//=============================================================================
THaDetMap::Iterator
THaDetMap::MakeIterator( const THaEvData& evdata )
{
  return {*this, evdata};
}

//_____________________________________________________________________________
THaDetMap::MultiHitIterator
THaDetMap::MakeMultiHitIterator( const THaEvData& evdata )
{
  return {*this, evdata};
}

//_____________________________________________________________________________
THaDetMap::Iterator::Iterator( const THaDetMap& detmap, const THaEvData& evdata,
                               bool do_init )
  : fDetMap(&detmap), fEvData(&evdata), fMod(nullptr), fNMod(fDetMap->GetSize()),
    fNTotChan(fDetMap->GetTotNumChan()), fNChan(0), fIMod(-1), fIChan(-1)
{
  fHitInfo.ev = fEvData->GetEvNum();
  // Initialize iterator state to point to the first item
  if( do_init )
    ++(*this);
}

//_____________________________________________________________________________
string THaDetMap::Iterator::msg( const char* txt ) const
{
  // Format message string for exceptions
  ostringstream ostr;
  ostr << "Event " << fHitInfo.ev << ", ";
  ostr << "channel "
       << fHitInfo.crate << "/" << fHitInfo.slot << "/"
       << fHitInfo.chan  << "/" << fHitInfo.hit << ": ";
  if( txt and *txt ) ostr << txt; else ostr << "Unspecified error.";
  return ostr.str();
}

#define CRATE_SLOT( x ) (x).crate, (x).slot
namespace {
bool LOOPDONE( Int_t i, UInt_t n ) {
  return i < 0 or std::cmp_greater_equal(i, n);
}
bool NO_NEXT( Int_t& i, UInt_t n ) {
  return std::cmp_greater_equal(i, n) or std::cmp_greater_equal(++i, n);
}
}

//_____________________________________________________________________________
void THaDetMap::Iterator::incr()
{
  // Advance iterator to next active channel
 nextmod:
  if( LOOPDONE(fIChan, fNChan) ) {
    if( NO_NEXT(fIMod, fNMod) ) {
      fHitInfo.reset();
      return;
    }
    fMod = fDetMap->GetModule(fIMod);
    if( !fMod )
      throw std::logic_error("NULL detector map module. Program bug. "
                             "Call expert.");
    fHitInfo.set_crate_slot(fMod);
    fHitInfo.module = fEvData->GetModule(CRATE_SLOT(fHitInfo));
    fNChan = fEvData->GetNumChan(CRATE_SLOT(fHitInfo));
    fIChan = -1;
  }
 nextchan:
  if( NO_NEXT(fIChan, fNChan) )
    goto nextmod;
  UInt_t chan = fEvData->GetNextChan(CRATE_SLOT(fHitInfo), fIChan);
  if( chan < fMod->lo or chan > fMod->hi )
    goto nextchan;  // Not one of my channels
  UInt_t nhit = fEvData->GetNumHits(CRATE_SLOT(fHitInfo), chan);
  fHitInfo.chan = chan;
  fHitInfo.nhit = nhit;
  if( nhit == 0 ) {
    ostringstream ostr;
    ostr << "No hits on active "
         << (fHitInfo.type == EModuleType::kADC ? "ADC" : "TDC")
         << " channel. Should never happen. Decoder bug. Call expert.";
    throw std::logic_error(msg(ostr.str().c_str()));
  }
  // If multiple hits on a TDC channel take the one earliest in time.
  // For a common-stop TDC, this is actually the last hit.
  fHitInfo.hit = (fHitInfo.type == EModuleType::kCommonStopTDC) ? nhit - 1 : 0;
  // Determine logical channel. Decode() methods that use this hit iterator
  // should always assume that logical channel numbers start counting from zero.
  Int_t lchan = fMod->ConvertToLogicalChannel(chan);
  if( fDetMap->fStartAtZero )
    ++lchan; // ConvertToLogicalChannel subtracts 1; undo that here
  if( lchan < 0 or static_cast<UInt_t>(lchan) >= size() ) {
    ostringstream ostr;
    size_t lmin = 1, lmax = size(); // Apparent range in the database file
    if( fDetMap->fStartAtZero ) { --lmin; --lmax; }
    ostr << "Illegal logical detector channel " << lchan << "."
         << "Must be between " << lmin << " and " << lmax << ". Fix database.";
    throw std::invalid_argument(msg(ostr.str().c_str()));
  }
  fHitInfo.lchan = lchan;
}

//_____________________________________________________________________________
void THaDetMap::Iterator::reset()
{
  // Reset iterator to first element, if any

  fNMod = fDetMap->GetSize();
  fIMod = fIChan = -1;
  fHitInfo.reset();
  ++(*this);
}

//_____________________________________________________________________________
THaDetMap::MultiHitIterator::MultiHitIterator( const THaDetMap& detmap,
                                               const THaEvData& evdata,
                                               bool do_init )
  : Iterator(detmap, evdata, false), fIHit(-1)
{
  if( do_init )
    ++(*this);
}

//_____________________________________________________________________________
void THaDetMap::MultiHitIterator::incr()
{
  // Advance iterator to next active hit or channel, allowing multiple hits
  // per channel
 nexthit:
  if( LOOPDONE(fIHit, fHitInfo.nhit) ) {
    Iterator::operator++();
    if( !Iterator::operator bool() )
      return;
    fIHit = -1;
  }
  if( NO_NEXT(fIHit, fHitInfo.nhit) )
    goto nexthit;
  fHitInfo.hit = fIHit; // overwrite value of single-hit case (0 or nhit-1)
}

//_____________________________________________________________________________
void THaDetMap::MultiHitIterator::reset()
{
  fIHit = -1;
  Iterator::reset();
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaDetMap)
#endif
