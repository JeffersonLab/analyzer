#ifndef THaScalerDB_
#define THaScalerDB_

/////////////////////////////////////////////////////////////////////
//
//   THaScalerDB
//
//   Scaler database.  See implementation for comments.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <iostream>
#include <cstdlib>

class Bdate {
  // Utility class of date information.
public:
  Int_t day,month,year;
  Bdate(Int_t d=0, Int_t m=0, Int_t y=0) : day(d), month(m), year(y) { }
  ~Bdate() { }
  void load(Int_t d, Int_t m, Int_t y) {
    day = d;  month = m;  year = y;
  }
  void load(std::vector<std::string> strvect) {
    day   = atoi(strvect[1].c_str());
    month = atoi(strvect[2].c_str());
    year  = atoi(strvect[3].c_str());
  }
  void Print() const {
    std::cout << "\nBdate :  day = "<<day;
    std::cout << "  month = "<<month<<"   year = "<<year<<std::endl;
  }
  Bdate(const Bdate& rhs) {
    day = rhs.day; month = rhs.month; year = rhs.year;
  }
  Bdate &operator=(const Bdate &rhs) {
    if ( &rhs != this ) {
      day = rhs.day; month = rhs.month; year = rhs.year;
    }
    return *this;
  }
  friend bool operator==(const Bdate& a, const Bdate& b) {
    return a.day==b.day && a.month==b.month && a.year==b.year;
  }
  friend bool operator<(const Bdate& a, const Bdate& b) {
    if (a.year < b.year) return true;
    if (a.year > b.year) return false;
    if (a.month < b.month) return true;
    if (a.month > b.month) return false;
    if (a.day < b.day) return true;
    return false;
  }
  friend bool operator<=(const Bdate& a, const Bdate& b) {
    if (a.year < b.year) return true;
    if (a.year > b.year) return false;
    if (a.month < b.month) return true;
    if (a.month > b.month) return false;
    if (a.day <= b.day) return true;
    return false;
  }
};

class SDB_chanKey {
  // Utility class to hold channel description
  // Used as the key for maps
public:
  SDB_chanKey(Int_t cr, Int_t hel, std::string desc):crate(cr),helicity(hel),description(desc){ };
  ~SDB_chanKey() { }
  void Print() const {
    std::cout << "\nSDB_chanKey:  crate = "<<crate;
    std::cout << "  hel = "<<helicity<<"   desc = "<<description<<std::endl;
  }
  SDB_chanKey(const SDB_chanKey& rhs) : description(rhs.description) {
    crate = rhs.crate;
    helicity = rhs.helicity;
  };
  SDB_chanKey &operator=(const SDB_chanKey &rhs) {
    if ( &rhs != this ) {
      crate = rhs.crate;
      helicity = rhs.helicity;
      description = rhs.description;
    }
    return *this;
  }
  friend bool operator==(const SDB_chanKey& a, const SDB_chanKey& b) {
    return (a.crate==b.crate && a.helicity==b.helicity && a.description==b.description);
  }
  friend bool operator<(const SDB_chanKey& a, const SDB_chanKey& b) {
    if (a == b) return false;
    if (a.crate == b.crate && a.helicity == b.helicity) return (a.description < b.description);
    if (a.crate == b.crate) return (a.helicity < b.helicity);
    return (a.crate < b.crate);
  }
private:
  Int_t crate, helicity;
  std::string description;
};

class SDB_chanDesc {
  // Utility class to hold description of a channel
public:
  SDB_chanDesc(Int_t sl=0, std::string desc=""):slot(sl),description(desc) { };
  ~SDB_chanDesc() { }
  SDB_chanDesc(const SDB_chanDesc& rhs)
    : slot(rhs.slot), scalerchan(rhs.scalerchan), description(rhs.description) {
  }
  SDB_chanDesc &operator=(const SDB_chanDesc &rhs) {
    if ( &rhs != this ) {
      slot = rhs.slot;
      description = rhs.description;
      scalerchan = rhs.scalerchan;
    }
    return *this;
  }
  void Print() const {
    std::cout << "\nSDB_chanDesc:  slot = "<<slot;
    std::cout << "   desc = "<<description<<std::endl;
    std::cout << "num of chan = "<<scalerchan.size()<<std::endl;
    for (UInt_t i = 0; i < scalerchan.size(); i++) {
      std::cout << "scalerchan["<<i<<"] = "<<scalerchan[i]<<std::endl;
    }
  }
  void LoadNextChan(Int_t start, Int_t num) {
    for (Int_t i = start; i < start+num; i++) {
      scalerchan.push_back(i);
    }
  }
  Int_t GetSlot() const { return slot; };
  std::string GetDesc() const { return description; };
  Int_t GetChan(Int_t ichan) const {
    if (ichan >= 0 && ichan < (Int_t)scalerchan.size())
      return scalerchan[ichan];
    return 0;
  }
private:
  Int_t slot;
  std::vector<Int_t> scalerchan;
  std::string description;
};

class SDB_directive {
  // Utility class to hold directives
public:
  SDB_directive() { };
  ~SDB_directive() { }
  std::string GetDirective(Int_t crate, const std::string& key, Int_t isubkey) {
    char ckey[50];
    sprintf(ckey,"%d",isubkey);
    std::string skey(ckey);
    return GetDirective(crate, key, skey);
  };
  std::string GetDirective(Int_t crate, const std::string& key, const std::string& subkey) {
    std::string none = "none";
    std::pair<Int_t, std::string> pkey = make_pair(crate, key);
    std::map<std::pair<Int_t, std::string>, std::map<std::string, std::string> >::iterator pm = directives.find(pkey);
    if (pm == directives.end()) return none;
    std::map<std::string, std::string> stemp = pm->second;
    std::map<std::string, std::string>::iterator ps = stemp.find(subkey);
    if (ps == stemp.end()) return none;
    return ps->second;
  };
  Int_t GetDirectiveSize(Int_t crate, const std::string& key) {
    std::pair<Int_t, std::string> pkey = make_pair(crate, key);
    std::map<std::pair<Int_t, std::string>, std::map<std::string, std::string> >::iterator pm = directives.find(pkey);
    if (pm == directives.end()) return 0;
    std::map<std::string, std::string> stemp = pm->second;
    return stemp.size();
  }
  void Load(std::string key, Int_t crate, std::vector<std::string>& direct) {
    std::map<std::string, std::string> stemp;
    bool ok = false;
    for (std::vector<std::string>::iterator str = direct.begin(); str != direct.end(); ++str) {
      std::string sdir = *str;
      if (ParseDir(sdir)) {
	stemp.insert(make_pair(fSkey, fSdata));
        ok = true;
      }
    }
    if (ok) {
      std::pair<Int_t, std::string> pkey = make_pair(crate, key);
      std::map<std::pair<Int_t, std::string>, std::map<std::string, std::string> >::iterator pm = directives.find(pkey);
      if (pm != directives.end()) {
	(pm->second).insert(make_pair(fSkey, fSdata));
      } else {
	directives.insert(make_pair(make_pair(crate,key), stemp));
      }
    }
  };
  void Print() const {
    std::cout << std::endl << " -- Directives -- "<<std::endl;
    for (std::map<std::pair<Int_t, std::string>,
	   std::map<std::string, std::string> >::const_iterator dm = directives.begin();
    dm != directives.end(); ++dm) {
      std::pair<Int_t, std::string> is = dm->first;
      std::cout << "key i = "<<is.first<<"    string = "<<is.second<<std::endl;
      std::map<std::string, std::string> ss = dm->second;
      for (std::map<std::string, std::string>::const_iterator dss = ss.begin();
	   dss != ss.end(); ++dss) {
	std::cout << "string map = "<<dss->first<<"  "<<dss->second<<std::endl;
      }
    }
  };
private:
  SDB_directive(const SDB_directive &sd);
  SDB_directive& operator=(const SDB_directive &sd);
  std::map<std::pair<Int_t, std::string>, std::map<std::string, std::string> > directives;
  bool ParseDir(std::string sdir) {
    fSkey  = "";
    fSdata = "";
    std::string::size_type pos1;
    pos1 = sdir.find(":");
    if (pos1 != std::string::npos) {
      fSkey.assign(sdir.substr(0,pos1));
      fSdata.assign(sdir.substr(pos1+1,sdir.length()));
    }
    return true;
  }
  std::string fSkey, fSdata;
};


class THaScalerDB  {
public:

  THaScalerDB();
  virtual ~THaScalerDB();
  bool extract_db(const Bdate& bdate);
  std::string GetLongDesc(Int_t crate, const std::string& desc, Int_t helicity=0) const;
  std::string::size_type FindNoCase(const std::string& s1,
				    const std::string& s2) const;
  Int_t GetSlot(Int_t crate, const std::string& desc, Int_t helicity=0) const;
  Int_t GetSlot(Int_t crate, Int_t tgtstate, Int_t helicity) const;
  Int_t GetChan(Int_t crate, const std::string& desc, Int_t helicity=0, Int_t chan=0) const;
  std::vector<std::string> GetShortNames(Int_t crate, Int_t slot, Int_t chan) const;
  Int_t GetNumDirectives(Int_t crate, const std::string& directive) const;
  Int_t GetIntDirectives(Int_t crate, const std::string& directive,
			 const std::string& key) const;
  std::string GetStringDirectives(Int_t crate, const std::string& directive,
				  const std::string& key) const;
  void LoadCrateToInt(const char *bank, Int_t cr);
  Bool_t UsesTargetState(Int_t crate) const;
  Int_t CrateToInt(const std::string& scrate) const;
  void PrintChanMap() const;
  void PrintDirectives() const;
  Int_t SetDebug( Int_t level );

private:

  // key for channame map
  typedef std::pair<std::pair<Int_t, Int_t>, Int_t> Namekey_t;

  THaScalerDB(const THaScalerDB &bk);
  THaScalerDB& operator=(const THaScalerDB &bk);
  Bool_t LoadMap(const std::string& sinput);
  Bool_t LoadDirective(const std::string& sinput);
  std::string GetLineType(const std::string& sline) const;
  void Init();
  SDB_chanDesc GetChanDesc(Int_t crate, const std::string& desc, Int_t helicity=0) const;
  Bool_t IsHelicityTied(Int_t crate, Int_t helicity) const;
  Int_t TiedCrate(Int_t crate, Int_t helicity) const;
  Int_t GetSlotOffset(Int_t crate, Int_t helicity) const;
  std::vector<std::string> vsplit(const std::string& s) const;

  Bool_t found_date;
  std::string::size_type fgnfar;
  std::vector<std::string> directnames;
  std::map< SDB_chanKey, SDB_chanDesc > chanmap;
  std::map< std::string, Int_t > crate_strtoi;
  std::map< Namekey_t, std::vector< std::string> > channame;
  SDB_directive *direct;
  Int_t fDebug;

  ClassDef(THaScalerDB,0)  // Text-based time-dependent database for scaler map and directives

};

#endif
