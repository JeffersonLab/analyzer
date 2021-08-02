/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e whether slots are
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
#include "Database.h"
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

#define ALL(c) (c).begin(), (c).end()

// This is a well-known problem with strerror_r
#if defined(__linux__) && (defined(_GNU_SOURCE) || !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE > 600))
# define GNU_STRERROR_R
#endif

using namespace std;

//_____________________________________________________________________________
static string StrError()
{
  // Return description of current errno as a std::string
  const size_t BUFLEN = 128;
  char buf[BUFLEN];

#ifdef GNU_STRERROR_R
  string ret = strerror_r(errno, buf, BUFLEN);
#else
  strerror_r(errno, buf, BUFLEN);
  string ret = buf;
#endif
  return ret;
}

//_____________________________________________________________________________
namespace Decoder {

const UInt_t THaCrateMap::MAXCHAN = 8192;
const UInt_t THaCrateMap::MAXDATA = 65536;
const Int_t  THaCrateMap::CM_OK = 1;
const Int_t  THaCrateMap::CM_ERR = -1;

//_____________________________________________________________________________
THaCrateMap::THaCrateMap( const char* db_filename )
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
UInt_t THaCrateMap::getScalerCrate( UInt_t word) const
{
  for( UInt_t crate = 0; crate < crdat.size(); crate++ ) {
    const auto& cr = crdat[crate];
    if( !cr.crate_used ) continue;
    if( isScalerCrate(crate) ) {
      UInt_t headtry = word & 0xfff00000;
      UInt_t zero = word & 0x0000ff00;
      if( (zero == 0) &&
          (headtry == cr.sltdat[1].header) )
        return crate;
    }
  }
  return 0;
}

//_____________________________________________________________________________
int THaCrateMap::setCrateType( UInt_t crate, const char* type )
{
  assert(crate < crdat.size());
  string stype(type);
  CrateInfo_t& cr = crdat[crate];
  cr.crate_used = true;
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
    cr.crate_used = false;
    cr.crate_type_name = "unknown";
    cr.crate_code = kUnknown;
    return CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setModel( UInt_t crate, UInt_t slot, Int_t mod,
                           UInt_t nchan, UInt_t ndata )
{
  setUsed(crate, slot);
  auto& slt = crdat[crate].sltdat[slot];
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
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  struct ModelPar_t { UInt_t model, nchan, ndata; };
  static const array<ModelPar_t, 19> modelpar = {{
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
//    { 526, 128, 1024 },  // VETROC
  }};
  const auto* item =
    find_if(ALL(modelpar), [model]( const ModelPar_t& modelParam ) { return model == modelParam.model; });
  if( item != modelpar.end() ) {
    crdat[crate].sltdat[slot].nchan = item->nchan;
    crdat[crate].sltdat[slot].ndata = item->ndata;
    return 1;
  }
  return 0;
}

//_____________________________________________________________________________
void THaCrateMap::setUsed( UInt_t crate, UInt_t slot )
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  auto& cr = crdat[crate];
  cr.crate_used = true;
  cr.sltdat[slot].used = true;
  if( std::find(used_crates.begin(), used_crates.end(), crate) == used_crates.end() )
    used_crates.push_back(crate);
  if( std::find(cr.used_slots.begin(), cr.used_slots.end(), slot) == cr.used_slots.end() ) {
    cr.used_slots.push_back(slot);
  }
}

//_____________________________________________________________________________
void THaCrateMap::setUnused( UInt_t crate, UInt_t slot )
{
  assert( crate < crdat.size() && slot < crdat[crate].sltdat.size() );
  auto& cr = crdat[crate];
  cr.sltdat[slot].used = false;
  auto it = std::find(cr.used_slots.begin(), cr.used_slots.end(), slot);
  if( it != cr.used_slots.end() ) {
    cr.used_slots.erase(it);
  }
  if( cr.used_slots.empty() ) {
    auto jt = std::find(used_crates.begin(), used_crates.end(), crate);
    if( jt != used_crates.end() )
      used_crates.erase(jt);
  }
}

//_____________________________________________________________________________
Int_t THaCrateMap::readFile( FILE* fi, string& text )
{
  // Read contents of opened file 'fi' to 'text'
  if( !fi )
    return CM_ERR;
  Int_t ret = CM_ERR;
  errno = 0;
  if( fseek(fi, 0, SEEK_END) == 0 ) {
    long size = ftell(fi);
    if( size > 0 ) {
      rewind(fi);
      unique_ptr<char[]> fbuf{new char[size]};
      size_t nread = fread(fbuf.get(), sizeof(char), size, fi);
      if( nread == static_cast<size_t>(size) ) {
        text.reserve(nread);
        text.assign(fbuf.get(), fbuf.get()+nread);
        ret = CM_OK;
      }
    }
  }
  return ret;
}

//_____________________________________________________________________________
int THaCrateMap::init( FILE* fi, const char* fname )
{
  // Get the crate map definition string from the given database file.
  // The file is read completely into an internal string, which is then
  // parsed by init(const std::string&).

  const char* const here = "THaCrateMap::init(file)";

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
    fclose(fi);
    return CM_ERR;
  }
  fclose(fi);

  // Parse the crate map definition
  return init(db);
}

//_____________________________________________________________________________
int THaCrateMap::init(ULong64_t tloc)
{
  // Initialize the crate map from the database.
  // 'tloc' is the time-stamp/index into the database's periods of validity.

  const char* const here = "THaCrateMap::init(tloc)";
  fInitTime = tloc;
  FILE* fi = Podd::OpenDBFile(fDBfileName.c_str(), fInitTime, here, "r", 1);
  return init(fi, fDBfileName.c_str());
}

//_____________________________________________________________________________
void THaCrateMap::print(ostream& os) const
{
  // Pretty-print crate map
  for( UInt_t roc = 0; roc < crdat.size(); roc++ ) {
    const auto& cr = crdat[roc];
    if( !cr.crate_used || cr.used_slots.empty() ) continue;
    os << "==== Crate " << roc << " type " << cr.crate_type_name;
    if( !cr.scalerloc.empty() )  os << " \"" << cr.scalerloc << "\"";
    os << endl;
    os << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( UInt_t slot = 0; slot < cr.sltdat.size(); slot++ ) {
      const auto& slt = cr.sltdat[slot];
      if( !slt.used ) continue;
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
Int_t THaCrateMap::loadConfig( string& line, string& cfgstr )
{
  // Check if module configuration option is specified in 'line' and,
  // if so, extract it and erase the option string from 'line'.
  const char* const here = "THaCrateMap::loadConfig";
  auto pos1 = line.find("cfg:");
  auto pos2 = line.find("dbfile:");
  bool have_cfg = (pos1 != string::npos);
  bool have_dbf = (pos2 != string::npos);
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
      FILE* fi = Podd::OpenDBFile(fname.c_str(), fInitTime, here);
      if ( !fi ) {
        ::Error(here, "Error opening decoder module database file "
                      "\"db_%s.dat\": %s\n line = %s",
                fname.c_str(), StrError().c_str(), line.c_str() );
        return CM_ERR;
      }
      if( readFile(fi, cfgstr) != CM_OK ) {
        ::Error(here, "Error reading decoder module database file "
                      "\"db_%s.dat\": %s", fname.c_str(), StrError().c_str());
        fclose(fi);
        return CM_ERR;
      }
      fclose(fi);
      line.erase(pos2);
    }
    Podd::Trim(cfgstr);
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::init(const string& the_map)
{
  // initialize the crate-map according to the lines in the string 'the_map'
  // parse each line separately, to ensure that the format is correct

  const char* const here = "THaCrateMap::init(string)";

  if ( the_map.empty() ) {
    // Warn if we didn't get any data
    ::Warning( here, "Empty crate map definition. Decoder will not be usable. "
        "Check database." );
  }

  // For the time being, we use a simple hardcoded array size
  UInt_t ncrates = MAXROC;
  crdat.clear();   // support re-init
  crdat.resize(ncrates);
  used_crates.clear(); used_crates.reserve(ncrates/2);

  UInt_t crate = kMaxUInt; // current CRATE
  string line; line.reserve(128);
  istringstream s(the_map);
  while( getline(s, line) ) {
    auto pos = line.find_first_of("!#");    // drop comments
    if( pos != string::npos )
      line.erase(pos);

    if( line.find_first_not_of(" \t") == string::npos )
      continue; // empty or blank line

// Make the line "==== Crate" not care about how many "=" chars or other
// chars before "Crate", but lines beginning in # are still a comment
    pos = line.find("Crate");
    if( pos != string::npos )
      line.erase(0, pos);

// set the next CRATE number and type

    char ctype[21];
    if( sscanf(line.c_str(), "Crate %u type %20s", &crate, ctype) == 2 ) {
      if( crate >= crdat.size() || setCrateType(crate, ctype) != CM_OK ) {
        cerr << "THaCrateMap:: fatal ERROR 2  setCrateType " << endl;
	return CM_ERR;
      }
      // for a scaler crate, get the 'name' or location as well
      if( crdat[crate].crate_code == kScaler ) {
        if( sscanf(line.c_str(), "Crate %*u type %*s %20s", ctype) != 1 ) {
          cerr << "THaCrateMap:: fatal ERROR 3   " << endl;
	  return CM_ERR;
	}
	TString scaler_name(ctype);
        scaler_name.ReplaceAll("\"", ""); // drop extra quotes
        crdat[crate].scalerloc = scaler_name;
      }
      continue; // onto the next line
    }

    // Extract any configuration string given on the line and, if found,
    // remove it from line and put a copy in cfgstr. This is primarily intended
    // for bank-decoded VME and scaler modules.
    string cfgstr;
    auto code = crdat[crate].crate_code;
    if( code == kVME /*or code == kScaler*/ ) {
      Int_t st = loadConfig(line, cfgstr);
      if( st != CM_OK )
        return st;
    }

    // The line is of the format:
    //        slot#  model#  [clear header  mask  nchan ndata ]
    // where clear, header, mask, nchan and ndata are optional interpreted in
    // that order.
    // Another option is "bank decoding" :  all data in this CODA bank
    // belongs to this slot and model.  The line has the format
    //        slot#  model#  bank#

    // Default values:
    Int_t  cword = 1, imodel = 0;
    UInt_t slot = kMaxUInt;
    UInt_t mask = 0, iheader = 0, ichan = MAXCHAN, idata = MAXDATA;
    // must read at least the slot and model numbers
    Int_t nread = sscanf(line.c_str(),"%u %d %d %x %x %u %u",
                         &slot,&imodel,&cword,&iheader,&mask,&ichan,&idata);
    if( nread < 2 ) {
      // unexpected input
      cerr << "THaCrateMap:: fatal ERROR 4   " << endl << "Bad line " << endl << line << endl;
      cerr << "    Warning: a bad line could cause wrong decoding !" << endl;
      return CM_ERR;
    }

    if (nread > 5 )
      setModel(crate,slot,imodel,ichan,idata);
    else
      setModel(crate,slot,imodel);

    SlotInfo_t& slt = crdat[crate].sltdat[slot];
    if( nread == 3 )
      slt.bank = cword;
    else if( nread > 3 ) {
      slt.clear = cword;
      slt.header = iheader;
      if( nread > 4 )
        slt.headmask = mask;
    }
    slt.cfgstr = std::move(cfgstr);
  }

  for( auto& cr : crdat ) {
    sort(ALL(cr.used_slots));
    cr.bank_structure =
      any_of(ALL(cr.sltdat),
             []( const SlotInfo_t& slt ) { return slt.bank >= 0; } );
  }
  sort(ALL(used_crates));

  return CM_OK;
}

//_____________________________________________________________________________
THaCrateMap::CrateInfo_t::CrateInfo_t() :
  crate_code(kUnknown), crate_type_name("unknown"),
  crate_used(false), bank_structure(false)
{
  used_slots.reserve(sltdat.size()/2);
}

} // namespace Decoder

//_____________________________________________________________________________
ClassImp(Decoder::THaCrateMap)
