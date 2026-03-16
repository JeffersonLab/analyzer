/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e. whether slots are
//  fastbus or vme, what the module types are, and
//  what header info to expect.  Probably nobody needs
//  to know about this except the author, and at present
//  an object of this class is a private member of the decoder.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "THaCrateMap.h"
#include "Database.h"   // for OpenDBFile, ReadFile, GetTZOffsetToLocal, IsD...
#include "Decoder.h"    // for ECrateCode, ECrateCode::kScalerCrate, ECrateC...
#include "Helper.h"     // for ToInt, IntDigits
#include "Module.h"     // for Module
#include "TDatime.h"    // for TDatime
#include "TError.h"     // for Error, Warning
#include "Textvars.h"   // for Trim
#include <cassert>      // for assert
#include <algorithm>    // for all_of, any_of
#include <cctype>       // for isspace
#include <cerrno>       // for errno
#include <cstdio>       // for fclose, sscanf, size_t, FILE, ferror
#include <cstring>      // for strerror_r
#include <exception>    // for exception
#include <iomanip>      // for operator<<, setfill, setw
#include <iostream>     // for basic_ostream, operator<<, basic_ios, basic_i...
#include <ranges>       // for operator|, elements_view, values, views
#include <sstream>      // for basic_istringstream
#include <string>       // for basic_string, char_traits, string, operator<=>
#include <string_view>  // for basic_string_view, operator==, operator""sv
#include <utility>      // for get, operator<=>

static constexpr size_t kInitialMapSize = 64;

// This is a well-known problem with strerror_r
#if defined(__linux__) && (defined(_GNU_SOURCE) || !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE > 600))
# define GNU_STRERROR_R
#endif

using namespace std;
using namespace Podd;
using enum Decoder::ECrateCode;

namespace {
//_____________________________________________________________________________
string StrError()
{
  // Return description of current errno as a std::string
  constexpr size_t BUFLEN = 128;
  char buf[BUFLEN];

#ifdef GNU_STRERROR_R
  const char* s = strerror_r(errno, buf, BUFLEN);
  string ret = s ? string(s) : string("unknown error ") + to_string(errno);
#else
  strerror_r(errno, buf, BUFLEN);
  string ret = buf;
#endif
  return ret;
}

//_____________________________________________________________________________
const char* GetCrateTypeName( Decoder::ECrateCode code )
{
  static const std::map<Decoder::ECrateCode, const char*> type_names = {
    {kUnknown, "(unknown)"}, {kVME, "vme"}, {kFastbus, "fastbus"},
    {kCamac, "camac"}, {kScalerCrate, "scaler"}
  };
  auto it = type_names.find(code);
  return it != type_names.end() ? it->second : "(unknown)";
}

//_____________________________________________________________________________
void trim_left( string_view& str )
{
  while(!str.empty() && isspace(str.front()))
    str.remove_prefix(1);
}

} // namespace

//_____________________________________________________________________________
namespace Decoder {
// default crate number for trigger supervisor
const UInt_t THaCrateMap::DEFAULT_TSROC = 21;

//_____________________________________________________________________________
THaCrateMap::THaCrateMap( const char* db_filename )
  : fInitTime{0}
  , fTSROC{DEFAULT_TSROC}
  , fTSROCSet{false}
  , fIsInit{false}
  , fDoReset{false}
  , fDebug(0)
  , fCrateDat(kInitialMapSize)
{
  // Construct uninitialized crate map. The argument is the name of
  // the database file to use for initialization
  SetDBfileName(db_filename);
}

//_____________________________________________________________________________
void THaCrateMap::SetDBfileName( const char* db_filename )
{
  if( !db_filename || !*db_filename ) {
    Warning( "THaCrateMap::SetDBfileName", "Undefined database file name, "
             "using default \"db_cratemap.dat\"" );
    db_filename = "cratemap";
  }
  fDBfileName = db_filename;
}

//_____________________________________________________________________________
void THaCrateMap::Clear()
{
  SoftReset();   // Clear fCrateDat and fUsedCrates
  fCrateDat.reserve(kInitialMapSize);
  fTSROC = DEFAULT_TSROC;  // default value, if not found in db_cratemap
  fTSROCSet = false;
  fModuleCfg.clear();
  fIsInit = false;
  fDoReset = false;
}

//_____________________________________________________________________________
UInt_t THaCrateMap::GetSize() const
{
  // Convenience function to return total number of used slots. Sums the sizes
  // of all used_slots arrays of all used crates.
  UInt_t sum = 0;
  for( const auto& cr: fCrateDat | views::values ) {
    sum += cr.sltdat.size();
  }
  return sum;
}

//_____________________________________________________________________________
UInt_t THaCrateMap::getScalerCrate( UInt_t word ) const
{
  // Return number of the crate with a scaler module in slot 1 whose header
  // matches the topmost 12 bits of word. If not found, or if word has any
  // bits 8-15 set, return 0. If multiple crates match, it is undefined which
  // one will be picked.
  if( (word & 0xff00) == 0 ) {
    UInt_t headtry = word & 0xfff00000;
    for( const auto& [crate, crdat]: fCrateDat ) {
      if( auto slt1 = crdat.sltdat.find(1);
            slt1 != crdat.sltdat.end()
            && slt1->second.type == EModuleType::kScaler
            && headtry == slt1->second.header )
        return crate;
    }
  }
  return 0;
}

//_____________________________________________________________________________
std::string THaCrateMap::getConfigStr( UInt_t crate, UInt_t slot ) const
{
  // Get the optional configuration string for the module in crate/slot.
  // If a global default is defined for the module type, use it.

  string cfgstr;
  if( const auto& slt = FindSlot(crate,slot); slt.model != 0 ) {
    string_view defstr;
    if( auto mcfg = fModuleCfg.find(slt.model); mcfg != fModuleCfg.end() )
      defstr = mcfg->second;
    if( string_view str = slt.cfgstr; !str.empty() ) {
      if( str[0] == '+' ) {
        // If there is a '+', prepend default string
        if( !defstr.empty() ) {
          str.remove_prefix(1); // wipe the leading '+'
          trim_left(str);       // not just OCD, but handles str == "+"
          cfgstr = defstr;
          if( !str.empty() )
            cfgstr.append(" ").append(str);
        } else
          cfgstr = str.substr(1);
      } else
        // Have cfgstr without '+' -> ignore default string
        cfgstr = str;
    } else {
      // No slot-specific string -> use default
      cfgstr = defstr;
    }
  }
  return cfgstr;
}

//_____________________________________________________________________________
int THaCrateMap::setCrateType( UInt_t crate, const char* type )
{
  if( auto it = fCrateDat.find(crate); it != fCrateDat.end() )
    it->second = {};
  auto& cr = fCrateDat[crate];
  cr.crate = crate;
  if( type == "fastbus"sv )
    cr.crate_code = kFastbus;
  else if( type == "vme"sv )
    cr.crate_code = kVME;
  else if( type == "scaler"sv )
    cr.crate_code = kScalerCrate;
  else if( type == "camac"sv )
    cr.crate_code = kCamac;
  else {
    setUnused(crate);
    return type == "unused"sv ? CM_OK : CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
void THaCrateMap::setUsed( UInt_t crate, UInt_t slot )
{
  auto& cr = MakeCrate(crate);
  auto& slt = cr.sltdat[slot];  // create slot entry if necessary
  slt.slot = slot;
  cr.used_slots.insert(slot);
}

//_____________________________________________________________________________
void THaCrateMap::setUnused( UInt_t crate )
{
  fCrateDat.erase(crate);
  fUsedCrates.erase(crate);
}

//_____________________________________________________________________________
void THaCrateMap::setUnused( UInt_t crate, UInt_t slot )
{
  auto elem = fCrateDat.find(crate);
  if( elem == fCrateDat.end() )
    return;
  auto& cr = elem->second;
  cr.used_slots.erase(slot);
  cr.sltdat.erase(slot);
  if( cr.used_slots.empty() )
    setUnused(crate);
}

//_____________________________________________________________________________
int THaCrateMap::init( FILE* fi, const char* fname )
{
  // Get the crate map definition string from the given database file,
  // which must be opened and closed by the caller.
  // The file is read completely into an internal string, which is then
  // parsed by init(const std::string&).

  const char* const here = "THaCrateMap::init(file)";

  if( !fname || !*fname )
    fname = "(unspecified)";
  fDBfileName = fname;

  if ( !fi ) {
    ::Error( here, "Error opening crate map database file %s: %s",
        fname, StrError().c_str() );
    return CM_ERR;
  }
  // Build the string to parse
  string db;
  if( ReadFile(fi, db) != 0 || ferror(fi) ) {
    ::Error( here, "Error reading crate map database file %s: %s",
        fname, StrError().c_str() );
    return CM_ERR;
  }

  // Parse the crate map definition
  return init(db);
}

//_____________________________________________________________________________
int THaCrateMap::init( Long64_t tloc )
{
  // Initialize the crate map from the database.
  // 'tloc' is the time-stamp/index into the database's periods of validity.

  const char* const here = "THaCrateMap::init(tloc)";
  int status;
  FILE* fi = nullptr;
  fInitTime = tloc;
  gNeedTZCorrection = (GetTZOffsetToLocal(tloc) != 0);
  WithDefaultTZ(TDatime date = tloc);
  try {
    fi = OpenDBFile(GetName(), date, here, "r", 1);
    status = init(fi, GetName());
  } catch ( const exception& e ) {
    Error( here, "%s", e.what() );
    status = CM_ERR;
  }
  (void) fclose(fi);
  return status;
}

//_____________________________________________________________________________
void THaCrateMap::print_header( ostream& os, const array<int,8>& widths,
                                UInt_t roc, const CrateInfo_t& cr )
{
  // Print header for crate 'cr'

  const auto [spc, w_slot, w_model, w_clear, w_bank,
    w_header, w_mask, w_nchan] = widths;

  os << "==== Crate " << roc << " type " << GetCrateTypeName(cr.crate_code);
  if( !cr.scalerloc.empty() )  os << " \"" << cr.scalerloc << "\"";
  os << endl;
  os << "#"
       << setw(w_slot+spc)   << "slot"
       << setw(w_model+spc)  << "model";
  if( cr.all_banks ) {
    if( cr.HasConfig() )
      os << setw(w_bank+spc);
    os                          << "bank";
  } else if( !cr.has_banks )
    os << setw(w_clear+spc)  << "clear";
  else {
    int w_bkcl = std::max(w_bank, w_clear);
    os << setw(w_bkcl+spc)   << "bk/cl";
  }
  if( !cr.all_banks) {
    os << setw(w_header+spc) << "header"
       << setw(w_mask+spc)   << "mask";
    if( cr.HasConfig() )
      os << setw(w_nchan+spc);
    os                          << "nchan";
  }
  if( cr.HasConfig() )
    os   << "cfgstr";
  os << endl;
}

//_____________________________________________________________________________
void THaCrateMap::print_slot( ostream& os, const array<int,8>& widths,
  const CrateInfo_t& cr, const SlotInfo_t& slt )
{
  // Print info for slot 'slt' in one line

  const auto [spc, w_slot, w_model, w_clear, w_bank,
    w_header, w_mask, w_nchan] = widths;

  bool have_cfg = !slt.cfgstr.empty();
  os << " "
     << setw(w_slot+spc)     << slt.slot
     << setw(w_model+spc)    << slt.model;
  if( slt.bank >= 0 ) {
    if( have_cfg )
      os << setw(w_bank+spc);
    os                          << slt.bank;
    if( !cr.all_banks && have_cfg )
      os << setw(w_header+w_mask+w_nchan+6+w_clear-w_bank) << " ";
  } else {
    os << setw(w_clear+spc)  << slt.do_clear
       << setfill('0')
       // ios::setfill does not work right with "left" alignment; it fill at
       // the right (trailing zeros). With "right" alignment, ios::showbase
       // does not work correctly together with "showbase"; it fill to the
       // left of the "0x" base indicator. So do it by hand, while we wait
       // for std::format finally to appear.
       << "0x" << right << hex << setw(w_header-2) << slt.header   << "  "
       << "0x" << right << hex << setw(w_mask-2)   << slt.headmask << "  "
       << setfill(' ') << dec << left;
    if( have_cfg )
      os << setw(w_nchan+spc);
    os << slt.nchan;
  }
  if( have_cfg )
    os << "cfg:" << slt.cfgstr;
  os << endl;
}

//_____________________________________________________________________________
void THaCrateMap::print(ostream& os) const
{
  // Pretty-print crate map. The output format is suitable for init().

  // Print global configuration parameters
  if( fTSROCSet )
    os << "TSROC " << fTSROC << endl;
  int w_config_model = 1;
  for( auto model: fModuleCfg | views::keys )
    w_config_model = std::max(w_config_model, ToInt(IntDigits(model)));
  for( const auto& [ model, defcfg ]: fModuleCfg ) {
    if( model != 0 && !defcfg.empty() )
      os << "config " << left << dec << setw(w_config_model+1) << model
         << "cfg:" << defcfg << endl;
  }
  // Slot parameter field widths. These are all longer than the print width of
  // the numbers in the columns underneath (except for header & mask), so they
  // can be const. Type int is what setw() wants.
  static const array widths = { 2, // 2 = number of spaces between fields
    ToInt(strlen("slot")),  ToInt(strlen("model")), ToInt(strlen("clear")),
    ToInt(strlen("bank")),  10 /* "header" */,      10 /* "mask" */,
    ToInt(strlen("nchan"))
  };
  // Print the map
  ios::fmtflags oldf = os.setf(ios::left, ios::adjustfield);
  for( auto roc: fUsedCrates ) {
    const auto& iroc = fCrateDat.find(roc);
    assert( iroc != fCrateDat.end() );
    const auto& cr = iroc->second;
    print_header(os, widths, roc, cr );
    for( auto slot: cr.used_slots ) {
      const auto& jt = cr.sltdat.find(slot);
      assert( jt != cr.sltdat.end() );
      print_slot(os, widths, cr, jt->second);
    }
  }
  os.flags(oldf);
}

//_____________________________________________________________________________
THaCrateMap::CrateInfo_t& THaCrateMap::MakeCrate( UInt_t crate )
{
  // Find, and if necessary create, an entry for the given crate number

  auto& cr = fCrateDat[crate];
  cr.crate = crate;
  fUsedCrates.insert(crate);
  return cr;
}

//_____________________________________________________________________________
Int_t THaCrateMap::loadConfig( string& line, string& cfgstr ) const
{
  // Check if module configuration option is specified in 'line' and,
  // if so, extract it and erase the option string from 'line'.
  // Configuration options start either with "cfg:" followed by the string
  // or "dbfile:" followed by a database file name.
  const char* const here = "THaCrateMap::loadConfig";
  auto pos1 = line.find("cfg:");
  auto pos2 = line.find("dbfile:");
  bool have_cfg = (pos1 != string::npos);
  bool have_dbf = (pos2 != string::npos);
  cfgstr.clear();
  if( have_cfg or have_dbf ) {
    if( have_cfg and have_dbf ) {
      ::Error(here, "Cannot specify both database file and options, "
                    "line = \n%s", line.c_str());
      return CM_ERR;
    }
    if( have_cfg ) {
      cfgstr = line.substr(pos1 + 4);
      line.erase(pos1);
    } else {
      string fname = line.substr(pos2 + 7);
      Trim(fname);
      errno = 0;
      WithDefaultTZ(TDatime date = fInitTime);
      FILE* fi = OpenDBFile(fname.c_str(), date, here);
      if ( !fi ) {
        ::Error(here, "Error opening decoder module database file "
                      "\"db_%s.dat\": %s\n line = %s",
                fname.c_str(), StrError().c_str(), line.c_str() );
        return CM_ERR;
      }
      if( ReadFile(fi, cfgstr) != 0 ) {
        ::Error(here, "Error reading decoder module database file "
                      "\"db_%s.dat\": %s", fname.c_str(), StrError().c_str());
        (void)fclose(fi);
        return CM_ERR;
      }
      (void)fclose(fi);
      line.erase(pos2);
    }
    Trim(cfgstr);
  }
  return CM_OK;
}

//_____________________________________________________________________________
Int_t THaCrateMap::ParseCrateInfo( const std::string& line,
                                   size_t pos, UInt_t& crate )
{
  static const char* const here = "THaCrateMap::ParseCrateInfo";

  const char* cstr = line.c_str();
  char ctype[21];
  if( sscanf(cstr + pos, "Crate %u type %20s", &crate, ctype) == 2 ) {
    if( setCrateType(crate, ctype) != CM_OK ) {
      cerr << "THaCrateMap:: Unknown crate type \"" << ctype << "\"" << endl;
      return CM_ERR;
    }
    if( !crateUsed(crate) )  // type = "unused"
      crate = kMaxUInt;
    if( ctype != "scaler"sv )
      return CM_OK;
    auto& cr = fCrateDat.at(crate);
    // for a scaler crate, get the 'name' or location as well
    if( cr.crate_code == kScalerCrate ) {
      if( sscanf(cstr, "Crate %*u type %*s %20s", ctype) != 1 ) {
        cerr << "THaCrateMap:: failed to scan scaler name" << endl;
        return CM_ERR;
      }
      string scaler_name{ctype};
      std::erase(scaler_name, '\"'); // drop extra quotes
      cr.scalerloc = std::move(scaler_name);
    }
    return CM_OK;
  }
  Error(here, "Invalid/incomplete Crate definition: \"%s\"", cstr);
  return CM_ERR;
}

//_____________________________________________________________________________
Int_t THaCrateMap::ParseHeader( const string& line, int lineno, bool& do_continue )
{
  const char* const here = "THaCrateMap::ParseHeader";

  constexpr array headers =
    {"TSROC "sv, "config "sv, "reset"sv, "always_reset"sv};
  const auto [TSROC_KEY, CONFIG_KEY, RESET_KEY,
    ALWAYS_RESET_KEY] = headers;

  // Parse optional TSROC definition at start of map (or right after a timestamp)
  if( line.starts_with(TSROC_KEY) ) {
    size_t pos;
    unsigned long val = DEFAULT_TSROC;
    bool err = false; try {
      val = stoul(line.substr(TSROC_KEY.length()), &pos);
    } catch( ... ) { err = true; }
    if( !err && pos + TSROC_KEY.length() == line.length() ) {
      fTSROC = val;
      fTSROCSet = true;
      do_continue = true;
      return CM_OK;
    }
    Error(here, "db_%s.dat:%d: Failed to parse line \"%s\". "
          "Expected \"TSROC <ROC number>\".",
          GetName(), lineno, line.c_str());
    return CM_ERR;
  }
  // Load optional configuration string defaults
  if( line.starts_with(CONFIG_KEY) ) {
    int model = 0;
    size_t pos;
    bool err = false; try {
      model = stoi(line.substr(CONFIG_KEY.length()), &pos);
    } catch( ... ) { err = true; }
    if( !err && pos > 0 && model != 0 ) {
      if( string cfgstr, ln = line; loadConfig(ln, cfgstr) == CM_OK ) {
        if( cfgstr.empty() )
          fModuleCfg.erase(model); // no config string -> clear the entry
        else
          fModuleCfg[model] = cfgstr;
        do_continue = true;
        return CM_OK;
      }
    }
    Error(here, "db_%s.dat:%d: Failed to parse line \"%s\". "
          "Expected \"config <model number != 0> cfg:<parameters>\". ",
          GetName(), lineno, line.c_str());
    return CM_ERR;
  }
  // Clear out crate map, except header directives
  if( line == RESET_KEY ) {
    SoftReset();
    do_continue = true;
    return CM_OK;
  }
  if( line == ALWAYS_RESET_KEY ) {
    fDoReset = true;
    do_continue = true;
  }
  return CM_OK;
}

//_____________________________________________________________________________
Int_t THaCrateMap::ParseCrateHeader( const std::string& line, int lineno,
  UInt_t& crate, bool& in_crate, bool& do_continue )
{
  const char* const here = "THaCrateMap::ParseCrateHeader";

  if( auto pos = line.find("Crate"); pos != string::npos ) {
    // Bugcheck: When starting a new crate, ensure that the previous crate
    // (if any) either has defined slots or is not in fUsedCrates
    assert(crate == kMaxUInt || getNslot(crate) > 0 || !fUsedCrates.contains(crate));
    // Set the next CRATE number and type
    if( ParseCrateInfo(line, pos, crate) != CM_OK )
      return CM_ERR;
    do_continue = in_crate = true;
    return CM_OK;
  }
  if( crate == kMaxUInt || !in_crate ) {
    const char* msg = in_crate ? "." : " or preamble keyword.";
    Error(here, "db_%s.dat:%d: Expected Crate definition%s\n"
          "For example: \"==== Crate 5 type vme\". Found instead: \"%s\"",
          GetName(), lineno, msg, line.c_str());
    return CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
void THaCrateMap::PostInit()
{
  // Post-init processing:
  // - Remove crates without slots
  // - Set bank status flags for each crate

  for( auto it = fCrateDat.begin(); it != fCrateDat.end(); ) {
    auto& cr = it->second;
    if( cr.sltdat.empty() ) {
      UInt_t crate = it->first;
      assert(!fUsedCrates.contains(crate));
      it = fCrateDat.erase(it);
    } else {
      cr.SetBankInfo();
      ++it;
    }
  }
}

//_____________________________________________________________________________
void THaCrateMap::SoftReset()
{
  // Reset only crate data, not any other flags in the map
  fCrateDat.clear();
  fUsedCrates.clear();
}

//_____________________________________________________________________________
Int_t THaCrateMap::CrateInfo_t::ParseSlotInfo( const THaCrateMap* crmap,
                                               string& line )
{
  // 'line' is expected to be the definition for a single module (slot).
  // Parse it and fill corresponding element of the sltdat array.

  const char* here = "THaCrateMap::ParseSlotInfo";

  // Check for and extract any configuration string given on the line and,
  // if found, remove it from line and put a copy in cfgstr.
  string cfgstr;
  if( Int_t st = crmap->loadConfig(line, cfgstr); st != CM_OK )
    return st;

  // Default values
  Int_t  cword = 1, imodel = 0;
  UInt_t slot = kMaxUInt;
  UInt_t mask = 0, iheader = 0;
  UShort_t ichan = MAXCHAN;

  // The line is of the format
  //        slot  model  [clear header mask nchan ndata ]
  // where clear, header, mask, nchan and ndata are optional.
  //
  // Another option is "bank decoding":  all data in this CODA bank
  // belongs to this slot and model.  The line has the format
  //        slot  model  bank
  Int_t nread = sscanf(line.c_str(), "%u %d %d %x %x %hu",
                       &slot, &imodel, &cword, &iheader, &mask, &ichan);
  // must have at least the slot and model numbers and one additional word
  if( nread < 3 || imodel == 0 ) {
    Error(here, "Bad slot definition: %s\nNeeds >= 3 words and model != 0. "
          "Slot will not be decoded.", line.c_str());
    return CM_ERR;
  }

  // Find the given model in the module registry
  const auto& modtypes = Module::fgModuleTypes();
  auto imodule = modtypes.find(imodel);
  Module::ModuleType module;
  if( imodule == modtypes.end() ) {
#ifdef PODD_ENABLE_TESTS
    if( crmap->fDebug & kAllowMissingModel ) {
      // For testing only: get basic module info (but no class) from lookup table
      module = Module::GetModuleType(imodel);
      if( module.fModel == 0 ) {
#endif
        Error(here, "No decoder module with ID = %d defined. "
              "Slot %u in crate %u will not be decoded.", imodel, slot, crate);
        return CM_ERR;
      }
#ifdef PODD_ENABLE_TESTS
    }
  } else
#endif
    module = *imodule;
  if( module.fBus != crate_code ) {
    Error(here, "Slot %u crate %u: Type \"%s\" of module %d "
          "conflicts with crate type \"%s\".", slot, crate,
          GetCrateTypeName(module.fBus), imodel, GetCrateTypeName(crate_code));
    return CM_ERR;
  }
  // Create or retrieve slot info
  if( auto islot = sltdat.find(slot); islot != sltdat.end() ) {
    Warning(here, "Duplicate slot definition: %s", line.c_str());
    islot->second = {};  // clear previous definition
  }
  SlotInfo_t& slt = sltdat[slot];

  // Fill slot info with the data we just read
  slt.slot = slot;
  slt.model = imodel;
  slt.type = module.fType;
  if( nread == 3 ) {
    // bank decoding, default with CODA 3
    slt.bank = cword;
  } else {  // nread > 3
    // legacy slot or non-bank data
    slt.do_clear = cword;
    slt.header = iheader;
    if( nread > 4 )
      slt.headmask = mask;
  }
  if( nread > 5 ) {
    if( ichan != module.fNchan ) {
      Warning(here, "Slot %u crate %u: Module %d defines %u channels, "
              "but crate map specifies %hu. Using crate map value.",
              slot, crate, imodel, module.fNchan, ichan);
    }
    slt.nchan = ichan;
  } else {
    slt.nchan = module.fNchan;
  }
  slt.cfgstr = std::move(cfgstr);  // global config string applied in getConfigStr

  used_slots.insert(slot);
  return CM_OK;
}

//_____________________________________________________________________________
void THaCrateMap::CrateInfo_t::SetBankInfo()
{
  auto slot_is_bank = []( const auto& slt ) { return slt.second.bank >= 0; };
  has_banks = ranges::any_of(sltdat, slot_is_bank);
  all_banks = ranges::all_of(sltdat, slot_is_bank);
}

//_____________________________________________________________________________
// Do any slots of this crate have a config string?
bool THaCrateMap::CrateInfo_t::HasConfig() const
{
  auto slot_has_cfg = []( const auto& slt ) { return !slt.cfgstr.empty(); };
  return ranges::any_of(sltdat | views::values, slot_has_cfg);
}

//_____________________________________________________________________________
int THaCrateMap::init(const string& the_map)
{
  // Initialize the crate map according to the lines in the string 'the_map'

  const char* const here = "THaCrateMap::init(string)";

  Clear();

  if ( the_map.empty() ) {
    // Warn if we didn't get any data
    ::Error( here, "Empty crate map definition. Decoder will not be usable. "
        "Check database." );
    return CM_ERR;
  }

  UInt_t crate = kMaxUInt; // current CRATE
  string line; line.reserve(128);
  istringstream s(the_map);
  Long64_t keydate = 0, prevdate = 0;
  bool do_ignore = false, in_crate = false;
  int lineno = 0;

  while( getline(s, line) ) {
    ++lineno;
    // Drop comments. For historical reasons, they may start with '!' or '#'
    if( auto pos = line.find_first_of("!#"); pos != string::npos )
      line.erase(pos);
    Trim(line);
    if( line.empty() )
      continue; // skip empty or comment-only lines

    // Check for timestamps. Employ the algorithm from Podd::LoadDBvalue, i.e.
    // read data for timestamps <= init time and newer than prior timestamps.
    // This may lead to certain crates being defined multiple times as its
    // parameters are updated for a new validity period.
    // To remove a previously-defined crate after a certain time stamp, use
    // crate type "unused" or, if many crates need to be removed, "reset".
    if( IsDBtimestamp(line, keydate) ) {
      do_ignore = fInitTime > 0 && (keydate > fInitTime || keydate < prevdate);
      in_crate = false;
      if( !do_ignore ) {
        prevdate = keydate;
        if( fDoReset )
          SoftReset();
      }
      continue;
    }
    if( do_ignore )
      continue;

    bool do_continue = false;
    if( !in_crate ) {
      if( ParseHeader(line, lineno, do_continue) != CM_OK )
        return CM_ERR;  // Error already printed
      if( do_continue )
        continue;       // Header line found -> next line
    }

    // Check for "=== Crate <num> type <type>"
    if( ParseCrateHeader(line, lineno, crate, in_crate, do_continue) != CM_OK )
      return CM_ERR;    // Error already printed
    if( do_continue )
      continue;         // New crate header found -> next line
    assert(fCrateDat.contains(crate));

    // Interpret lines following "=== Crate" as slot definitions
    if( fCrateDat[crate].ParseSlotInfo(this, line) != CM_OK )
      return CM_ERR;

    // At least one slot successfully defined
    fUsedCrates.insert(crate);

  } // while getline

  // Remove empty crates and set crate status flags
  PostInit();

  if( fUsedCrates.empty() ) {
    Error(here, "db_%s.dat: No valid crate map entries loaded. "
          "Decoder will not be usable. Check database.", GetName() );
    return CM_ERR;
  }

  if ( !fTSROCSet ) {
    // Set fTSROC to > MAXROC to be able to detect non-presence of a definition
    // in the crate map later. In this way, we can avoid printing a warning
    // if we're analyzing CODA 2 data, for which the TSROC doesn't matter.
    //FIXME this is clunky - improve
    fTSROC += 100;
  }
  fIsInit = true;
  return CM_OK;
}

} // namespace Decoder

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Decoder::THaCrateMap)
#endif
