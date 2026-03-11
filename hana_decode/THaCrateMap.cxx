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


#include "Decoder.h"
#include "THaCrateMap.h"
#include "Helper.h"
#include "Database.h"
#include "Textvars.h"
#include "Module.h"
#include "TError.h"
#include "TSystem.h"
#include "TString.h"
#include <cstdio>
#include <cerrno>  // for errno
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <cstring>  // for strerror_r
#include <memory>   // for unique_ptr
#include <algorithm> // for std::find, std::sort
#include <array>
#include <ranges>
#include <utility>
#include <string_view>

static constexpr size_t kInitialMapSize = 64;

// This is a well-known problem with strerror_r
#if defined(__linux__) && (defined(_GNU_SOURCE) || !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE > 600))
# define GNU_STRERROR_R
#endif

using namespace std;
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
    {kUnknown, "(unknown)"}, {kVME, "VME"}, {kFastbus, "Fastbus"},
    {kCamac, "CAMAC"}, {kScalerCrate, "scaler"}
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
  , fIsInit{false}
  , fDebug(0)
  , fCrateDat(kInitialMapSize)
{
  // Construct uninitialized crate map. The argument is the name of
  // the database file to use for initialization

  if( !db_filename || !*db_filename ) {
    ::Warning( "THaCrateMap", "Undefined database file name, "
	       "using default \"db_cratemap.dat\"" );
    db_filename = "cratemap";
  }
  fDBfileName = db_filename;
}

//_____________________________________________________________________________
void THaCrateMap::Clear()
{
  fCrateDat.clear();
  fCrateDat.reserve(kInitialMapSize);
  fUsedCrates.clear();
  fTSROC = DEFAULT_TSROC;  // default value, if not found in db_cratemap
  fIsInit = false;
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
Int_t THaCrateMap::readFile( FILE* fi, string& text )
{
  // Read entire file 'fi' into string 'text'.
  // Returns CM_OK if successful, otherwise CM_ERR, in which case the
  // contents of 'text' are undefined.
  if( !fi )
    return CM_ERR;
  errno = 0;
  if( fseeko(fi, 0L, SEEK_END) != 0 )
    return CM_ERR;
  auto size = ftello(fi);
  if( size <= 0 || Podd::Rewind(fi) != 0 )
    return CM_ERR;
  try {
    text.resize(size);
    auto nread = fread(text.data(), sizeof(char), size, fi);
    if( std::cmp_not_equal(nread ,size) )
      return CM_ERR;
  } catch ( ... ) {
    text.clear();
    return CM_ERR;
  }
  return CM_OK;
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
  if( readFile(fi, db) != CM_OK || ferror(fi) ) {
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
  Podd::gNeedTZCorrection = (Podd::GetTZOffsetToLocal(tloc) != 0);
  WithDefaultTZ(TDatime date = tloc);
  try {
    fi = Podd::OpenDBFile(GetName(), date, here, "r", 1);
    status = init(fi, GetName());
  } catch ( const exception& e ) {
    Error( here, "%s", e.what() );
    status = CM_ERR;
  }
  (void) fclose(fi);
  return status;
}

//_____________________________________________________________________________
void THaCrateMap::print(ostream& os) const
{
  // Pretty-print crate map
  for( auto roc: fUsedCrates ) {
    const auto& it = fCrateDat.find(roc);
    assert( it != fCrateDat.end() );
    const auto& cr = it->second;
    os << "==== Crate " << roc << " type " << GetCrateTypeName(cr.crate_code);
    if( !cr.scalerloc.empty() )  os << " \"" << cr.scalerloc << "\"";
    os << endl;
    os << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\n";
    for( auto slot: cr.used_slots ) {
      const auto& jt = cr.sltdat.find(slot);
      assert( jt != cr.sltdat.end() );
      const auto& slt = jt->second;
      os << "  " << slot << "\t" << slt.model << "\t" << slt.do_clear;
      ios::fmtflags oldf = os.setf(ios::right, ios::adjustfield);
      os << "\t0x" << hex << setfill('0') << setw(8) << slt.header
	   << "\t0x" << hex << setfill('0') << setw(8) << slt.headmask
	   << dec << setfill(' ') << setw(0)
	   << "\t" << slt.nchan
	   << endl;
      os.flags(oldf);
    }
  }
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
      Podd::Trim(fname);
      errno = 0;
      WithDefaultTZ(TDatime date = fInitTime);
      FILE* fi = Podd::OpenDBFile(fname.c_str(), date, here);
      if ( !fi ) {
        ::Error(here, "Error opening decoder module database file "
                      "\"db_%s.dat\": %s\n line = %s",
                fname.c_str(), StrError().c_str(), line.c_str() );
        return CM_ERR;
      }
      if( readFile(fi, cfgstr) != CM_OK ) {
        ::Error(here, "Error reading decoder module database file "
                      "\"db_%s.dat\": %s", fname.c_str(), StrError().c_str());
        (void)fclose(fi);
        return CM_ERR;
      }
      (void)fclose(fi);
      line.erase(pos2);
    }
    Podd::Trim(cfgstr);
  }
  return CM_OK;
}

//_____________________________________________________________________________
Int_t THaCrateMap::ParseCrateInfo( const std::string& line, UInt_t& crate )
{
  static const char* const here = "THaCrateMap::ParseCrateInfo";

  char ctype[21];
  if( sscanf(line.c_str(), "Crate %u type %20s", &crate, ctype) == 2 ) {
    if( setCrateType(crate, ctype) != CM_OK ) {
      cerr << "THaCrateMap:: Unknown crate type \"" << ctype << "\"" << endl;
      return CM_ERR;
    }
    if( ctype != "scaler"sv )
      return CM_OK;
    auto& cr = fCrateDat.at(crate);
    // for a scaler crate, get the 'name' or location as well
    if( cr.crate_code == kScalerCrate ) {
      if( sscanf(line.c_str(), "Crate %*u type %*s %20s", ctype) != 1 ) {
        cerr << "THaCrateMap:: failed to scan scaler name" << endl;
        return CM_ERR;
      }
      TString scaler_name(ctype);
      scaler_name.ReplaceAll("\"", ""); // drop extra quotes
      cr.scalerloc = scaler_name;
    }
    return CM_OK;
  }
  Error(here, "Invalid/incomplete Crate definition: \"%s\"",
        line.c_str());
  return CM_ERR;
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
  bool do_ignore = false, in_crate = false, found_tscrate = false;
  int lineno = 0;

  while( getline(s, line) ) {
    ++lineno;
    // Drop comments. For historical reasons, they may start with '!' or '#'
    if( auto pos = line.find_first_of("!#"); pos != string::npos )
      line.erase(pos);
    Podd::Trim(line);
    if( line.empty() )
      continue; // skip empty or comment-only lines

    // Check for timestamps. Employ the algorithm from Podd::LoadDBvalue, i.e.
    // read data for timestamps <= init time and newer than prior timestamps.
    // This may lead to certain crates being defined multiple times as its
    // parameters are updated for a new validity period.
    // To remove a previously-defined crate after a certain time stamp, use
    // crate type "unused".
    if( Podd::IsDBtimestamp(line, keydate) ) {
      do_ignore = fInitTime > 0 && (keydate > fInitTime || keydate < prevdate);
      in_crate = false;
      continue;
    }
    if( do_ignore )
      continue;
    prevdate = keydate;

    if( !in_crate ) {
      // Parse optional TSROC definition at start of map (or right after a timestamp)
      if( constexpr string_view TSROC_KEY = "TSROC "; line.starts_with(TSROC_KEY) ) {
        size_t pos;
        unsigned long val;
        bool err = false; try {
          val = stoul(line.substr(TSROC_KEY.length()), &pos);
        } catch( ... ) { err = true; }
        if( !err && pos + TSROC_KEY.length() == line.length() ) {
          found_tscrate = true;
          fTSROC = val;
          continue;
        }
        Error(here, "db_%s.dat:%d: Failed to parse line \"%s\". "
              "Expected \"TSROC <ROC number>\".",
              GetName(), lineno, line.c_str());
        return CM_ERR;
      }
      // Load optional configuration string defaults
      if( constexpr string_view CONFIG_KEY = "config "; line.starts_with(CONFIG_KEY) ) {
        int model = 0;
        size_t pos;
        bool err = false; try {
          model = stoi(line.substr(CONFIG_KEY.length()), &pos);
        } catch( ... ) { err = true; }
        if( !err && pos > 0 && model != 0 ) {
          if( string cfgstr; loadConfig(line, cfgstr) == CM_OK ) {
            if( cfgstr.empty() )
              fModuleCfg.erase(model); // no config string -> clear the entry
            else
              fModuleCfg[model] = cfgstr;
            continue;
          }
        }
        Error(here, "db_%s.dat:%d: Failed to parse line \"%s\". "
              "Expected \"config <model number != 0> cfg:<parameters>\". ",
              GetName(), lineno, line.c_str());
        return CM_ERR;
      }
    }

    if( auto pos = line.find("Crate"); pos != string::npos ) {
      // Bugcheck: When starting a new crate, ensure that the previous crate
      // (if any) either has defined slots or is not in fUsedCrates
      assert(crate == kMaxUInt || getNslot(crate) > 0 || !fUsedCrates.contains(crate));
      // Make the line "==== Crate" not care about how many "=" chars or other
      // chars before "Crate", except that lines beginning with '#' or '!' are
      // still a comment (filtered out above).
      line.erase(0, pos);
      // Set the next CRATE number and type
      if( ParseCrateInfo(line, crate) != CM_OK )
        return CM_ERR;
      in_crate = crateUsed(crate);  // false for crate type "unused"
      continue; // onto the next line
    }
    if( crate == kMaxUInt || !in_crate ) {
      Error(here, "db_%s.dat:%d: Expected Crate definition.\n"
                  "For example: \"==== Crate 5 type vme\". "
                  "Found instead: \"%s\"",
            GetName(), lineno, line.c_str());
      return CM_ERR;
    }
    assert(fCrateDat.contains(crate));

    if( fCrateDat[crate].ParseSlotInfo(this, line) != CM_OK )
      return CM_ERR;
    // At least one slot successfully defined
    fUsedCrates.insert(crate);

  } // while getline

  // Post-processing:
  // - Remove crates without slots
  // - Set bank status flags for each crate
  for( auto it = fCrateDat.begin(); it != fCrateDat.end(); ) {
    auto& cr = it->second;
    if( cr.sltdat.empty() ) {
      crate = it->first;
      assert(!fUsedCrates.contains(crate));
      it = fCrateDat.erase(it);
    } else {
      cr.SetBankInfo();
      ++it;
    }
  }

  if( fUsedCrates.empty() ) {
    Error(here, "db_%s.dat: No valid crate map entries loaded. "
          "Decoder will not be usable. Check database.", GetName() );
    return CM_ERR;
  }

  if ( !found_tscrate ) {
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
ClassImp(Decoder::THaCrateMap)
