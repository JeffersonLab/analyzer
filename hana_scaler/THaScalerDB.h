#ifndef THaScalerDB_
#define THaScalerDB_


/////////////////////////////////////////////////////////////////////
//
//   THaScalerDB
//
//   A text-based time-dependent database needed for scaler
//   crate maps.  In addition to class THaScalerDB, we put
//   here a couple small utility classes which hold date and
//   crate information.
//   If an MYSQL database becomes available, it can replace
//   this code.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

//#include "Rtypes.h"
#include "TObject.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#define ADB_DEBUG 0

class Bdate {
// A utility class of date information.
public:
  Bdate(int d, int m, int y) : day(d), month(m), year(y) { }
  Bdate() : day(0), month(0), year(0) { }
  ~Bdate() { }
  void load(int d, int m, int y) {
     day = d;  month = m;  year = y;
  };
  int day,month,year;
  void load(int i, int data) {  // load data by index
    switch (i) {
      case 0:
  	  day   = data;   break;
      case 1: 
          month = data;   break;
      case 2:
          year  = data;   break;
      default:
          break;
    }
  }
  void Print() {cout << "(D,M,Y) = "<<day<<" "<<month<<" "<<year<<endl;};
  friend bool operator==(Bdate a, Bdate b) {
    return a.day==b.day && a.month==b.month && a.year==b.year;
  };
  friend bool operator<(const Bdate a, const Bdate b) {  
    if (a.year < b.year) return true;
    if (a.year > b.year) return false;
    if (a.month < b.month) return true;
    if (a.month > b.month) return false;
    if (a.day < b.day) return true;
    return false;
  };    
  friend bool operator<=(const Bdate a, const Bdate b) {  
    if (a.year < b.year) return true;
    if (a.year > b.year) return false;
    if (a.month < b.month) return true;
    if (a.month > b.month) return false;
    if (a.day <= b.day) return true;
    return false;
  };
};

class BscaLoc {
// A utility class to hold scaler location info
#define DB_NINT 6  // num data stored here, plus 1 
private:
  int validbits;
public:
  BscaLoc() { clear(); };
  ~BscaLoc() { }
  int nint() { return DB_NINT; };
  bool first;
  int helicity,crate,slot;
#ifndef __CINT__
  vector<int> startchan,numchan;
  string short_desc;
  string long_desc;
#endif
  bool valid() {   // This object is valid if properly loaded.
    for (int i = 0; i < DB_NINT-1; i++) {
       if ( !(validbits&(1<<i)) ) return false;
    } 
    return true;
  }
  void setvalid() {
    for(int i = 0; i < DB_NINT-1; i++) validbits |= (1<<i); 
  };
  void clear() {
    validbits = 0; startchan.clear(); numchan.clear();
  };
  void load(int i, int data) {  // load data by index
    if (i >= 0 && i < DB_NINT-1) validbits |= (1<<i);
    switch (i) {
      case 0:
  	  helicity = data; break;
      case 1:
  	  crate = data; break;
      case 2: 
          slot = data; break;
      case 3:
	if (startchan.size() == 0) {
           startchan.push_back(data);
        } else {
	   startchan[0] = data; 
	}
        break;
      case 4:
	if (numchan.size() == 0) {
           numchan.push_back(data);
        } else {
	   numchan[0] = data; 
	}
        break;
      default:
          break;
    }
  };
  friend bool operator==(BscaLoc a, BscaLoc b) {
    return (a.short_desc==b.short_desc && a.crate==b.crate &&
            a.slot==b.slot);
  };
};

class THaScalerDB 
{
public:

   THaScalerDB();
   virtual ~THaScalerDB();
   bool extract_db(const Bdate& bdate, multimap< string, BscaLoc >& bmap);

private:

   bool found_date;
   void compress_bslvect(vector<BscaLoc>& bslvec);
   bool insert_map(Bdate& bdate, vector<BscaLoc>& bslvec, map<Bdate, vector<BscaLoc> >& bdloc);
   void print_dmap(map<Bdate, vector<BscaLoc> >& bdloc);
   vector<string> vsplit(const string& s);
   void print_bmap(multimap<string, BscaLoc >& bmap);

#ifndef ROOTPRE3
ClassDef(THaScalerDB,0)  // Text-based time-dependent database for scaler locations.
#endif


};

#endif




















