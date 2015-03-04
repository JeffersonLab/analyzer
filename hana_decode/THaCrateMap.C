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
#include <ctime>

#ifndef STANDALONE
#include "THaAnalysisObject.h"
#endif

#include "TDatime.h"
#include "TError.h"

#include <cstdio>
#include <string>
#include <iomanip>

#include <sstream>
#define ISSTREAM istringstream
#define OSSTREAM ostringstream
#define ASSIGN_SSTREAM(a,b) a=b.str()

using namespace std;

namespace Decoder {

const UShort_t THaCrateMap::MAXCHAN = 100;
const UShort_t THaCrateMap::MAXDATA = 1024;
const int THaCrateMap::CM_OK = 1;
const int THaCrateMap::CM_ERR = -1;

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

int THaCrateMap::getScalerCrate(int data) const {
  for (int crate=0; crate<MAXROC; crate++) {
    if (!crdat[crate].crate_used) continue;
    if (isScalerCrate(crate)) {
      int headtry = data&0xfff00000;
      int zero = data&0x0000ff00;
      if ((zero == 0) &&
	  (headtry == crdat[crate].header[1])) return crate;
    }
  }
  return 0;
}


int THaCrateMap::setCrateType(int crate, const char* ctype) {
  assert( crate >= 0 && crate < MAXROC );
  TString type(ctype);
  crdat[crate].crate_used = true;
  crdat[crate].crate_type = type;
  if (type == "fastbus")
    crdat[crate].crate_code = kFastbus;
  else if (type == "vme")
    crdat[crate].crate_code = kVME;
  else if (type == "scaler")
    crdat[crate].crate_code = kScaler;
  else if (type == "camac")
    crdat[crate].crate_code = kCamac;
  else {
    crdat[crate].crate_used = false;
    crdat[crate].crate_type = "unknown";
    crdat[crate].crate_code = kUnknown;
    return CM_ERR;
  }
  return CM_OK;
}

int THaCrateMap::setModel(int crate, int slot, UShort_t mod,
			  UShort_t nc, UShort_t nd ) {
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  setUsed(crate,slot);
  crdat[crate].model[slot] = mod;
  if( !SetModelSize( crate, slot, mod )) {
    crdat[crate].nchan[slot] = nc;
    crdat[crate].ndata[slot] = nd;
  }
  return CM_OK;
}

int THaCrateMap::SetModelSize( int crate, int slot, UShort_t imodel )
{
  // Set the max number of channels and data words for some known modules
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  struct ModelPar_t { UShort_t model, nchan, ndata; };
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
    { 3201, 32, 512 },  // JLab F1 TDC high resolution
    { 792, 32, 32 },  // CAEN V792 QDC
    { 1190, 128, 1024 }, //CAEN 1190A
    { 0 }
  };
  const ModelPar_t* item = modelpar;
  while( item->model ) {
    if( imodel == item->model ) {
      crdat[crate].nchan[slot] = item->nchan;
      crdat[crate].ndata[slot] = item->ndata;
      return 1;
    }
    item++;
  }
  return 0;
}

int THaCrateMap::setHeader(int crate, int slot, int head) {
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  incrNslot(crate);
  setUsed(crate,slot);
  crdat[crate].header[slot] = head;
  return CM_OK;
}

int THaCrateMap::setMask(int crate, int slot, int mask) {
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  incrNslot(crate);
  setUsed(crate,slot);
  crdat[crate].headmask[slot] = mask;
  return CM_OK;
}

int THaCrateMap::setScalerLoc(int crate, const char* loc) {
  assert( crate >= 0 && crate < MAXROC );
  incrNslot(crate);
  setCrateType(crate,"scaler");
  crdat[crate].scalerloc = loc;
  return CM_OK;
}

void THaCrateMap::incrNslot(int crate) {
  assert( crate >= 0 && crate < MAXROC );
  //FIXME: urgh, really count every time?
  crdat[crate].nslot = 0;
  for (int slot=0; slot<MAXSLOT; slot++) {
    if (crdat[crate].model[slot] != 0)
      crdat[crate].nslot++;
  }
}

int THaCrateMap::init(ULong64_t tloc) {
  // Modify the interface to be able to use a database file.
  // The real work is done by the init(TString&) method

  // Only print warning/error messages exactly once
  //  static bool first = true;
  static const char* const here = "THaCrateMap::init";

  //FIXME: replace with TTimeStamp
  TDatime date((UInt_t)tloc);

  TString db;

#ifndef STANDALONE
  FILE* fi = THaAnalysisObject::OpenFile(fDBfileName,date,here,"r",0);
#else
  TString fname = "db_"; fname.Append(fDBfileName); fname.Append(".dat");
  FILE* fi = fopen(fname,"r");
#endif
  if ( fi ) {
    // just build the string to parse later
    int ch;
    while ( (ch = fgetc(fi)) != EOF ) {
      db += static_cast<char>(ch);
    }
    fclose(fi);
  }

  if ( db.Length() <= 0 ) {
    // Die if we can't open the crate map file
    ::Error( here, "Error reading crate map database file db_%s.dat",
	    fDBfileName.Data() );
    return CM_ERR;
  }
  return init(db);
}

void THaCrateMap::print(ofstream *file) const {
{
  for( int roc=0; roc<MAXROC; roc++ ) {
    if( !crdat[roc].crate_used || crdat[roc].nslot ==0 ) continue;
    *file << "==== Crate " << roc << " type " << crdat[roc].crate_type;
    if( !crdat[roc].scalerloc.IsNull() )  *file << " \"" << crdat[roc].scalerloc << "\"";
    *file << endl;
    *file << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( int slot=0; slot<MAXSLOT; slot++ ) {
      if( !slotUsed(roc,slot) ) continue;
      *file << "  " << slot << "\t" << crdat[roc].model[slot]
	   << "\t" << crdat[roc].slot_clear[slot];
      *file << "   \t0x" << hex << crdat[roc].header[slot]
	   << "    \t0x" << hex  << crdat[roc].headmask[slot]
	   << dec << "   "
	   << "  \t" << crdat[roc].nchan[slot]
	   << "  \t" << crdat[roc].ndata[slot]
	   << endl;
    }
  }
}

}


void THaCrateMap::print() const
{
  for( int roc=0; roc<MAXROC; roc++ ) {
    if( !crdat[roc].crate_used || crdat[roc].nslot ==0 ) continue;
    cout << "==== Crate " << roc << " type " << crdat[roc].crate_type;
    if( !crdat[roc].scalerloc.IsNull() )  cout << " \"" << crdat[roc].scalerloc << "\"";
    cout << endl;
    cout << "#slot\tmodel\tclear\t  header\t  mask  \tnchan\tndata\n";
    for( int slot=0; slot<MAXSLOT; slot++ ) {
      if( !crdat[roc].slot_used[slot] ) continue;
      cout << "  " << slot << "\t" << crdat[roc].model[slot]
	   << "\t" << crdat[roc].slot_clear[slot];
      // using this instead of manipulators again for bckwards compat with g++-2
      ios::fmtflags oldf = cout.setf(ios::right, ios::adjustfield);
      cout << "\t0x" << hex << setfill('0') << setw(8) << crdat[roc].header[slot]
	   << "\t0x" << hex << setfill('0') << setw(8) << crdat[roc].headmask[slot]
	   << dec << setfill(' ') << setw(0)
	   << "\t" << crdat[roc].nchan[slot]
	   << "\t" << crdat[roc].ndata[slot]
	   << endl;
      cout.flags(oldf);
    }
  }
}

int THaCrateMap::init(TString the_map)
{
  // initialize the crate-map according to the lines in the string 'the_map'
  // parse each line separately, to ensure that the format is correct

  // be certain the_map ends with a '\0' so we can make a stringstream from it
  the_map += '\0';
  ISSTREAM s(the_map.Data());

  int linecnt = 0;
  string line;
  int crate; // current CRATE
  int slot;

  typedef string::size_type ssiz_t;

  for(crate=0; crate<MAXROC; crate++) {
    crdat[crate].nslot = 0;
    crdat[crate].crate_used = false;
    setCrateType(crate,"unknown"); //   crate_type[crate] = "unknown";
    crdat[crate].minslot=MAXSLOT;
    crdat[crate].maxslot=0;
    for(slot=0; slot<MAXSLOT; slot++) {
      crdat[crate].slot_used[slot] = false;
      crdat[crate].model[slot] = 0;
      crdat[crate].header[slot] = 0;
      crdat[crate].slot_clear[slot] = true;
    }
  }

  crate=-1; // current CRATE

  while ( getline(s,line).good() ) {
    linecnt++;
    ssiz_t l = line.find_first_of("!#");    // drop comments
    if (l != string::npos ) line.erase(l);

    if ( line.length() <= 0 ) continue;

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
    // where clear, header, mask, nchan and ndata are optional interpretted in
    // that order.

    // Default values:
    int imodel, clear=1;
    unsigned int mask=0, iheader=0, ichan=MAXCHAN, idata=MAXDATA;
    int nread;
    // must read at least the slot and model numbers
    if ( crate>=0 &&
	 (nread=
	  sscanf(line.c_str(),"%d %d %d %x %x %u %u",
		 &slot,&imodel,&clear,&iheader,&mask,&ichan,&idata)) >=2 ) {
      if (nread>=6)
	setModel(crate,slot,imodel,ichan,idata);
      else
	setModel(crate,slot,imodel);

      if (nread>=3)
	setClear(crate,slot,clear);
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
    Int_t imin=MAXSLOT;
    Int_t imax=0;
    for(slot=0; slot<MAXSLOT; slot++) {
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
