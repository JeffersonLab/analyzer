/////////////////////////////////////////////////////////////////////
//
//   THaScalerDB
//
//   A text-based time-dependent database for scaler
//   "crate maps" and "directives".
//
//   The "crate map" defines the correspondence
//   between  the following two groups of info:
//   SDB_chanKey =
//     [description of channel, i.e. detector name,
//        crate #, helicity]
//   -- and --
//   SDB_chanDesc =
//     [slot and channel info]
//
//   (the chanKey is internal to a detector,
//    the chanDesc refers to physical scaler units)
//
//   "directives" are other info one can obtain
//   from the map file, e.g. clock speed or
//   IP address of server (see GetDirectives()).
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaScalerDB.h"
#include "TError.h"
#include <cerrno>
#include <fstream>

using namespace std;

const std::string scomment = "#";
const std::string sdate = "DATE";

THaScalerDB::THaScalerDB() { Init(); }
THaScalerDB::~THaScalerDB() {
    if (direct) delete direct;
}

void THaScalerDB::Init() {
    direct = new SDB_directive();
    found_date = false;
    fgnfar=10;
    crate_strtoi.clear();
    directnames.clear();
    directnames.push_back("xscaler");
    directnames.push_back("helicity");
    directnames.push_back("crate");
    directnames.push_back("slot");
    directnames.push_back("target");
    fDebug = 0;
}

bool THaScalerDB::extract_db(const Bdate& bdate)
{
  // Load the database.
  // Input: Bdate = time (day,month,year) when database wanted
  // Return : true if successful, else false.
  // Search order for scaler.map:
  //   $DB_DIR/DEFAULT $DB_DIR ./DB/DEFAULT ./DB ./db/DEFAULT ./db DEFAULT .
  const string scalmap("scaler.map");
  char* dbdir=getenv("DB_DIR");
  Int_t ndir = 6;
  if( dbdir ) ndir += 2;
  string filename;
  vector<string> fname(ndir);
  Int_t i = 0;
  fname[i++] = scalmap; // always look in current directory first
  if( dbdir ) {
    string dbpath(dbdir);
    dbpath.append("/");
    fname[i++] = dbpath + "DEFAULT/" + scalmap;
    fname[i++] = dbpath + scalmap;
  }
  fname[i++] = "DB/DEFAULT/" + scalmap;
  fname[i++] = "DB/" + scalmap;
  fname[i++] = "db/DEFAULT/" + scalmap;
  fname[i++] = "db/" + scalmap;
  fname[i] = "DEFAULT/" + scalmap;
  i = 0;
  ifstream mapfile;
  do {
    mapfile.clear(); // needed to forget the previous 'misses'
    filename = fname[i];
    mapfile.open(filename.c_str());
  } while( (!mapfile) && (++i)<ndir );
  if ( !mapfile ) {
    // Bad but not fatal. Without a scaler.map one can
    // still "GetScaler" by crate, slot, chan.
    cerr << "WARNING: THaScalerDB: scaler.map file does not exist !!"<<endl;
    cerr << "You should have scaler.map in present working directory,"<<endl;
    cerr << "or in $DB_DIR directory." << endl;
    cerr << "Download  http://hallaweb.jlab.org/adaq/scaler.map"<<endl;
    return false;
  } else {
    ::Info("THaScalerDB", "Opened scaler map file %s", fname[i].c_str() );
  }
  string sinput;
  vector<string> strvect;
  found_date = false;

  // 1st pass: find the date, 2nd pass: load maps
  Bdate bd;  Bdate dfound(1,1,1990);
  while ( getline(mapfile,sinput) ) {
    if (GetLineType(sinput) == "DATE") {
      bd.load(vsplit(sinput));
      if (dfound <= bd && bd <= bdate) {
	found_date = true;  dfound = bd;
      }
    }
  }

  if (found_date) {
    dfound.Print();
    mapfile.close(); mapfile.clear();
    mapfile.open(filename.c_str());
#ifdef WITH_DEBUG
    if (fDebug) dfound.Print();
#endif
    while ( getline(mapfile,sinput) ) {
      if (GetLineType(sinput) == "DATE") {
	bd.load(vsplit(sinput));
#ifdef WITH_DEBUG
	if (fDebug) bd.Print();
#endif
	if (bd == dfound) {
	  while ( getline(mapfile,sinput) ) { // load until next DATE
	    string linetype = GetLineType(sinput);
	    if ( linetype == "DATE" ) goto finish1;
	    if ( linetype == "COMMENT") continue;
	    if (linetype == "MAP") {
              LoadMap(sinput);  continue;
	    }
	    LoadDirective(linetype); // otherwise its a "directive"
	  }
	}
      }
    }
  } else {
    ::Warning("THaScalerDB", "Did not find data in database" );
    return false;
  }

 finish1:

#ifdef WITH_DEBUG
  if (fDebug) {
    PrintChanMap();
    PrintDirectives();
  }
#endif
  return true;

};


void THaScalerDB::PrintChanMap() const {
  UInt_t i = 0;
  cout << "Print of Channel Map.  Size = "<<chanmap.size()<<endl;
  for( map< SDB_chanKey, SDB_chanDesc>::const_iterator pm = chanmap.begin();
       pm != chanmap.end(); ++pm) {
    i++;
    SDB_chanKey ck = pm->first;
    SDB_chanDesc cd = pm->second;
    cout << "Chan Key [" << i << "] = ";
    ck.Print();
    cout << "\nChan Desc [" << i << "] = ";
    cd.Print();
    cout << "-----------------------------------"<<endl;
  }
};

void THaScalerDB::PrintDirectives() const {
  if (direct) direct->Print();
};


Int_t THaScalerDB::SetDebug( Int_t level )
{
  Int_t prev_level = fDebug;
  fDebug = level;
  return prev_level;
}

static string::size_type AmtSpace( const string& s )
{
  string::size_type i = 0, nsp = 0;
  while (i++ != s.size()) if(isspace(s[i])) nsp++;
  return nsp;
}

string THaScalerDB::GetLineType(const string& sline) const
{
  // Decide if the line is a date, comment, directive, or a map field.
  if( sline.length() == 0 || sline.length() == AmtSpace(sline) )
    return "COMMENT";
  string::size_type pos1 = FindNoCase(sline,sdate);
  string::size_type pos2 = FindNoCase(sline,scomment);
  if( pos1 != string::npos ) { // date was found
    if( pos2 != string::npos ) {  // comment found
      if( pos2 < pos1 ) return "COMMENT";
    }
    return "DATE";
  }
  // Directives line, even if after a comment (#)
  // as long as not too far after (fgnfar)
  string result;
  if( pos2 == string::npos || pos2 < fgnfar ) {
    for( vector<string>::size_type i=0; i<directnames.size(); i++ ) {
      pos1 = FindNoCase(sline, directnames[i]);
      if( pos1 != string::npos ) {
	result.assign(sline.substr(pos1,sline.length()));
	return result;
      }
    }
  }
  // Not a directive but has a comment near start.
  if( pos2 != string::npos && pos2 < fgnfar ) return "COMMENT";
  // otherwise its a map line
  return "MAP";
}

SDB_chanDesc THaScalerDB::GetChanDesc(Int_t crate, const string& desc, Int_t helicity) const
{
  // No distinction by target state (they share channel mapping)
  // First check if crate is tied to the map of another crate.
  Int_t lcrate = TiedCrate(crate, helicity);
  Int_t lhelicity = helicity;
  // If this helicity is tied to helicity=0 map, we use helicity=0.
  if ( IsHelicityTied(crate, helicity) ) lhelicity = 0;
  if ( IsHelicityTied(crate, helicity) ) lhelicity = 0;
  SDB_chanKey sk(lcrate, lhelicity, desc);
  SDB_chanDesc cdesc(-1,"empty");  // empty so far
  map< SDB_chanKey, SDB_chanDesc>::const_iterator pm = chanmap.find(sk);
  if (pm != chanmap.end()) cdesc = pm->second;
  return cdesc;
}

string THaScalerDB::GetLongDesc(Int_t crate, const string& desc, Int_t helicity) const
{
  // Returns the slot in a scaler corresp. to the
  // channel in the detector described by "desc".
  SDB_chanDesc cdesc = GetChanDesc(crate, desc, helicity);
  return cdesc.GetDesc();
}

Int_t THaScalerDB::GetSlot(Int_t crate, const string& desc, Int_t helicity) const
{
  // Returns the slot in a scaler corresp. to the
  // channel in the detector described by "desc".
  SDB_chanDesc cdesc = GetChanDesc(crate, desc, helicity);
  return cdesc.GetSlot() + GetSlotOffset(crate, helicity);
}


Int_t THaScalerDB::GetSlot(Int_t crate, Int_t tgtstate, Int_t helicity) const
{
  // Returns the slot in a scaler corresp. to the
  // combination of target state and beam helicity.
  string subdir = "";
  if (tgtstate == 0 && helicity == 0) subdir="00";
  if (tgtstate == 1 && helicity == 1) subdir="pp";
  if (tgtstate == 1 && helicity ==-1) subdir="pm";
  if (tgtstate ==-1 && helicity == 1) subdir="mp";
  if (tgtstate ==-1 && helicity ==-1) subdir="mm";
  return GetIntDirectives(crate, "target-beam", subdir);
}

Int_t THaScalerDB::GetChan(Int_t crate, const string& desc, Int_t helicity, Int_t chan) const
{
  // Returns the channel in a scaler corresp. to the
  // channel in the detector described by "desc".
  SDB_chanDesc cdesc = GetChanDesc(crate, desc, helicity);
  return cdesc.GetChan(chan);
}

bool THaScalerDB::LoadMap(const string& sinput)
{
  typedef map<SDB_chanKey, SDB_chanDesc>::value_type valType;
  pair<map<SDB_chanKey, SDB_chanDesc>::iterator, bool> pin;
  SDB_chanDesc sd;

  vector<string> vstring = vsplit(sinput);
  if (vstring.size() < 6) return false;
  string long_desc = "";
  if (vstring.size() >= 7) {
    for (UInt_t i = 6; i < vstring.size(); i++) {
      long_desc = long_desc + vstring[i] + " ";
    }
  }
  string sdesc = vstring[0];
  Int_t helicity = atoi(vstring[1].c_str());
  Int_t crate = atoi(vstring[2].c_str());
  Int_t slot = atoi(vstring[3].c_str());
  Int_t first_chan = atoi(vstring[4].c_str());
  Int_t nchan = atoi(vstring[5].c_str());

  Namekey_t cs = make_pair(make_pair(crate, slot), first_chan);
  map<Namekey_t, vector<string> >::iterator ci = channame.find(cs);
  if( ci == channame.end() ) {
    vector<string> cnewstr;
    cnewstr.clear();
    cnewstr.push_back(sdesc);
    channame.insert(make_pair(make_pair(make_pair(crate, slot), first_chan),cnewstr));
  } else {
    // Add sdesc to the name list of existing map entry
    ci->second.push_back(sdesc);
  }
  SDB_chanKey sk(crate, helicity, sdesc);
  map<SDB_chanKey, SDB_chanDesc>::iterator pm = chanmap.find(sk);
  if (pm != chanmap.end()) {  // key already exists
    sd = pm->second;
    sd.LoadNextChan(first_chan, nchan);
  } else {   // new map entry
    SDB_chanDesc sd_new(slot, long_desc);
    sd_new.LoadNextChan(first_chan, nchan);
    pin = chanmap.insert(valType(sk, sd_new));
    return pin.second;
  }
  return true;
}

Bool_t THaScalerDB::LoadDirective(const string& sinput)
{
  if (!direct) return false;
  vector<string> vstring = vsplit(sinput);
  if (vstring.size() < 3) return false;
  string sname = vstring[0];
  Int_t crate = CrateToInt(vstring[1]);
  vector<string> sdir;
  string sword,sloadword;
  string::size_type pos1,pos2;
  string::size_type index = 2;
  while (index < vstring.size()) {
    sword = vstring[index];
    index++;
    sloadword = sword;
    pos1 = sword.find("'",0);
    if (pos1 != string::npos) {
      sloadword = sword.substr(0,pos1) + sword.substr(pos1+1,sword.length());
      pos2 = sloadword.find("'",0);
      if (pos2 != string::npos) {
        sloadword = sloadword.substr(pos2,sloadword.length());
        goto load1;
      }
      for (string::size_type j = index; j < vstring.size(); j++) {
        index++;
        sword = vstring[j];
        pos2 = sword.find("'",0);
        if (pos2 != string::npos) {
	  sloadword = sloadword + " " + sword.substr(0,pos2);
          goto load1;
	} else {
          sloadword = sloadword + " " + sword;
	}
      }
    }
load1:
    sdir.push_back(sloadword);
  }
  if (direct) direct->Load(sname, crate, sdir);
  return true;
}

vector<string> THaScalerDB::GetShortNames(Int_t crate, Int_t slot, Int_t chan) const
{
  Int_t lcrate = TiedCrate(crate,0);
  Int_t slot0 = GetSlot(lcrate, "TS-accept", 0);
  Int_t slotm = GetSlot(lcrate, "TS-accept", -1);
  Int_t slotp = GetSlot(lcrate, "TS-accept", 1);
  if ( slot == slotm && IsHelicityTied(lcrate, -1) ) slot = slot0;
  if ( slot == slotp && IsHelicityTied(lcrate,  1) ) slot = slot0;
  Namekey_t cs = make_pair(make_pair(lcrate, slot), chan);
  map< Namekey_t, vector<string> >::const_iterator ci = channame.find(cs);
  if( ci == channame.end()) {
    vector<string> null_result;
    null_result.push_back("none");
    return null_result;
  }
  return ci->second;
}

string THaScalerDB::GetStringDirectives( Int_t crate, const string& directive,
					 const string& key ) const
{
  if (!direct) return "";
  return direct->GetDirective(crate, directive, key);
}

Int_t THaScalerDB::GetNumDirectives( Int_t crate, const string& directive ) const
{
  if (!direct) return 0;
  return direct->GetDirectiveSize(crate, directive);
}

Int_t THaScalerDB::GetIntDirectives( Int_t crate, const string& directive,
				     const string& key ) const
{
  if (!direct) return 0;
  string sdir = direct->GetDirective(crate, directive, key);
  return atoi(sdir.c_str());
}

void THaScalerDB::LoadCrateToInt( const char *bank, Int_t icr )
{
  // Load the correspondence of crate string to integer
  string scr(bank);
  string scrate = "";
  for (string::const_iterator p = scr.begin(); p != scr.end(); ++p) {
    scrate += tolower(*p);
  }
  crate_strtoi.insert(make_pair(scrate, icr));
}

Int_t THaScalerDB::CrateToInt(const string& scr) const
{
  // Find integer representation of crate "scrate".
  string scrate = "";
  for (string::const_iterator p = scr.begin(); p != scr.end(); ++p) {
    scrate += tolower(*p);
  }
  map<string, Int_t>::const_iterator si = crate_strtoi.find(scrate);
  if( si == crate_strtoi.end() )
    return 0;
  return si->second;
}

Bool_t THaScalerDB::UsesTargetState(Int_t crate) const
{
  // Does this crate have target state info ?
  if (!direct) return kFALSE;
  return (direct->GetDirective(crate, "target-beam", "pp") != "none");
}


// The following 3 routines involve tying one combination of
// (crate, helicity) to another crate's helicity=0 data with a
// different slot#.  What is "tied" is the scaler.map data to
// avoid long & repetitive file.  Example: crate 11 and 8 are
// the same map, and helicities -1 & +1 are tied to 0.

Bool_t THaScalerDB::IsHelicityTied(Int_t crate, Int_t helicity) const
{
  // Does this (crate, helicity) get tied to a helicity=0 slot ?
  if (!direct) return kFALSE;
  return (direct->GetDirective(crate, "slot-offset", helicity) != "none");
}

Int_t THaScalerDB::TiedCrate(Int_t crate, Int_t helicity) const
{
  // Find the crate that this (crate, helicity) is tied to.
  if (!direct) return crate;
  string sdir = direct->GetDirective(crate, "crate-tied", helicity);
  if (sdir == "none") return crate;
  return atoi(sdir.c_str());
}

Int_t THaScalerDB::GetSlotOffset(Int_t crate, Int_t helicity) const {
// Find the slot offset relative to helicity=0 data.
  if (!direct) return 0;
  string sdir = direct->GetDirective(crate, "slot-offset", helicity);
  if (sdir == "none") return 0;
  return atoi(sdir.c_str());
}

string::size_type THaScalerDB::FindNoCase( const string& sdata,
					   const string& skey ) const
{
  // Find iterator of word "sdata" where "skey" starts.  Case insensitive.
  string sdatalc, skeylc;
  sdatalc = "";  skeylc = "";
  for (string::const_iterator p = sdata.begin(); p != sdata.end(); ++p) {
    sdatalc += tolower(*p);
  }
  for (string::const_iterator p = skey.begin(); p != skey.end(); ++p) {
    skeylc += tolower(*p);
  }
  return sdatalc.find(skeylc,0);
}

vector<string> THaScalerDB::vsplit(const string& s) const
{
  // split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type string_size;
  string_size i = 0;
  while( i != s.size() ) {
    while( i != s.size() && isspace(s[i]) ) ++i;
    string_size j = i;
    while (j != s.size() && !isspace(s[j])) ++j;
    if (i != j) {
      ret.push_back(s.substr(i, j-i));
      i = j;
    }
  }
  return ret;
};

ClassImp(THaScalerDB)
