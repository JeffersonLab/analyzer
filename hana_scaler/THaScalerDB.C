/////////////////////////////////////////////////////////////////////
//
//   THaScalerDB
//
//   A text-based time-dependent database needed for scalers.
//   In addition to class THaScalerDB, we have a couple small 
//   utility classes which hold date and crate information.
//
//   If an MYSQL database becomes available, it can replace
//   this one.
//
//   Notes to casual users:  If this 'alpha' version works, all 
//   you need to know is about the main class THaScaler and the 
//   database file scaler.map, which are quite transparent.  
//   But if this class 'breaks', complain to me, I'll fix it.
//   One may imagine a user's typo errors in scaler.map causing
//   problems.  The 'pattern recognition' of scaler.map is not 
//   fool-proof.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaScalerDB.h"
#include <errno.h>

using namespace std;

THaScalerDB::THaScalerDB() { }
THaScalerDB::~THaScalerDB() { }

bool THaScalerDB::extract_db(const Bdate& bdate, multimap< string, BscaLoc >& bmap) {
// Input: Bdate = time (day,month,year) when database is valid
// Output: BscaLoc = scaler map (loaded by this function).
// Return : true if successful, else false.
// Search order for scaler.map:
//   $DB_DIR/DEFAULT $DB_DIR ./DB/DEFAULT ./DB ./db/DEFAULT ./db DEFAULT .
  const string scalmap("scaler.map");
  char* dbdir=getenv("DB_DIR");
  int ndir = 6;
  if( dbdir ) ndir += 2;
  vector<string> fname(ndir);
  int i = 0;
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
  fname[i++] = "DEFAULT/" + scalmap;
  i = 0;
  ifstream mapfile;
  do {
    mapfile.open(fname[i].c_str());
  } while( (! mapfile.is_open() ) && (++i)<ndir );
  if (! mapfile.is_open() ) {
     cerr << "ERROR: THaScalerDB: scaler.map file does not exist !!"<<endl;
     cerr << "You must have scaler.map in present working directory,"<<endl;
     cerr << "or in $DB_DIR directory." << endl;
     cerr << "Download  http://hallaweb.jlab.org/adaq/scaler.map"<<endl;
     cerr << "    or"<<endl;
     cerr << "jlabs1:/group/halla/www/hallaweb/html/adaq/scaler.map"<<endl;
     return false;
  } else {
    cout << "Opened scaler map file " << fname[i] << endl;
  }
   bmap.clear();
   const char key[]="DATE";
   string comment = "#";
   Bdate bd;
   vector<Bdate> tm;
   BscaLoc bloc;
   map < Bdate, vector<BscaLoc> > bdloc;
   vector<BscaLoc> bslvec;
   //   pair < string, BscaLoc > strlocpair;
   typedef multimap< string, BscaLoc >::value_type valType;
   int j,k;
   string sinput;
   vector<string> strvect;
   bdloc.clear();
   bslvec.clear();
   bool newdate = false;
   while (getline(mapfile,sinput)) {
      j = sinput.find(key,0);
      strvect.clear();
      strvect = vsplit(sinput);   
      if (j >= 0) {     
        if (newdate) {
           if ( !insert_map(bd,bslvec,bdloc) ) return false;
        }
        bslvec.clear();
        j = k = 0;
        for (vector<string>::iterator p = strvect.begin(); 
            p != strvect.end(); p++) 
            if (j++ > 0) bd.load(k++,atoi(p->c_str()));
        tm.push_back(bd);
        newdate = true;
      } else {
        if ((long)strvect.size() >= bloc.nint() && strvect[0] != comment) {
           bloc.clear();
           bloc.short_desc = strvect[0];
           string slong = strvect[bloc.nint()];
           for (j = bloc.nint()+1; j < (long)strvect.size(); j++) slong += " "+strvect[j];
           bloc.long_desc = slong;
           j = k = 0;
           for (vector<string>::iterator p = strvect.begin(); 
      	      p != strvect.end(); p++) {
               if (j++ > 0) bloc.load(k++,atoi(p->c_str()));
           }
           bslvec.push_back(bloc);
	}        
      }
   }
   if ( !insert_map(bd,bslvec,bdloc) ) return false;
   if (ADB_DEBUG) print_dmap(bdloc);
   Bdate dmax(1,1,1990);   
   for (vector<Bdate>::iterator p = tm.begin();
      p != tm.end(); p++) {
         if ( (*p <= bdate) && (dmax <= *p) ) {
            found_date = true;
            dmax = *p;
         }
   }
   if(ADB_DEBUG) cout << "date found y,m,d "<<dmax.year<<" "<<dmax.month<<" "<<dmax.day<<endl;
   if (!found_date) {
      cout << "Warning: THaScalerDB: Did not find data in database"<<endl;
      return false;
   }
   map< Bdate, vector<BscaLoc> >::iterator pm = bdloc.find(dmax);
     if (pm != bdloc.end()) {
        bslvec = pm->second;
        compress_bslvect(bslvec);
        for (vector<BscaLoc>::iterator p = bslvec.begin();
             p != bslvec.end(); p++) {
	  //           strlocpair.first = p->short_desc;
	  //           strlocpair.second = *p;      
           bmap.insert(valType(p->short_desc,*p));
        }
     } else {
        return false;
     }
     if (ADB_DEBUG) print_bmap(bmap);
     return true;
};


bool THaScalerDB::insert_map(Bdate& bdate, vector<BscaLoc>& bslvec, map<Bdate, vector<BscaLoc> >& bdloc) {
// Insert an element into the map bdloc
  typedef map<Bdate, vector<BscaLoc> >::value_type valType;
  //  pair < Bdate, vector<BscaLoc> > bdlocpair;
  //  bdlocpair.first = bdate;
  //  bdlocpair.second = bslvec;
  pair<map<Bdate,vector<BscaLoc> >::iterator,bool> p = 
    bdloc.insert(valType(bdate,bslvec));
    //bdlocpair);
  return p.second; 
}

void THaScalerDB::print_dmap(map<Bdate, vector<BscaLoc> >& bdloc) {
// Print out the map between dates and scaler locations.
   int j = 0;
   cout << "Map between dates and scaler loc, size = "<<bdloc.size()<<endl;
   for (map< Bdate, vector<BscaLoc> >::iterator pm = bdloc.begin();
     pm != bdloc.end() ;  pm++) {
       Bdate bd = pm->first;
       vector<BscaLoc> bslvec = pm->second;
       j++;
       cout << "---- Element "<<j<<endl;
       cout << "Bdate "<<bd.year<<" "<<bd.month<<" "<<bd.day<<endl;
       for (vector<BscaLoc>::iterator p = bslvec.begin(); 
         p != bslvec.end(); p++) {
         cout << "short descr "<<p->short_desc<<endl;
           for (int k = 0; k < (long)p->startchan.size(); k++) {
	     cout << "   start channel "<<p->startchan[k];
             cout << "   numchan "<<p->numchan[k]<<endl;
           }
       }
   }
};

void THaScalerDB::print_bmap(multimap<string, BscaLoc >& bmap) {
// Print out the map between names and scaler locations.
   int j = 0;
   cout << "Map between names and scaler loc,  size = "<<bmap.size()<<endl;
   for (multimap< string, BscaLoc >::iterator pm = bmap.begin();
     pm != bmap.end() ;  pm++) {
       cout << "Element "<<j++;
       cout << "  Name = "<<pm->first<<endl;
       BscaLoc bloc = pm->second;
       cout << "BscaLoc: short descr "<<bloc.short_desc<<endl;
       cout << "  long desc = "<<bloc.long_desc<<endl;
       cout << "  crate = "<<bloc.crate<<endl;
       cout << "  slot = "<<bloc.slot<<endl;
       cout << "  helicity = "<<bloc.helicity<<endl;
       cout << "  size of startchan, numchan "<<bloc.startchan.size()<<endl;
       for (int k = 0; k < (long)bloc.startchan.size(); k++) {
	   cout << "   start channel "<<bloc.startchan[k];
           cout << "   numchan "<<bloc.numchan[k]<<endl;
       }
   }
};

void THaScalerDB::compress_bslvect(vector<BscaLoc>& bslvec) {
// Compress vector of scaler locations to handle repeated scaler.map entries
  int i,j,k;
  vector<BscaLoc> duplicate; 
  vector<int> delpt;   
  for (i = 0; i < (long)bslvec.size()-1; i++) {
    for (j = i+1; j < (long)bslvec.size(); j++) {  // loop over all pairs
      if (bslvec[i] == bslvec[j]) {          // identical channel
        int dupl = 0;
        for (vector<BscaLoc>::iterator pm = duplicate.begin(); 
	pm != duplicate.end(); pm++) {  // see if already a duplicate
           if (*pm == bslvec[i]) {
  	      pm->startchan.push_back(bslvec[i].startchan[0]);
              pm->numchan.push_back(bslvec[i].numchan[0]);
  	      pm->startchan.push_back(bslvec[j].startchan[0]);
              pm->numchan.push_back(bslvec[j].numchan[0]);
              dupl = 1;
	   } 
	}
        if (dupl) continue;
        bslvec[i].startchan.push_back(bslvec[j].startchan[0]);
        bslvec[i].numchan.push_back(bslvec[j].numchan[0]);
	duplicate.push_back(bslvec[i]);
        delpt.push_back(i);  
        delpt.push_back(j);
      }
    }
  }
  k = 0;
  vector<BscaLoc> bslcopy;
  for (vector<BscaLoc>::iterator p = bslvec.begin(); 
    p != bslvec.end(); p++) {
    int del = 0;
    for (vector<int>::iterator pm = delpt.begin();
       pm != delpt.end(); pm++) {
         if (*pm == k) del = 1;
    }
    k++;
    if (!del) bslcopy.push_back(*p);
  }
  for (vector<BscaLoc>::iterator p = duplicate.begin(); 
     p != duplicate.end(); p++) bslcopy.push_back(*p);
  bslvec = bslcopy;
};

vector<string> THaScalerDB::vsplit(const string& s) {
// split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type string_size;
  string_size i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      string_size j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
};

#ifndef ROOTPRE3
ClassImp(THaScalerDB)
#endif
