//*-- Author :    Robert Feuerbach   02-July-2003

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
//
//  Interface to a database stored in key-value format textfile(s).
//
//  Date/time-based granularity is implemented, though clumsy in the
//  file at the moment.
//
//  The data are organized by "system" (e.g. a detector or spectrometer) 
//  and "attribute" (actual names of data fields).  Data can be 
//  integers, doubles, strings, arrays of integers or doubles, or
//  two-dimensional arrays (matrix) or integers or doubles.
//
//  Data for each system is expected to be stored in an individual file 
//  with name "db_<system>.dat".  Inside this file,
//  the data are simple key-value pairs.  The files are expected to be located
//  in the "database directory".  This directory is either the one specified
//  in a call to SetDBDir() or the value of the environment variable DB_DIR.  
//  If neither is available, ./DB ./db and ./ (current directory) are tried.
//  
//  Time dependence of the data can be implemented in two ways: database files 
//  can be stored in subdirectories of the database directory whose names
//  represent a time stamp of the form YYYYMMDD.  This allows a granularity
//  of one day. 
//
//  In addition or instead of the directory-based method, database files 
//  may contain date tags of form
//  -----[ YYYY-MM-DD HH:MM:SS ]-----
//  Such tags must be ordered by ascending time. Everything following a 
//  date tag is considered valid starting with the indicated time up until
//  the date of the next tag. If there is no next tag, then the validity
//  never expires.
//
//  A subdirectory named "DEFAULT" and/or the database directory itself
//  can be used to store files that do not change as often as the
//  time stamps of the subdirectories.  Of course, these files may
//  contain date tags as described above.
//
//  For quick tests, dabatase files may be put in the current directory, which
//  is always searched first before looking in the database directory and its
//  subdirectories.
//
//  In addition to (or instead of) a single file per "system", there can be
//  a "master database file". Entries not found in single files are
//  searched for in the master file.  The database direcory, subdirectory,
//  and date tag logic described above applies to the master file as well,
//  so users may choose, for example, bewteen smaller master files distributed 
//  over several subdirectories and a single file stored in the database 
//  directory with no subdirectories at all.  Different experiments may
//  choose different master files, e.g. $DB_DIR/e01001.db, $DB_DIR/e05012, etc.
//  The default master file name is "default.db", but can be overridden with a 
//  call to SetInFile().
//
//  Within the master file, the keys are of format "system.attribute"
//  followed by the corresponding value(s).
//
//  The master file is an extension of the older "run database" concept.
//  THaDB does not support a special run database; instead run-specific
//  data should be stored like any other database entries.  For backwards
//  compatibility, however, THaFileDB can be configured to search run.db
//  files as well if entries are not found in either single files or the master
//  file. To enable this mode, call EnableRunDBSearch().
//
//////////////////////////////////////////////////////////////////////////

#include "THaFileDB.h"
#include "TDatime.h"
#include "TError.h"
#include "TSystem.h"
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <iterator>
#include <cctype>

using namespace std;

// for compatibility since strstream is being depreciated,
// but not all platforms have stringstreams yet
#ifdef HAS_SSTREAM
#include <sstream>
#define ISSTREAM istringstream
#define OSSTREAM ostringstream
#define ASSIGN_SSTREAM(a,b) a=b.str()
#else
#include <strstream>
#define ISSTREAM istrstream
#define OSSTREAM ostrstream
#define ASSIGN_SSTREAM(a,b)  b<<'\0'; a=b.str()
#endif

// the old GNUC has a funny version of string::compare
// egcs (GNUC old) has a non-standard compare method
#if defined(__GNUC__) && (__GNUC__ <= 2)
  #define STR_COMPARE(f,l,s) compare(s,f,l)
#else
  #define STR_COMPARE(f,l,s) compare(f,l,s)
#endif

typedef string::size_type ssiz_t;

// Hide this DbString class so that others do not have access
//_____________________________________________________________________________
namespace {
  class DbString {
  public:
    std::string fString;
    DbString(const std::string& str="") : fString(str) { }
  };

  std::ostream& operator<<(std::ostream& to, const DbString& str) {
    // for a string, break it into lines, writing each line out
    // with a single 'tab' character in front.
    
    ssiz_t oldpos=0;
    ssiz_t pos=0;
    ssiz_t len=str.fString.length();
    
    while ( (pos = str.fString.find('\n',oldpos)),pos<=len ) {
      to<<'\t'<<str.fString.substr(oldpos,pos-oldpos+1);
      oldpos=pos+1;
    }
    if (oldpos<len) {
      to<<'\t'<<str.fString.substr(oldpos+1,len-oldpos) << endl;
    }
    return to;
  }
  
  std::istream& operator>>(std::istream& from, DbString& str) {
    // for a string, read in each line belonging, and remove the initial
    // 'tab' character
    str.fString.erase();
    
    std::string line;
    // strings must begin with a '\t'
    while( from.peek()=='\t' && getline(from,line).good() ) {
      str.fString += line.substr(1);
    }
    return from;
  }
  
}

//_____________________________________________________________________________
THaFileDB::THaFileDB( const char* infile )
  : fOutFileName("out.db"), fDebug(2), modified(0)
{
  fDescription = "";

  SetInFile( infile );
  db[0].good = db[0].tried = db[2].good = db[2].tried = false;
  db[2].system_name = "run";
}

//_____________________________________________________________________________
THaFileDB::~THaFileDB()
{
  if (modified)
    FlushDB();
}

//_____________________________________________________________________________
int THaFileDB::FlushDB()
{
  // Write contents of memory out to file
  ofstream to(fOutFileName.c_str());
  to << db[1].contents;
  if (db[1].contents.length()>0 && db[1].contents[db[1].contents.length()-1] != '\n')
    to << endl;
  modified = 0;
  
  return to.good();
}

//_____________________________________________________________________________
void THaFileDB::PrintDB() const
{
  // Print database contents
  cout << *this << endl;
}

//_____________________________________________________________________________
Int_t  THaFileDB::SetDBDir( const char* name )
{
  static const char* const here = "THaFileDB::SetDBDir";

  if( !name || *name == 0 ) {
    Error( here, "Invalid directory name" );
    return -1;
  }

  // Check if this directory exists, if not then warn user.
  void* dirp = gSystem->OpenDirectory( name );
  if( !dirp ) {
    Warning( here, "Requested database directory %s unavailable! "
	     "Directory not changed.", name );
    return -2;
  }
  gSystem->FreeDirectory( dirp );

  fDBDir = name;
  // FIXME: flush any modifications
  // FIXME: clear memory
  return 0;
}

//_____________________________________________________________________________
void  THaFileDB::SetOutFile( const char* name )
{
  if( name && *name )
     fOutFileName = name; 
}

//_____________________________________________________________________________
void  THaFileDB::SetInFile( const char* name )
{
  if( name && *name )
    db[1].system_name = name;
  else
    db[1].system_name.clear();

  db[1].contents.clear();
  db[1].good = db[1].tried = false;
}

//_____________________________________________________________________________
void  THaFileDB::SetDescription(const char* description)
{
  // Set the description to be written out with the next Write
  fDescription = description;
}

//_____________________________________________________________________________
bool THaFileDB::CopyDB(istream& in, ostream& out, streampos pos)
{
  // Copy, starting at the current position through position "pos"
  // from "in" to "out"
  int ch;
  while ( (ch=in.get())!=EOF && in.good() && (pos < 0 || in.tellg()<pos) ) {
    out.put(ch);
  }
  if ( !in.good() && !in.eof() ) {
    //    Warning("Copy","Something weird happened while copying...");
    return false;
  }
  return true;
}

//  READING A SINGLE VALUE
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::ReadValue( const char* systemC, const char* attr,
			     T& value, const TDatime& date)
{
  // Read in a single value

  // look through the file, looking for the latest date stamp
  // that is still before 'date'

  if( !LoadDB(systemC, date) )
    return 0;

  for( int i=0; i<3; i++ ) {
    if( !db[i].good ) continue;
    const char* system = ( i>0 ) ? systemC : "";
    TDatime dbdate(date);

    ISSTREAM from(db[i].contents.c_str());
    if ( from.good() && FindEntry(system,attr,from,dbdate) ) {
      // found an entry    
      from >> value;
      return 1;
    }
  }
  return 0;
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetValue( const char* system, const char* attr,
			    Int_t& value, const TDatime& date)
{
  // Read an Int_t for system/attribute
  return ReadValue(system,attr,value,date);
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetValue( const char* system, const char* attr,
			    Double_t& value, const TDatime& date)
{
  // Read an Double_t for system/attribute
  return ReadValue(system,attr,value,date);
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetValue( const char* system, const char* attr,
			    string& value, const TDatime& date)
{
  // Read a string for system/attribute

  DbString dbstr;
  Int_t ret = ReadValue(system,attr,dbstr,date);
  if( ret )
    value = dbstr.fString;
  return ret;
}


//  READING AN ARRAY INTO A VECTOR
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::ReadArray( const char* systemC, const char* attr,
			     vector<T>& array, const TDatime& date,
			     UInt_t size )
{
  // Read in a series of values, using the istream >> operator
  // They are returned in a vector, with the return value and the
  // array.size() equal to the number of constants read in.
  //
  // Since "array" may be resized, anticipate the possiblity of it moving. 
  // Also, for efficiency .reserve'ing the appropriate space for the array
  // before the call is recommended.

  if( !LoadDB(systemC, date) )
    return 0;

  if( size == 0 )
    size = array.max_size();

  for( int i=0; i<3; i++ ) {
    if( !db[i].good ) continue;
    const char* system = ( i>0 ) ? systemC : "";
    TDatime dbdate(date);
  
    UInt_t cnt=0;
    ISSTREAM from(db[i].contents.c_str());
    if ( from.good() && FindEntry(system,attr,from,dbdate) ) {
      while ( from.good() && find_constant(from) ) {
	if( cnt >= size ) {
	  //FIXME: print key and db source
	  Warning("ReadArray", "Array truncated from %d to %d elements",
		  size, cnt );
	  break;
	}
	T val;
	from >> val;
	array.push_back(val);
	cnt++;
      }
      //  array.resize(cnt); // only keep it as large as necessary
      return cnt;
    }
  }
  return 0;
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetArray( const char* system, const char* attr,
			    vector<Int_t>& array, const TDatime& date)
{
  // Read in a set of Int_t's into a vector.
  
  return ReadArray(system,attr,array,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::GetArray( const char* system, const char* attr,
			    vector<Double_t>& array, const TDatime& date)
{
  // Read in a set of Double_t's into a vector.
  
  return ReadArray(system,attr,array,date);
}


//  READING AN ARRAY INTO A C-style ARRAY
//_____________________________________________________________________________
UInt_t THaFileDB::GetArray( const char* system, const char* attr,
			    Int_t* array, UInt_t size, const TDatime& date)
{
  // Read in a set of Int_t's in to a C-style array.
  
  vector<Int_t> vect;
  UInt_t cnt = ReadArray(system,attr,vect,date,size);
  copy( vect.begin(), vect.begin()+cnt, array );
  return cnt;
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetArray( const char* system, const char* attr,
			    Double_t* array, UInt_t size, const TDatime& date)
{
  // Read in a set of Double_t's in to a C-style array.
  
  vector<Double_t> vect;
  UInt_t cnt = ReadArray(system,attr,vect,date,size);
  copy( vect.begin(), vect.begin()+cnt, array );
  return cnt;
}

//  READING A MATRIX INTO A two-dimensional vector
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::ReadMatrix( const char* systemC, const char* attr,
			      vector<vector<T> >& rows, const TDatime& date )
{
  // Reads in a matrix of values, broken by '\n'. Each line in the matrix
  // must begin with whitespace

  if( !LoadDB(systemC, date) )
    return 0;

  for( int i=0; i<3; i++ ) {
    if( !db[i].good ) continue;
    ISSTREAM from(db[i].contents.c_str());
    const char* system = ( i>0 ) ? systemC : "";
  
    // Clear out the matrix vectors, first
    //    typename vector<vector<T> >::size_type vi;
    // Make certain we are starting with a fresh matrix
//     for (vi = 0; vi<rows.size(); vi++) {
//       rows[vi].clear();
//     }
    rows.clear();
  
    // this differs from a 'normal' vector in that here, each line
    // gets its own row in the matrix
    TDatime dbdate(date);
    if ( FindEntry(system,attr,from,dbdate) ) {
      // Start collecting a new row
      while ( from.good() && find_constant(from) ) {
	// collect a row, breaking on a '\n'
	vector<T> v;
	while ( from.good() && find_constant(from,1) ) {
	  T tmp;
	  from >> tmp;
	  v.push_back(tmp);
	}
	if (v.size()>0)
	  rows.push_back(v);
      }
    }
    return rows.size();
  }
  return 0;
}

//_____________________________________________________________________________
UInt_t THaFileDB::GetMatrix( const char* system, const char* attr,
			    vector<vector<Int_t> >& matrix,
			    const TDatime& date)
{
  // Read in a set of Int_t's into a 2-d matrix
  
  return ReadMatrix(system,attr,matrix,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::GetMatrix( const char* system, const char* attr,
			    vector<vector<Double_t> >& matrix,
			    const TDatime& date)
{
  // Read in a set of Double_t's into a 2-d matrix
  
  return ReadMatrix(system,attr,matrix,date);
}


//_____________________________________________________________________________
UInt_t THaFileDB::GetMatrix( const char* system, const char* name,
			     vector<string>& mtr_name, 
			     vector<vector<Double_t> >& mtr_rows,
			     const TDatime &date )
{
  // Read in the transport matrix from the text-file database.
  // Return each entry as correlated vectors of string and floats
  //  
  // Read in the matrix as a group, all at once
  // 

  TDatime orig_date(date);

  ISSTREAM from(db[1].contents.c_str());

//    ifstream from(fInFileName.c_str());

  
  // Clear out the matrix vectors, first
  mtr_name.clear();
  
  //  vector<vector<Double_t> >::size_type vi;

  // Make certain we are starting with a fresh matrix
//   for (vi = 0; vi<mtr_rows.size(); vi++) {
//     mtr_rows[vi].clear();
//   }
  mtr_rows.clear();
  
  //FIXME:
  const char*  attr = "matrix";
  
  while ( from.good() && FindEntry(system,attr,from,orig_date) ) {
    vector<Double_t> v;
    while ( from.good() && find_constant(from) ) {
      Double_t tmp;
      from >> tmp;
      v.push_back(tmp);
    }
    
    if (v.size()>0) {
      mtr_name.push_back(attr);
      mtr_rows.push_back(v);
    }
  }
  return mtr_rows.size();
}
  
  

//_____________________________________________________________________________
void THaFileDB::WriteDate( ostream& to, const TDatime& date ) 
{
  // Write a line that states the date of the future entries

  to << '\n';
  to << "--------[ ";
  to << date.AsSQLString() << " ]\n" << endl;
}


//    WRITING A SINGLE VALUE TO THE FILE
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::WriteValue( const char* system, const char* attr,
			      const T& value, const TDatime& date)
{
  // Write out a single value, using the ostream << operator
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db[1].contents.c_str());
  OSSTREAM to;

  // Find the last location with a date earlier or equal to the current date
  SeekDate(from,fnd_date);

  // Copy over the old contents
  Int_t pos = from.tellg();
  from.clear();
  from.seekg(0,ios::beg);
  CopyDB(from,to,pos);
  
  if ( date.Get() - fnd_date.Get() > 10) { // ten seconds
    // Write a date entry
    WriteDate(to,date);
  }
  
  if (fDescription != "") {
    to << "# " << fDescription << endl;
  }

  // Write out system/attribute  
  to << system << " " << attr << ' ';
  
  ios::fmtflags o_flag = to.flags();
  int o_precision = to.precision();
  
  to.precision(6);
  to.unsetf(ios::floatfield);
  
  to << value;
  to << endl;
  
  to.precision(o_precision);
  to.flags(o_flag);

  // Copy over the rest of the database
  from.seekg(pos,ios::beg);
  CopyDB(from,to);
  if ( pos == from.tellg() ) // were already at the end of the file
    to << endl;              // add a newline

  ASSIGN_SSTREAM(db[1].contents,to);

  modified = 1;
  
  return 1;
}

//_____________________________________________________________________________
UInt_t THaFileDB::PutValue( const char* system, const char* attr,
			    const Int_t& value, const TDatime& date )
{
  // Save the single Int_t value, corresponding to system/attribute
  return WriteValue(system,attr,value,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::PutValue( const char* system, const char* attr,
			    const Double_t& value, const TDatime& date )
{
  // Save the single Double_t value, corresponding to system/attribute
  return WriteValue(system,attr,value,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::PutValue( const char* system, const char* attr,
			    const string& value, const TDatime& date )
{
  // Save the single string value, corresponding to system/attribute

  DbString dbstr(value);

  return WriteValue(system,attr,dbstr,date);
}
  
//    WRITING A VECTOR TO THE FILE
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::WriteArray( const char* system, const char* attr,
			      const vector<T>& v, const TDatime& date )
{
  // Write out an array, using the ostream << operator
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db[1].contents.c_str());
  OSSTREAM to;
  
  // Find the last location with a date (or equal to) the current date
  SeekDate(from,fnd_date);

  // Copy over the old contents
  Int_t pos = from.tellg();
  from.clear();
  from.seekg(0,ios::beg);

  CopyDB(from,to,pos);

  if ( date.Get() - fnd_date.Get() > 10) { // ten seconds
    // Write a date entry
    WriteDate(to,date);
  }

  if (fDescription != "") {
    to << "# " << fDescription << endl;
  }
  
  streampos oldpos = to.tellp();
  to << system << " " << attr << ' ';
  
  Int_t nwrit = 0;

  ios::fmtflags o_flag = to.flags();
  int o_precision = to.precision();
  
  to.precision(6);
  to.unsetf(ios::floatfield);
  
  typename vector<T>::size_type i;
  for (i = 0; i < v.size(); i++) {
    if ( to.tellp() - oldpos > 75 ) {
      oldpos = to.tellp();
      to << "\n\t";
    }
    to << ' ' << v[i];
    nwrit++;
  }
  to << endl;
  to.precision(o_precision);
  to.flags(o_flag);

  // Copy over the rest of the database
  from.seekg(pos,ios::beg);
  CopyDB(from,to);
  if ( pos == from.tellg() ) // were already at the end of the file
    to << endl;              // add a newline
  
  ASSIGN_SSTREAM(db[1].contents,to);

  modified = 1;
  
  return nwrit;
}

//_____________________________________________________________________________
UInt_t THaFileDB::PutArray( const char* system, const char* attr,
			   const vector<Int_t>& array, const TDatime& date )
{
  // Save the vector of Int_t 's corresponding to system/attribute
  return WriteArray(system,attr,array,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::PutArray( const char* system, const char* attr,
			   const vector<Double_t>& array, const TDatime& date )
{
  // Save the vector of Double_t 's, corresponding to system/attribute
  return WriteArray(system,attr,array,date);
}


//    WRITING A C-style array TO THE FILE
//_____________________________________________________________________________
UInt_t THaFileDB::PutArray( const char* system, const char* attr,
			    const Int_t* array, UInt_t size,
			    const TDatime& date )
{
  // Save the vector of Int_t 's corresponding to system/attribute
  vector<Int_t> vect;
  copy( array, array+size, vect.begin() );
  return WriteArray(system,attr,vect,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::PutArray( const char* system, const char* attr,
			    const Double_t* array, UInt_t size,
			    const TDatime& date )
{
  // Save the vector of Double_t 's, corresponding to system/attribute
  vector<Double_t> vect;
  copy( array, array+size, vect.begin() );
  return WriteArray(system,attr,vect,date);
}

#if 0
//_____________________________________________________________________________
template<class T>
UInt_t THaFileDB::WriteArray( const char* system, const char* attr,
			      const T* array, UInt_t size, const TDatime& date)
{

  // Write out the sequence of values to the database file. Can also
  // write out scalers (size = 0 or 1)

  if (size == 0) size = 1;
  
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db[1].contents.c_str());
  OSSTREAM to;

  // Find the last location with a date (or equal to) the current date
  SeekDate(from,fnd_date);

  // Copy over the old contents
  UInt_t pos = from.tellg();
  from.clear();
  from.seekg(0,ios::beg);
  CopyDB(from,to,pos);
  
  if ( date.Get() - fnd_date.Get() > 10) { // ten seconds
    // Write a date entry
    WriteDate(to,date);
  }
  
  if (fDescription != "") {
    to << "# " << fDescription << endl;
  }
  
  // Write out system/attribute  
  streampos oldpos = to.tellp();
  to << system << " " << attr << ' ';
  
  UInt_t nwrit = 0;

  ios::fmtflags o_flag = to.flags();
  int o_precision = to.precision();
  
  to.precision(6);
  to.unsetf(ios::floatfield);
  
  UInt_t i;
  for (i = 0; i < size; i++) {
    if ( to.tellp() - oldpos > 75 ) {
      oldpos = to.tellp();
      to << "\n\t";
    }
    to << ' ' << array[i];
    nwrit++;
  }
  to << endl;
  to.precision(o_precision);
  to.flags(o_flag);

  // Copy over the rest of the database
  from.seekg(pos,ios::beg);
  CopyDB(from,to);
  if ( pos == from.tellg() )
    to << endl;
  
  ASSIGN_SSTREAM(db[1].contents,to);

  modified = 1;

  return nwrit;
}
#endif

//    WRITE A MATRIX, BUILT OF VECTORS
//_____________________________________________________________________________
template <class T>
UInt_t THaFileDB::WriteMatrix( const char* system, const char* attr,
			       const vector<vector<T> >& matrix, const TDatime& date )
{
  // Write out a matrix of arbitrary type, just needs the ostream<< operator
  // prepared. Each line corresponds to a row in the matrix
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db[1].contents.c_str());
  OSSTREAM to;

  // Find the last location with a date (or equal to) the current date
  SeekDate(from,fnd_date);

  // Copy over the old contents
  streampos pos = from.tellg();
  from.clear();
  from.seekg(0,ios::beg);
  CopyDB(from,to,pos);
  
  if ( date.Get() - fnd_date.Get() > 10) { // ten seconds
    // Write a date entry
    WriteDate(to,date);
  }
  
  if (fDescription != "") {
    to << "# " << fDescription << endl;
  }
  
  // Write out system/attribute  
  to << system << " " << attr << '\n';
  
  UInt_t nwrit = 0;

  ios::fmtflags o_flag = to.flags();
  int o_precision = to.precision();
  
  to.precision(6);
  to.unsetf(ios::floatfield);

  typename vector<vector<T> >::size_type i;
  
  for (i = 0; i < matrix.size(); i++) {
    typename vector<T>::size_type j;
    const vector<T>& v=matrix[i];

    to << '\t';
    for (j=0; j<v.size(); j++) {
      to << ' ' << v[j];
    }
    to<<endl;
    nwrit++;
  }
  to.precision(o_precision);
  to.flags(o_flag);

  // Copy over the rest of the database
  from.seekg(pos,ios::beg);
  CopyDB(from,to);
  if ( pos == from.tellg() )
    to << endl;
  
  ASSIGN_SSTREAM(db[1].contents,to);

  modified = 1;
  
  return nwrit;
}

//_____________________________________________________________________________
UInt_t THaFileDB::PutMatrix( const char* system, const char* name,
			     const vector<vector<Double_t> >& rows,
			     const TDatime& date )
{
  // Write out a matrix of Double_t 's
  return WriteMatrix(system,name,rows,date);
}
//_____________________________________________________________________________
UInt_t THaFileDB::PutMatrix( const char* system, const char* name,
			     const vector<vector<Int_t> >& rows,
			     const TDatime& date )
{
  // Write out a matrix of Int_t 's
  return WriteMatrix(system,name,rows,date);
}

//_____________________________________________________________________________
UInt_t THaFileDB::PutMatrix( const char* system, const char* name,
			     const vector<string>& mtr_name, 
			     const vector<vector<Double_t> >& mtr_rows,
			     const TDatime& date )
{
  // Write out the transport matrix to the text-file database.
  // They are passed in as correlated vectors in mtr_name, mtr_rows

  TDatime fnd_date(date);

  ISSTREAM from(db[1].contents.c_str());
  OSSTREAM to;
  
  // Find the last location with a date (or equal to) the current date
  SeekDate(from,fnd_date);

  // Copy over the old contents
  Int_t pos = from.tellg();
  from.clear();
  from.seekg(0,ios::beg);
  CopyDB(from,to,pos);
  
  if ( date.Get() - fnd_date.Get() > 10) { // ten seconds
    // Write a date entry
    WriteDate(to,date);
  }

  if (fDescription != "") {
    to << "# " << fDescription << endl;
  }
  
  vector<string>::size_type i;
  vector<Double_t>::size_type j;

  UInt_t nwrit = 0;
  for (i = 0; i < mtr_name.size(); i++) {
    to << system << " " << mtr_name[i];
    const vector<Double_t>& v = mtr_rows[i];
    
    for (j=0; j<v.size(); j++) {
      to << Form(" %11.4E",v[j]);
    }
    to << endl;
    nwrit++;
  }

  // Copy over the rest of the database
  from.seekg(pos,ios::beg);
  CopyDB(from,to);
  if ( pos == from.tellg() )
    to << endl;

  ASSIGN_SSTREAM(db[1].contents,to);

  modified = 1;

  return nwrit;
}

//_____________________________________________________________________________
bool THaFileDB::find_constant(istream& from, int linebreak)
{
  // A utility routine. Look through the stream 'from' 
  // for the next character that is permitted to be
  // part of the constant set
  
  int ci=0;  // next character to be read in

  while ( from.good() ) {
    // Need to process line-by-line, such that if we find a case
    // where there is a new-line WITHOUT a leading space, we know
    // we've reached the end of the set.
    
    // Eat space until the next non-space character or end-of-line
    while ( from.good() && (ci=from.peek())!='\n' && isspace(ci) )
      from.get();
    
    // Check to see if a new line is coming:
    if ( ci=='\n' ) {
      if (linebreak) return true;
      from.get();
      // If it does not begin with a space-like character, the end
      // of the entry has been reached.
      if ( !isspace(from.peek()) || !from.good() )
	break;
      continue;
    }
    
    // Discard the rest of the line at a comment character
    if ( ci=='#' ) {
      while( from.peek()!='\n' ) from.get();
      continue;
    }

    // fall through to here if we have a constant to use
    return true;
  }

  return false;
}

//_____________________________________________________________________________
bool THaFileDB::IsDate( const string& line, TDatime& date ) 
{
  // If 'line' is a date stamp, extract the date to date
  // Date stamps must be in SQL format: [ yyyy-mm-dd hh:mi:ss ]

  ssiz_t lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size()-12 )
    return false;
  ssiz_t rbrk = line.find(']',lbrk);
  if( rbrk == string::npos || rbrk <= lbrk+11 )
    return false;
  string dstr(line.substr(lbrk+1,rbrk-lbrk-1));
  if( dstr.find_first_not_of("0123456789 -:") != string::npos )
    return false;
  ISSTREAM inp(dstr);
  int yy, mm, dd, hh, mi, ss = 0;
  char ch;
  if( (inp >> yy) && inp.get(ch) && ch == '-' &&
      (inp >> mm) && inp.get(ch) && ch == '-' &&
      (inp >> dd) &&
      (inp >> hh) && inp.get(ch) && ch == ':' &&
      (inp >> mi) && inp.get(ch) && ch == ':' &&
      ((inp >> ss) || inp.eof()) &&
      !(mm<0 || mm>12 || dd<0 || dd>31 || hh<0 || hh>23 || 
	mi<0 || mi>59 || ss<0 || ss>59) ) {

    date.Set(yy, mm, dd, hh, mi, ss);
    return true;
  }

  ::Warning( "THaFileDB:IsDate", "Ignoring invalid time stamp \"%s\"",
	     line.c_str() );
  return false;
}


//_____________________________________________________________________________
bool THaFileDB::IsKey( const string& line, const string& key, ssiz_t& offset )
{
  // Check if 'line' is of the form "key = value" and matches 'key'
  // In keeping with Unix convention, keys ARE case sensitive.

  // Check for a good '='
  ssiz_t pos = line.find('=');
  if( pos == string::npos || pos == 0 )
    return false;
  // Trim leading and trailing whitespace from the key
  ssiz_t pos1 = line.substr(0,pos).find_first_not_of(" \t");
  if( pos1 == string::npos )
    return false;
  ssiz_t pos2 = line.substr(0,pos).find_last_not_of(" \t");
  if( pos2 == string::npos )
    return false;
  // Extract the key and compare
  string this_key(line.substr(pos1,pos2-pos1+1));
  if( this_key != key )
    return false;

  offset = pos+1;

  return true;
}

//_____________________________________________________________________________
bool THaFileDB::FindEntry( const char* system, const char* attr, 
			   istream& from, TDatime& date )
{
  // Find the line beginning with the desired key.
  // we are always beginning at the start of a line, never
  // in the middle of one
  
  static const char* const here = "THaFileDB::FindEntry";

  if( system == NULL ) system = "";
  if( attr == NULL )   attr = "";

  // Can't have only 'system'.
  if( system[0] != 0 && attr[0] == 0 )
    return false;

  // If system given, search for the key "system.attr", 
  // otherwise search for "attr".
  string key(attr);
  if( system[0] != 0 ) {
    key.insert(0,".");
    key.insert(0,system);
  }
  // And empty key indicates we are only interested in seeking a date
  bool do_key = !key.empty();

  // Rewind the stream
  from.clear();
  from.seekg(0,ios::beg);

  TDatime curdate(950101,0), bestdate(curdate), founddate(curdate),
    prevdate(curdate);
  bool found = false, ignore = false;
  string line;
  streampos foundpos = 0, datepos = 0, lastpos = from.tellg();
  while( getline(from,line) ) {
    // Skip blank lines and comments starting with '#'.
    ssiz_t pos = line.find_first_not_of(" \t");
    if( pos != string::npos && line[pos] != '#' ) {
      ssiz_t offset;
      if( do_key && !ignore && IsKey(line, key, offset) ) {
	found = true;
	foundpos = lastpos+static_cast<streampos>(offset);
	founddate = curdate;
      } else if( IsDate(line, curdate) ) {
	if( prevdate == bestdate )
	  datepos = lastpos;
	// Require time stamps to be ordered.  If they are not, warn user 
	// but continue, returning correct results as much as possible.
	if( curdate<prevdate ) {
	  // Copy strings to get around ROOT's lovely "internal static buffer"
	  string pd(prevdate.AsSQLString()), cd(curdate.AsSQLString());
	  ::Warning( here, "Time stamps for key %s not ordered. "
		     "%s followed by %s. ",
		     key.c_str(), pd.c_str(), cd.c_str() );
	} 
	// Quit once we are past 'date'
	else if( curdate>date )
	  break;
	else if( curdate>bestdate )
	  bestdate = curdate;
        // Ignore further keys until a more recent but still valid date
        // appears in the stream
 	ignore = ( curdate<founddate );
	prevdate = curdate;
      }
    }
    lastpos = from.tellg();
  }

  if( from.eof() ) {
    // If we hit EOF, we never found a time stamp past 'date'.
    if( prevdate == bestdate )
      // Go to end of file unless there was some mis-ordering
      datepos = 0;
  } else if( from.bad() ) {
    ::Error( here, "Error reading file for key %s. Key not found.", 
	     key.c_str() );
    return false;
  }

  from.clear();
  if( found ) {
    from.seekg( foundpos );
    date = founddate;
  } else { 
    // If the key was NOT found in valid time frame, position file at the end of the
    // section with the closest matching date and return that date.
    // This functionality may be used for writing data.
    if( datepos > 0 )
      from.seekg( datepos );
    else
      from.seekg( 0, ios::end );
    date = bestdate;
  } 

  return found;
}

//_____________________________________________________________________________
bool THaFileDB::SeekDate( istream& from, TDatime& date )
{
  // Find position in the stream 'from' appropriate for writing data valid for 
  // 'date'. Usually this is the position right before the time stamp later than
  // 'date'. If 'date' is later than all time stamps in the stream, go to the 
  // end of the stream.

  const char* dummy = "";
  return FindEntry( dummy, dummy, from, date );
}

//_____________________________________________________________________________
std::ostream& operator<<(std::ostream& out, const THaFileDB& db) {
  for( int i=0; i<3; i++ )
    out << db.db[i].contents << endl;
  return out;
}

//_____________________________________________________________________________
UInt_t THaFileDB::LoadDetMap(const TDatime& date) {
  // Load the complete detector map into memory, from the
  // text configuration file given by the DAQ/Espace
  
  
  UInt_t linecnt = 0;
  ifstream s(fDetFile.c_str());
  
  string line;
  while ( getline(s,line).good() ) {
    linecnt++;
    if ( line.length() <= 0 ) continue;
    ssiz_t l = line.find('!');
    if ( l != string::npos ) {
      line.erase(l);
    }
    // Any data in this line?
    if ( line.find_first_of("0123456789") == string::npos ) continue;
    
    THaDetConfig det(line);
    if ( det.IsGood() ) {
      fDetectorMap.push_back(det);
    }
  }
  return 0;
}

//_____________________________________________________________________________
void THaFileDB::LoadDetCfgFile( const char* detcfg ) {
  // Load ( and append ) the detector configuration from given filename
  
  if (detcfg)   fDetFile = detcfg;
  TDatime now;
  LoadDetMap(now);
}

//_____________________________________________________________________________
UInt_t THaFileDB::PutDetMap( const TDatime& date ) {
  // write the detectormap out to a default file
  ofstream out("out_detmap.config");
  unsigned int i;
  for (i=0; i<fDetectorMap.size(); i++) {
    out << fDetectorMap[i] << endl;
  }

  return 0;
}


//_____________________________________________________________________________
UInt_t THaFileDB::LoadValues ( const char* system, const TagDef* list,
			       const TDatime& date )
{
  // Load a number of entries from the database.
  // For array entries, the number of elements to be read in
  // must be given, and the memory already allocated
  //
  
  const TagDef *ti = list;
  UInt_t cnt=0;
  UInt_t this_cnt=0;

  while ( ti && ti->name ) {
    if (ti->expected>1) {
      // it is an array, use the appropriate interface
      switch (ti->type) {
      case (kDouble) :
	this_cnt = GetArray(system,ti->name,static_cast<Double_t*>(ti->var),
			    ti->expected,date);
	break;
      case (kInt) :
	this_cnt = GetArray(system,ti->name,static_cast<Int_t*>(ti->var),
			    ti->expected,date);
	break;
      default:
	Error("LoadValues","Invalid type to read %s %s",system,ti->name);
	break;
      }

    } else {
      switch (ti->type) {
      case (kDouble) :
	this_cnt = GetValue(system,ti->name,*static_cast<Double_t*>(ti->var),date);
	break;
      case (kInt) :
	this_cnt = GetValue(system,ti->name,*static_cast<Int_t*>(ti->var),date);
	break;
      default:
	Error("THaFileDB","Invalid type to read %s %s",system,ti->name);
	break;
      }
    }
    
    if (this_cnt<=0) {
      if ( ti->fatal ) {
	Error("LoadValues","Could not find %s %s in database",system,ti->name);
      }
    }

    cnt += this_cnt;
    ti++;
  }
  
  return cnt;
}

//_____________________________________________________________________________
UInt_t  THaFileDB::StoreValues( const char* system, const TagDef* list,
				const TDatime& date )
{
  // Store the set of values into the database.
  // For array entries, the number of elements to be written out
  // must be given
  
  const TagDef *ti = list;
  UInt_t cnt=0;
  UInt_t this_cnt=0;

  while ( ti && ti->name ) {
    switch ( ti->type ) {
    case kDouble:
      this_cnt = PutArray( system, ti->name, static_cast<Double_t*>(ti->var),
			   ti->expected, date );
      break;
    case kInt:
      this_cnt = PutArray( system, ti->name, static_cast<Int_t*>(ti->var),
			   ti->expected, date );
      break;
    default:
      Error("StoreValues","Invalid type to write %s %s",system,ti->name);
      break;
    }
    cnt += this_cnt;
    ti++;
  }
  
  return cnt;
}

  
//_____________________________________________________________________________
int THaFileDB::LoadFile( const char* systemC, const TDatime& date,
			 string& contents )
{
  // Find an individual db file for 'system' and for 'date', then copy it 
  // into 'contents'.

  const string defaultdir("DEFAULT");
  string here("\""); here += systemC; here += "\"";

  // Build search list of directories
  const char* tmp;
  vector<string> dnames;

  // If fDBDir is set, use it and look nowhere else
  if( !fDBDir.empty() )
    dnames.push_back( fDBDir );
  // otherwise, if DB_DIR is set, use it and look nowhere else
  else if( (tmp = gSystem->Getenv("DB_DIR")) )
    dnames.push_back( tmp );
  // otherwise try ./DB, ./db, and ./
  else {
    dnames.push_back( "DB" );
    dnames.push_back( "db" );
    dnames.push_back( "." );
  }

  // Try to open the database directories in the search list.
  // The first directory that can be opened is taken to be the database
  // directory. Subsequent directories are ignored.
  
  vector<string>::iterator it = dnames.begin();
  void* dirp;
  while( !(dirp = gSystem->OpenDirectory( (*it).c_str() )) && 
	 (++it != dnames.end()) );

  // None of the directories can be opened?
  if( it == dnames.end() ) {
    Error( here.c_str(), "Cannot open database directory. Tried" );
    ostream_iterator<string> outp( cerr, " " );
    copy( dnames.begin(), dnames.end(), outp ); cerr << endl;
    return -1;
  }

  // Pointer to database directory string
  vector<string>::iterator thedir = it;

  // In the database directory, get the names of all subdirectories matching 
  // a YYYYMMDD pattern.
  vector<string> time_dirs;
  string item;
  bool have_defaultdir = false;
  while( (tmp = gSystem->GetDirEntry(dirp)) ) {
    item = tmp;
    if( item.length() == 8 ) {
      unsigned int pos;
      for( pos=0; pos<8; ++pos )
	if( !isdigit(item[pos])) break;
      if( pos==8 )
	time_dirs.push_back( item );
    } else if ( item == defaultdir )
      have_defaultdir = true;
  }
  gSystem->FreeDirectory(dirp);

  // Search a date-coded subdirectory that corresponds to the requested date.
  bool found = false;
  if( time_dirs.size() > 0 ) {
    sort( time_dirs.begin(), time_dirs.end() );
    for( it = time_dirs.begin(); it != time_dirs.end(); it++ ) {
      Int_t item_date = atoi((*it).c_str());
      if( it == time_dirs.begin() && date.GetDate() < item_date )
	break;
      if( it != time_dirs.begin() && date.GetDate() < item_date ) {
	it--;
	found = true;
	break;
      }
      // Assume that the last directory is valid until infinity.
      if( it+1 == time_dirs.end() && date.GetDate() >= item_date ) {
	found = true;
	break;
      }
    }
  }

  // Construct the database file name. It is of the form <system>.db.
  // NB: Usually, subdetectors use the same files as their parent detectors!
  string filename(systemC);
  filename += ".db";

  // Build the searchlist of file names in the order:
  //  ./filename 
  //  <dbdir>/<date-dir>/filename
  //  <dbdir>/DEFAULT/filename
  //  <dbdir>/filename

  vector<string> fnames;
  fnames.push_back( filename );
  if( found ) {
    item = *thedir + "/" + *it + "/" + filename;
    fnames.push_back( item );
  }
  if( have_defaultdir ) {
    item = *thedir + "/" + defaultdir + "/" + filename;
    fnames.push_back( item );
  }
  fnames.push_back( *thedir + "/" + filename );

  // Try to open these files in turn and read the first one found.

  ifstream infile;
  it = fnames.begin();
  do {
    if( fDebug>1 )
      cout << "<" << /* here << */ ">: Opening database file " << *it;
    // Open the database file
    infile.clear();
    infile.open( (*it).c_str() );
      
    if( fDebug>1 ) {
      if( !infile.good() ) 
	cout << " ... failed" << endl;
      else
	cout << " ... ok" << endl;
    } else if( fDebug>0 && infile.good() )
      cout << "<" << /* here << */ ">: Opened database file " << *it << endl;
    // continue until we succeed
  } while ( !infile.good() && ++it != fnames.end() );

  if( !infile.good() && fDebug>0 ) {
    Error( here.c_str(), "Cannot open database file %s", filename.c_str() );
    return -2;
  }

  // Now we have the file. Read its contents into the 'contents' string

  infile.seekg( 0, ios::end );
  streamsize len = infile.tellg();
  infile.seekg( 0, ios::beg );
  char* buf = new char[len+1];
  if( !buf )
    return -255;
  infile.read( buf, len );
  buf[len] = 0;
  contents = buf;
  delete [] buf;

  infile.close();
  return 0;
}

//_____________________________________________________________________________
int THaFileDB::LoadFile( DBFile& db )
{
  int ret = LoadFile( db.system_name.c_str(), db.date, db.contents );
  db.good = ( ret == 0 );
  return ret;
}


//_____________________________________________________________________________
bool THaFileDB::LoadDB( const char* systemC, const TDatime& date )
{
  // Read database file(s) for 'system' into memory 
  // db[0]:  individual file for system
  // db[1]:  master database file
  // db[2]:  run database

  const char* system = ( systemC != NULL ) ? systemC : "";

  if( db[0].system_name != system ) {
    db[0].system_name = system;
    db[0].good = db[0].tried = false;
  }

  for( int i=0; i<3; i++ ) {
    if( db[i].date != date ) {
      db[i].date = date;
      db[i].tried = false;
    }
    if( !db[i].tried ) {
      LoadFile( db[i] );
      db[i].tried = true;
    }
  }

  // return true if at least one file is good
  return ( db[0].good || db[1].good || db[2].good );
}

////////////////////////////////////////////////////////////////////////////////

ClassImp(THaFileDB)
