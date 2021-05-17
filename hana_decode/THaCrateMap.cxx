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
#include <memory>

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
UInt_t THaCrateMap::getScalerCrate( UInt_t word) const {
  for( UInt_t crate = 0; crate < crdat.size(); crate++ ) {
    if( !crdat[crate].crate_used ) continue;
    if( isScalerCrate(crate) ) {
      UInt_t headtry = word & 0xfff00000;
      UInt_t zero = word & 0x0000ff00;
      if( (zero == 0) &&
          (headtry == crdat[crate].header[1]) )
        return crate;
    }
  }
  return 0;
}

//_____________________________________________________________________________
int THaCrateMap::setCrateType( UInt_t crate, const char* type ) {
  assert(crate < crdat.size());
  string stype(type);
  CrateInfo_t& cr = crdat[crate];
  cr.crate_used = true;
  cr.crate_type = stype;
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
    cr.crate_type = "unknown";
    cr.crate_code = kUnknown;
    return CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setModel( UInt_t crate, UInt_t slot, Int_t mod,
                           UInt_t nchan, UInt_t ndata ) {
  assert( crate < crdat.size() && slot < crdat[crate].model.size() );
  setUsed(crate,slot);
  CrateInfo_t& cr = crdat[crate];
  if( cr.model[slot] == 0 && mod != 0 )
    cr.nslot++;
  else if( cr.model[slot] != 0 && mod == 0 && cr.nslot > 0 )
    cr.nslot--;
  cr.model[slot] = mod;
  if( !SetModelSize( crate, slot, mod )) {
    cr.nchan[slot] = nchan;
    cr.ndata[slot] = ndata;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::SetModelSize( UInt_t crate, UInt_t slot, UInt_t model )
{
  // Set the max number of channels and data words for some known modules
  assert( crate < crdat.size() && slot < crdat[crate].nchan.size() );
  struct ModelPar_t { UInt_t model, nchan, ndata; };
  static const ModelPar_t modelpar[] = {
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
    { 0 }
  };
  const ModelPar_t* item = modelpar;
  while( item->model ) {
    if( model == item->model ) {
      crdat[crate].nchan[slot] = item->nchan;
      crdat[crate].ndata[slot] = item->ndata;
      return 1;
    }
    item++;
  }
  return 0;
}

//_____________________________________________________________________________
int THaCrateMap::setHeader( UInt_t crate, UInt_t slot, UInt_t head ) {
  assert( crate < crdat.size() && slot < crdat[crate].header.size() );
  setUsed(crate,slot);
  crdat[crate].header[slot] = head;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setMask( UInt_t crate, UInt_t slot, UInt_t mask ) {
  assert( crate < crdat.size() && slot < crdat[crate].headmask.size() );
  setUsed(crate,slot);
  crdat[crate].headmask[slot] = mask;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setBank( UInt_t crate, UInt_t slot, Int_t bank ) {
  assert( crate < crdat.size() && slot < crdat[crate].bank.size() );
  setUsed(crate,slot);
  crdat[crate].bank[slot] = bank;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setScalerLoc( UInt_t crate, const char* location ) {
  assert( crate < crdat.size() );
  setCrateType(crate,"scaler");
  crdat[crate].scalerloc = location;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::init( FILE* fi, const char* fname )
{
  // Get the crate map definition string from the given database file.
  // The file is read completely into an internal string, which is then
  // parsed by init(TString).

  const char* const here = "THaCrateMap::init";

  if ( !fi ) {
    ::Error( here, "Error opening crate map database file %s: %s",
        fname, StrError().c_str() );
    return CM_ERR;
  }
  // Build the string to parse
  string db;
  bool ok = false;
  errno = 0;
  if( fseek(fi, 0, SEEK_END) == 0 ) {
    long size = ftell(fi);
    if( size > 0 ) {
      rewind(fi);
      unique_ptr<char[]> fbuf{new char[size]};
      size_t nread = fread(fbuf.get(), sizeof(char), size, fi);
      if( nread == static_cast<size_t>(size) ) {
        db.reserve(nread);
        db.assign(fbuf.get(), fbuf.get()+nread);
        ok = true;
      }
    }
  }
  if( !ok || ferror(fi) ) {
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
int THaCrateMap::init(ULong64_t /*tloc*/) {
  // Initialize the crate map from the database.
  //
  // If a time-dependent or run-number indexed database is available,
  // (in derived classes), use the given 'tloc' argument as the index.
  //
  // If the database file name (fDBfileName) contains any slashes, it is
  // assumed to be the actual file path.
  //
  // If opening fDBfileName fails, no slash is present in the name, and the
  // name does not already have the format "db_*.dat", the routine will also
  // try to open "db_<fDBfileName>.dat".
  //
  // If the file is still not found, but DB_DIR is defined in the environment,
  // repeat the search in $DB_DIR.

  TString fname(fDBfileName);

  FILE* fi = fopen(fname,"r");
  if( !fi ) {
    TString dbfname;
    Ssiz_t pos = fname.Index('/');
    bool no_slash = (pos == kNPOS);
    bool no_abspath = (pos != 0);
    if( no_slash && !fname.BeginsWith("db_") && !fname.EndsWith(".dat") ) {
      dbfname = "db_" + fname +".dat";
      fi = fopen(dbfname,"r");
    }
    if( !fi && no_abspath ) {
      const char* db_dir = gSystem->Getenv("DB_DIR");
      if( db_dir ) {
        long int path_max = pathconf(".", _PC_PATH_MAX);
        if( path_max <= 0 )
          path_max = 1024;
        if( strlen(db_dir) <= static_cast<size_t>(path_max) ) {
          TString dbdir(db_dir);
          if( !dbdir.EndsWith("/") )
            dbdir.Append('/');
          fname.Prepend(dbdir);
          fi = fopen(fname,"r");
          if( !fi && !dbfname.IsNull() ) {
            dbfname.Prepend(dbdir);
            fi = fopen(dbfname,"r");
          }
          // if still !fi here, init(fi,...) below will report error
        }
      }
    }
  }
  return init(fi, fname);
}

//_____________________________________________________________________________
void THaCrateMap::print(ostream& os) const {
  // Pretty-print crate map
  for( UInt_t roc=0; roc<crdat.size(); roc++ ) {
    const CrateInfo_t& cr = crdat[roc];
    if( !cr.crate_used || cr.nslot == 0 ) continue;
    os << "==== Crate " << roc << " type " << cr.crate_type;
    if( !cr.scalerloc.empty() )  os << " \"" << cr.scalerloc << "\"";
    os << endl;
    os << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( UInt_t slot=0; slot<cr.model.size(); slot++ ) {
      if( !cr.slot_used[slot] ) continue;
      os << "  " << slot << "\t" << cr.model[slot]
         << "\t" << cr.slot_clear[slot];
      // using this instead of manipulators again for backwards compat with g++-2
      ios::fmtflags oldf = os.setf(ios::right, ios::adjustfield);
      os << "\t0x" << hex << setfill('0') << setw(8) << cr.header[slot]
	   << "\t0x" << hex << setfill('0') << setw(8) << cr.headmask[slot]
	   << dec << setfill(' ') << setw(0)
	   << "\t" << cr.nchan[slot]
	   << "\t" << cr.ndata[slot]
	   << endl;
      os.flags(oldf);
    }
  }
}

//_____________________________________________________________________________
int THaCrateMap::init(const string& the_map) {
  // initialize the crate-map according to the lines in the string 'the_map'
  // parse each line separately, to ensure that the format is correct

  const char* const here = "THaCrateMap::init";

  if ( the_map.empty() ) {
    // Warn if we didn't get any data
    ::Warning( here, "Empty crate map definition. Decoder will not be usable. "
        "Check database." );
  }

  // For the time being, we use simple hardcoded array sizes
  crdat.resize(MAXROC);
  for( auto& cr : crdat ) {
    cr.init(MAXSLOT);
  }
  didslot.assign(MAXSLOT, false);

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
        cout << "THaCrateMap:: fatal ERROR 2  setCrateType "<<endl;
	return CM_ERR;
      }
      // for a scaler crate, get the 'name' or location as well
      if( crdat[crate].crate_code == kScaler ) {
        if( sscanf(line.c_str(), "Crate %*d type %*s %20s", ctype) != 1 ) {
          cout << "THaCrateMap:: fatal ERROR 3   "<<endl;
	  return CM_ERR;
	}
	TString scaler_name(ctype);
	scaler_name.ReplaceAll("\"",""); // drop extra quotes
	setScalerLoc(crate,scaler_name);
      }
      continue; // onto the next line
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
    if( nread >=2 ) {
      if (nread>=6)
	setModel(crate,slot,imodel,ichan,idata);
      else
	setModel(crate,slot,imodel);

      if (nread==3) 
        setBank(crate, slot, cword);
      if (nread>3)
	setClear(crate,slot,cword);
      if (nread>=4)
	setHeader(crate,slot,iheader);
      if (nread>=5)
	setMask(crate,slot,mask);
      continue;
    }

    // unexpected input
    cout << "THaCrateMap:: fatal ERROR 4   "<<endl<<"Bad line "<<endl<<line<<endl;
    cout << "    Warning: a bad line could cause wrong decoding !"<<endl;

    return CM_ERR;
  }

  for( auto& cr : crdat ) {
    UInt_t imin = kMaxUInt;
    UInt_t imax = 0;
    for( UInt_t slot = 0; slot < cr.bank.size(); slot++ ) {
      if( cr.bank[slot] >= 0 ) cr.bank_structure = true;
      if( cr.slot_used[slot] ) {
        if( slot < imin ) imin = slot;
        if( slot > imax ) imax = slot;
      }
    }
    cr.minslot = imin;
    cr.maxslot = imax;
  }

  return CM_OK;
}

//_____________________________________________________________________________
void THaCrateMap::CrateInfo_t::init( UInt_t numslot ) {
  crate_code = kUnknown;
  crate_type = "unknown";
  scalerloc.clear();
  nslot = 0;
  minslot = kMaxUInt;
  maxslot = 0;
  model.assign(numslot, 0);
  header.assign(numslot, 0);
  headmask.assign(numslot, 0xFFFFFFFF);
  bank.assign(numslot, -1);
  nchan.assign(numslot, 0);
  ndata.assign(numslot, 0);
  slot_used.assign(numslot, false);
  slot_clear.assign(numslot, true);
  crate_used = false;
  bank_structure = false;
}

} // namespace Decoder

//_____________________________________________________________________________
ClassImp(Decoder::THaCrateMap)
