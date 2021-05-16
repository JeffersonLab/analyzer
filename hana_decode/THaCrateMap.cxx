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
#include <iostream>

#include "TError.h"
#include "TSystem.h"

#include <cstdio>
#include <cerrno>  // for errno
#include <string>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <cstring>  // for strerror_r

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
  : didslot{}
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
  for (UInt_t crate=0; crate<MAXROC; crate++) {
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
  assert(crate < MAXROC);
  TString stype(type);
  crdat[crate].crate_used = true;
  crdat[crate].crate_type = stype;
  if( stype == "fastbus" )
    crdat[crate].crate_code = kFastbus;
  else if( stype == "vme" )
    crdat[crate].crate_code = kVME;
  else if( stype == "scaler" )
    crdat[crate].crate_code = kScaler;
  else if( stype == "camac" )
    crdat[crate].crate_code = kCamac;
  else {
    crdat[crate].crate_used = false;
    crdat[crate].crate_type = "unknown";
    crdat[crate].crate_code = kUnknown;
    return CM_ERR;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setModel( UInt_t crate, UInt_t slot, Int_t mod,
                           UInt_t nchan, UInt_t ndata ) {
  assert( crate < MAXROC && slot < MAXSLOT );
  setUsed(crate,slot);
  crdat[crate].model[slot] = mod;
  if( !SetModelSize( crate, slot, mod )) {
    crdat[crate].nchan[slot] = nchan;
    crdat[crate].ndata[slot] = ndata;
  }
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::SetModelSize( UInt_t crate, UInt_t slot, UInt_t model )
{
  // Set the max number of channels and data words for some known modules
  assert( crate < MAXROC && slot < MAXSLOT );
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
  assert( crate < MAXROC && slot < MAXSLOT );
  incrNslot(crate);
  setUsed(crate,slot);
  crdat[crate].header[slot] = head;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setMask( UInt_t crate, UInt_t slot, UInt_t mask ) {
  assert( crate < MAXROC && slot < MAXSLOT );
  incrNslot(crate);
  setUsed(crate,slot);
  crdat[crate].headmask[slot] = mask;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setBank( UInt_t crate, UInt_t slot, Int_t bank ) {
  assert( crate < MAXROC && slot < MAXSLOT );
  incrNslot(crate);
  setUsed(crate,slot);
  crdat[crate].bank[slot] = bank;
  return CM_OK;
}

//_____________________________________________________________________________
int THaCrateMap::setScalerLoc( UInt_t crate, const char* location ) {
  assert( crate < MAXROC );
  incrNslot(crate);
  setCrateType(crate,"scaler");
  crdat[crate].scalerloc = location;
  return CM_OK;
}

void THaCrateMap::incrNslot( UInt_t crate) {
  assert( crate < MAXROC );
  //FIXME: urgh, really count every time?
  crdat[crate].nslot = 0;
  for( auto slot : crdat[crate].model ) {
    if( slot != 0 )
      crdat[crate].nslot++;
  }
}


int THaCrateMap::init( FILE* fi, const TString& fname )
{
  // Get the crate map definition string from the given database file.
  // The file is read completely into an internal string, which is then
  // parsed by init(TString).

  const char* const here = "THaCrateMap::init";

  if ( !fi ) {
    ::Error( here, "Error opening crate map database file %s: %s",
        fname.Data(), StrError().c_str() );
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
        fname.Data(), StrError().c_str() );
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

void THaCrateMap::print(ostream& os) const
{
  for( UInt_t roc=0; roc<MAXROC; roc++ ) {
    if( !crdat[roc].crate_used || crdat[roc].nslot ==0 ) continue;
    os << "==== Crate " << roc << " type " << crdat[roc].crate_type;
    if( !crdat[roc].scalerloc.IsNull() )  os << " \"" << crdat[roc].scalerloc << "\"";
    os << endl;
    os << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( UInt_t slot=0; slot<MAXSLOT; slot++ ) {
      if( !crdat[roc].slot_used[slot] ) continue;
      os << "  " << slot << "\t" << crdat[roc].model[slot]
	   << "\t" << crdat[roc].slot_clear[slot];
      // using this instead of manipulators again for backwards compat with g++-2
      ios::fmtflags oldf = os.setf(ios::right, ios::adjustfield);
      os << "\t0x" << hex << setfill('0') << setw(8) << crdat[roc].header[slot]
	   << "\t0x" << hex << setfill('0') << setw(8) << crdat[roc].headmask[slot]
	   << dec << setfill(' ') << setw(0)
	   << "\t" << crdat[roc].nchan[slot]
	   << "\t" << crdat[roc].ndata[slot]
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

  istringstream s(the_map);

  UInt_t linecnt = 0;
  string line;
  UInt_t crate; // current CRATE
  UInt_t slot;

  typedef string::size_type ssiz_t;

  for(crate=0; crate<MAXROC; crate++) {
    crdat[crate].nslot = 0;
    crdat[crate].crate_used = false;
    crdat[crate].bank_structure = false;
    setCrateType(crate,"unknown"); //   crate_type[crate] = "unknown";
    crdat[crate].minslot=MAXSLOT;
    crdat[crate].maxslot=0;
    for(slot=0; slot<MAXSLOT; slot++) {
      crdat[crate].slot_used[slot] = false;
      crdat[crate].model[slot] = 0;
      crdat[crate].header[slot] = 0;
      crdat[crate].slot_clear[slot] = true;
      crdat[crate].bank[slot] = -1;
    }
  }

  crate=-1; // current CRATE

  while ( getline(s,line).good() ) {
    linecnt++;
    ssiz_t l = line.find_first_of("!#");    // drop comments
    if (l != string::npos ) line.erase(l);

    if ( line.empty() ) continue;

    if ( line.find_first_not_of(" \t") == string::npos ) continue; // nothing useful

    char ctype[21];

// Make the line "==== Crate" not care about how many "=" chars or other
// chars before "Crate", but lines beginning in # are still a comment
    ssiz_t st = line.find("Crate", 0, 5);
    if (st != string::npos) {
      string lcopy = line;
      line.replace(0, lcopy.length(), lcopy, st, st+lcopy.length());
    }

// set the next CRATE number and type

    if ( sscanf(line.c_str(),"Crate %d type %20s",&crate,ctype) == 2 ) {
      if ( setCrateType(crate,ctype) != CM_OK )  {
        cout << "THaCrateMap:: fatal ERROR 2  setCrateType "<<endl;
	return CM_ERR;
      }
      // for a scaler crate, get the 'name' or location as well
      if ( crdat[crate].crate_code == kScaler ) {
	if (sscanf(line.c_str(),"Crate %*d type %*s %20s",ctype) != 1) {
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
    Int_t  cword=1, imodel;
    UInt_t mask=0, iheader=0, ichan=MAXCHAN, idata=MAXDATA;
    // must read at least the slot and model numbers
    Int_t nread = sscanf(line.c_str(),"%u %u %d %x %x %u %u",
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

  for(crate=0; crate<MAXROC; crate++) {
    UInt_t imin=MAXSLOT;
    UInt_t imax=0;
    for(slot=0; slot<MAXSLOT; slot++) {
      if (crdat[crate].bank[slot]>=0) crdat[crate].bank_structure=true;
      if (crdat[crate].slot_used[slot]) {
	if (slot < imin) imin=slot;
        if (slot > imax) imax=slot;
      }
    }
    crdat[crate].minslot=imin;
    crdat[crate].maxslot=imax;
  }

  return CM_OK;
}

}

ClassImp(Decoder::THaCrateMap)
