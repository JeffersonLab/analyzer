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

//_____________________________________________________________________________
static string StrError()
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
namespace Decoder {
// default crate number for trigger supervisor
const UInt_t THaCrateMap::DEFAULT_TSROC = 21;

//_____________________________________________________________________________
THaCrateMap::THaCrateMap( const char* db_filename )
  : fInitTime{0}
  , fTSROC{DEFAULT_TSROC}
  , fIsInit{false}
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
void THaCrateMap::clear()
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
  for( const auto& elem: fCrateDat ) {
    sum += elem.second.sltdat.size();
  }
  return sum;
}

//_____________________________________________________________________________
UInt_t THaCrateMap::getScalerCrate( UInt_t word) const
{
  for( const auto& elem: fCrateDat ) {
    const auto& cr = elem.second;
    if( cr.isScalerCrate() ) {
      UInt_t headtry = word & 0xfff00000;
      UInt_t zero = word & 0x0000ff00;
      //FIXME
      // if( (zero == 0) &&
      //     (headtry == crdat.sltdat[1].header) )
        return cr.crate;
    }
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaCrateMap::resetCrate( UInt_t crate )
{
  setUnused(crate);
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setCrateType( UInt_t crate, const char* type )
{
  string stype(type);
  auto& cr = fCrateDat[crate];
  cr.crate_type_name = stype;
  if( stype == "fastbus" )
    cr.crate_code = kFastbus;
  else if( stype == "vme" )
    cr.crate_code = kVME;
  else if( stype == "scaler" )
    cr.crate_code = kScaler;
  else if( stype == "camac" )
    cr.crate_code = kCamac;
  else {
    resetCrate(crate);
    return stype == "unused" ? CM_OK : CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setModel( UInt_t crate, UInt_t slot, Int_t mod,
                           UInt_t nchan, UInt_t ndata )
{
  setUsed(crate, slot);
  auto it = fCrateDat.find(crate);
  assert( it != fCrateDat.end() );
  auto jt = it->second.sltdat.find(slot);
  assert( jt != it->second.sltdat.end() );
  auto& slt = jt->second;
  if( slt.model != 0 && mod == 0 ) {
    setUnused(crate, slot);
  }
  slt.model = mod;
  if( !SetModelSize( crate, slot, mod )) {
    slt.nchan = nchan;
    slt.ndata = ndata;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::SetModelSize( UInt_t crate, UInt_t slot, UInt_t model )
{
  // Set the max number of channels and data words for some known modules
  struct ModelPar_t { UInt_t model, nchan, ndata; };
  static const array<ModelPar_t, 18> modelpar = {{
    { 1875, 64, 512 },  // Detector TDC
    { 1877, 96, 672 },  // Wire-chamber TDC
    { 1881, 64, 64 },   // Detector ADC
    { 550, 512, 1024 }, // CAEN 550 (RICH)
    { 560,  16, 16 },   // CAEN 560 scaler
    { 1182, 8, 128 },   // LeCroy 1182 ADC (?)
    { 1151, 16, 16 },   // LeCroy 1151 scaler
    { 3123, 16, 16 },   // VMIC 3123 ADC
    { 3800, 32, 32 },   // Struck 3800 scaler
    { 3801, 32, 32 },   // Struck 3801 scaler
    { 7510, 8, 1024 },   // Struck 7510 ADC (multihit)
    { 767, 128, 1024 },  // CAEN 767 multihit TDC
    { 6401, 64, 1024 },  // JLab F1 TDC normal resolution
    { 3201, 32, 512 },   // JLab F1 TDC high resolution
    { 775, 32, 32 },     // CAEN V775 TDC
    { 792, 32, 32 },     // CAEN V792 QDC
    { 1190, 128, 1024 }, //CAEN 1190A
    { 250, 16, 20000 },  // FADC 250
  }};
  const auto* item =
    ranges::find_if(modelpar, [model]( const ModelPar_t& modelParam ) {
      return model == modelParam.model;
    });
  if( item != modelpar.end() ) {
    fCrateDat[crate].sltdat[slot].nchan = item->nchan;
    fCrateDat[crate].sltdat[slot].ndata = item->ndata;
    return 1;
  }
  return 0;
}

//_____________________________________________________________________________
void THaCrateMap::setUsed( UInt_t crate, UInt_t slot )
{
  auto& cr = FindCrate(crate);
  auto& slt = cr.sltdat[slot];
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
  const auto& elem = fCrateDat.find(crate);
  if( elem == fCrateDat.end() )
    return;
  auto& cr = elem->second;
  cr.used_slots.erase(slot);
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
  WithDefaultTZ(TDatime date = tloc);
  try {
    fi = Podd::OpenDBFile(GetName(), date, here, "r", 1);
    status = init(fi, GetName());
  } catch ( ... ) {
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
    os << "==== Crate " << roc << " type " << cr.crate_type_name;
    if( !cr.scalerloc.empty() )  os << " \"" << cr.scalerloc << "\"";
    os << endl;
    os << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( auto slot: cr.used_slots ) {
      const auto& jt = cr.sltdat.find(slot);
      assert( jt != cr.sltdat.end() );
      const auto& slt = jt->second;
      os << "  " << slot << "\t" << slt.model << "\t" << slt.clear;
      ios::fmtflags oldf = os.setf(ios::right, ios::adjustfield);
      os << "\t0x" << hex << setfill('0') << setw(8) << slt.header
	   << "\t0x" << hex << setfill('0') << setw(8) << slt.headmask
	   << dec << setfill(' ') << setw(0)
	   << "\t" << slt.nchan
	   << "\t" << slt.ndata
	   << endl;
      os.flags(oldf);
    }
  }
}

//_____________________________________________________________________________
THaCrateMap::CrateInfo_t& THaCrateMap::FindCrate( UInt_t crate )
{
  // Find, and if necessary create, an entry for the given crate number

  fUsedCrates.insert(crate);
  auto& cr = fCrateDat[crate];
  assert(cr.crate == crate || cr.crate == 0);
  cr.crate = crate;
  return cr;
}

//_____________________________________________________________________________
Int_t THaCrateMap::loadConfig( string& line, string& cfgstr ) const
{
  // Check if module configuration option is specified in 'line' and,
  // if so, extract it and erase the option string from 'line'.
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
    // for a scaler crate, get the 'name' or location as well
    auto& cr = fCrateDat[crate];
    if( cr.crate_code == kScaler ) {
      if( sscanf(line.c_str(), "Crate %*u type %*s %20s", ctype) != 1 ) {
        cerr << "THaCrateMap:: failed to scan scaler name" << endl;
        return CM_ERR;
      }
      TString scaler_name(ctype);
      scaler_name.ReplaceAll("\"", ""); // drop extra quotes
      cr.scalerloc = scaler_name;
    }
    return CM_OK;
    //in_crate = cr.crate_used;  // false for ctype == "unused"
    //continue; // onto the next line
  }
  Error(here, "Invalid/incomplete Crate definition: \"%s\"",
        line.c_str());
  return CM_ERR;
}

//_____________________________________________________________________________
Int_t THaCrateMap::CrateInfo_t::ParseSlotInfo( THaCrateMap* crmap, UInt_t crate,
                                               string& line )
{
  // 'line' is expected to be the definition for a single module (slot).
  // Parse it and fill corresponding element of the sltdat array.

  // Check for and extract any configuration string given on the line and,
  // if found, remove it from line and put a copy in cfgstr.
  string cfgstr;
  if( Int_t st = crmap->loadConfig(line, cfgstr); st != CM_OK )
    return st;

  // The line is of the format
  //        slot  model  [clear header mask nchan ndata ]
  // where clear, header, mask, nchan and ndata are optional.
  //
  // Another option is "bank decoding":  all data in this CODA bank
  // belongs to this slot and model.  The line has the format
  //        slot  model  bank

  // Default values
  Int_t  cword = 1, imodel = 0;
  UInt_t slot = kMaxUInt;
  UInt_t mask = 0, iheader = 0, ichan = MAXCHAN, idata = MAXDATA;

  Int_t nread = sscanf(line.c_str(), "%u %d %d %x %x %u %u",
                       &slot, &imodel, &cword, &iheader, &mask, &ichan, &idata);
  // must have at least the slot and model numbers
  if( nread < 2 ) {
    cerr << "THaCrateMap:: fatal ERROR 4   " << endl << "Bad line " << endl
         << line << endl
         << "    Warning: a bad line could cause wrong decoding !" << endl;
    return CM_ERR;
  }

  if (nread > 5 )
    crmap->setModel(crate,slot,imodel,ichan,idata);
  else
    crmap->setModel(crate,slot,imodel);

  SlotInfo_t& slt = sltdat[slot];
  if( nread == 3 )
    slt.bank = cword;
  else if( nread > 3 ) {
    slt.clear = cword;
    slt.header = iheader;
    if( nread > 4 )
      slt.headmask = mask;
  }
  // If a slot-specific configuration string starts with a '+', append it
  // to the global configuration for this model, if any.
  // In any case, erase the '+'.
  string default_cfgstr;
  if( imodel != 0 ) {
    if( auto mcfg = crmap->fModuleCfg.find(imodel);
          mcfg != crmap->fModuleCfg.end() )
      default_cfgstr = mcfg->second;
  }
  if( !cfgstr.empty() ) {
    if( cfgstr[0] == '+' ) {
      // If there is a '+', prepend default string
      if( !default_cfgstr.empty() ) {
        cfgstr[0] = ' '; // wipe the leading '+'
        Podd::Trim(cfgstr); // OCD moment
        slt.cfgstr = default_cfgstr + ' ' + cfgstr;
      } else
        slt.cfgstr = cfgstr.substr(1);
    } else
      // Have cfgstr without '+' -> ignore default string
      slt.cfgstr = std::move(cfgstr);
  } else
    // No slot-specific string -> use default
    slt.cfgstr = std::move(default_cfgstr);

  return CM_OK;
}

//_____________________________________________________________________________
Int_t THaCrateMap::SetBankInfo()
{
  for( auto& cr: fCrateDat | views::values ) {
    auto slot_is_bank = []( const auto& slt ) { return slt.second.bank >= 0; };
    cr.bank_structure = ranges::any_of(cr.sltdat, slot_is_bank);
    cr.all_banks = ranges::all_of(cr.sltdat, slot_is_bank);
  }
  return 0;
}

//_____________________________________________________________________________
int THaCrateMap::init(const string& the_map)
{
  // Initialize the crate map according to the lines in the string 'the_map'

  const char* const here = "THaCrateMap::init(string)";

  clear();

  if ( the_map.empty() ) {
    // Warn if we didn't get any data
    ::Warning( here, "Empty crate map definition. Decoder will not be usable. "
        "Check database." );
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
      // Make the line "==== Crate" not care about how many "=" chars or other
      // chars before "Crate", except that lines beginning with '#' or '!' are
      // still a comment (filtered out above).
      line.erase(0, pos);
      // Set the next CRATE number and type
      if( ParseCrateInfo(line, crate) != CM_OK )
        return CM_ERR;
      in_crate = fCrateDat.contains(crate);  // false for crate type "unused"
      continue; // onto the next line
    }
    if( crate == kMaxUInt || !in_crate ) {
      Error(here, "db_%s.dat:%d: Expected Crate definition.\n"
                  "For example: \"==== Crate 5 type vme\". "
                  "Found instead: \"%s\"",
            GetName(), lineno, line.c_str());
      return CM_ERR;
    }

    if( fCrateDat[crate].ParseSlotInfo(this, crate, line) != CM_OK )
      return CM_ERR;

  } // while getline

  if( fUsedCrates.empty() ) {
    Error(here, "db_%s.dat: No valid crate map entries loaded. "
          "Decoder will not be usable. Check database.", GetName() );
    return CM_ERR;
  }

  SetBankInfo();

  if ( !found_tscrate ) {
    // Set fTSROC to > MAXROC to be able to detect non-presence of a definition
    // in the crate map later. In this way, we can avoid printing a warning
    // if we're analyzing CODA 2 data, for which the TSROC doesn't matter.
    fTSROC += 100;
  }
  fIsInit = true;
  return CM_OK;
}

//_____________________________________________________________________________
Long64_t THaCrateMap::GetInitTime() const
{
  return fInitTime;
}

//_____________________________________________________________________________
THaCrateMap::CrateInfo_t::CrateInfo_t()
  : crate_type_name("unknown")
  , crate(0)
  , crate_code(kUnknown)
  , crate_used(false)
  , bank_structure(false)
  , all_banks(false)
  , sltdat(MAXSLOT)
{}

} // namespace Decoder

//_____________________________________________________________________________
ClassImp(Decoder::THaCrateMap)
