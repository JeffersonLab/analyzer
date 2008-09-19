#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include "TString.h"

#include "THaDB.h"
#include "THaDetMap.h"

using namespace std;



THaDB* gHaDB = NULL;

typedef string::size_type ssiz_t;

//_____________________________________________________________________________

THaDB::THaDB()
{ if (!gHaDB) gHaDB=this; }

//_____________________________________________________________________________
THaDB::~THaDB() {
  if (gHaDB == this) {
    gHaDB = NULL;
  }
}

//_____________________________________________________________________________
ostream& operator<<(ostream& s, const THaDetConfig& d) {
  // look only at complete lines
  s << Form(" %-10s",d.name.c_str());
  s << Form("      %5d",d.wir);
  s << Form("    %5d",d.roc);
  s << Form("     %5d",d.crate);
  s << Form("        %5d",d.slot);
  s << Form("      %3d - %3d",d.chan_lo,d.chan_hi);
  s << Form("      %8s",d.model.c_str());
  
//    s << string(6,' ')  << setw(5) << d.wir;
//    s << string(4,' ') << setw(5) << d.roc;
//    s << string(5,' ') << setw(5) << d.crate;
//    s << string(8,' ') << setw(5) << d.slot;
//    s << string(6,' ') << setw(3) << d.chan_lo;
//    s << " - " << setw(3) << d.chan_hi;
//    s << string(6,' ') << setw(8) << d.model;
  return s;
}

//_____________________________________________________________________________
bool THaDetConfig::IsGood() const {
  return name != "JUNK";
}

//_____________________________________________________________________________
THaDetConfig::THaDetConfig(string line) {
  // Read in the detector map, with the same functionality as load_detmap.f,
  // not the advertised adjustable range and specifiers listed for the routine
  // in the det_map configuration file.

  // '!' and '#' are comment characters, commenting out the rest of a line
  // the format of a line is:
  // DET_NAME  FIRST_WIR  ROC  CRATE  SLOT  CHANNEL_LO  CHANNEL_HI  SLOT_MODEL

  // look only at complete lines

  ssiz_t l = line.find_first_of("!#");
  if ( l != string::npos ) line.erase(l);

  ssiz_t pos=0,pos2;

  // replace '-' and ':' with ' '
  while ( (pos = line.find_first_of(":-\t")) != string::npos ) {
    line.replace(pos,1," ");
  }

  // if it is an entirely blank line, just leave
  
  // look through the string, grab out the fields
  string data;
  vector<string> v;

  pos = 0;
  while ( pos != string::npos
	  && (pos = line.find_first_not_of(' ',pos)) != string::npos ) {
    
    pos2 = line.find_first_of(' ',pos);

    if ( pos2 != string::npos ) {
      data = line.substr( pos, pos2-pos );
    } else {
      data = line.substr( pos, pos2 );
    }

    v.push_back(data);
    pos = pos2;
  }

  if ( v.size()>0 && v.size() != 8 ) {
    cerr << "Error reading Detector Configuration! The line is: " << endl
	 << line << endl;
    v.clear();
  }
  if (v.size()==0) {
    name = "JUNK";
    return;
  }
  
  
  name = v[0];
  wir = atoi(v[1].c_str());
  roc = atoi(v[2].c_str());
  crate = atoi(v[3].c_str());
  slot = atoi(v[4].c_str());
  chan_lo = atoi(v[5].c_str());
  chan_hi = atoi(v[6].c_str());
  model = v[7];

}

//_____________________________________________________________________________
THaDetConfig::~THaDetConfig() { }

//_____________________________________________________________________________
Int_t THaDB::GetDetMap( const char* sysname, THaDetMap& detmap, const TDatime& date ) {
  // look through the configuration file to determine the
  // mapping of this detector to the read-out electronics.

  // read in the configuration file, if necessary

  if (fDetectorMap.size()<=0) {
    LoadDetMap(date);   // load detector map into memory
  }
  
  // look through the detector map, matching on the string
  typedef vector<THaDetConfig>::const_iterator CI;
  Int_t nfnd = 0;
  
  for (CI i=fDetectorMap.begin(); i != fDetectorMap.end() ; i++) {
    const THaDetConfig& dc = *i;
    if ( dc.name == sysname ) {
      if ( detmap.AddModule(dc.roc,dc.slot,dc.chan_lo,dc.chan_hi,dc.wir,atoi(dc.model.c_str())) < 0 ) {
	Error("THaDB::Get_DetMap","Too many modules listed for %s. They will be dropped!",sysname);
	return -1;
      }
      nfnd++;
    }
  }
  
  return nfnd;
}
  
////////////////////////////////////////////////////////////////////////////////

ClassImp(THaDB)
ClassImp(THaDetConfig)
