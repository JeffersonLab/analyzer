#include "THaDBFile.h"

#include "TString.h"
#include "TMath.h"
#include "TSystem.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

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

using namespace std;

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
    // 
    
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
    
    int ci;
    std::string line;
    int cnt=0;
    // strings must begin with a '\t'
    while ( (ci=from.peek(),ci=='\t') && getline(from,line).good() ) {
      if (cnt>0 && line[0]!='\t') 
	break;
      str.fString += line[0];
    }
    return from;
  }
  
}

//_____________________________________________________________________________
THaDBFile::THaDBFile(const char* infile, const char* detcfg,
		       vector<THaDetConfig>* detmap)
  : fInFileName(infile), fOutFileName("out.db"), fDetFile(detcfg)
{
  cerr << "Reading configuration information from : " << endl;
  cerr << "\t" << infile << " and " << detcfg << endl;
  
  if (detmap) fDetectorMap = *detmap;
  fDescription = "";
  LoadDB();
}

//_____________________________________________________________________________
THaDBFile::~THaDBFile()
{
  // the THaDB parent handles the assigning/freeing of the global
  // variable pointer
  if (modified) FlushDB();
  
}

//_____________________________________________________________________________
int THaDBFile::LoadDB()
{
  // Read database from fInFileName, dropping all that is in memory
  db_contents.erase();
  ifstream from(fInFileName.c_str());
  string line;
  while ( from ) {
    getline(from,line);
    db_contents.append(line);
    db_contents += '\n';
  }
  modified = 0;

  return from.eof();
}

//_____________________________________________________________________________
int THaDBFile::FlushDB()
{
  // Write contents of memory out to file
  ofstream to(fOutFileName.c_str());
  to << db_contents;
  if (db_contents.length()>0 && db_contents[db_contents.length()-1] != '\n')
    to << endl;
  modified = 0;
  
  return to.good();
}

//_____________________________________________________________________________
void THaDBFile::PrintDB()
{
  // Print database contents
  cout << *this << endl;
}

//_____________________________________________________________________________
void  THaDBFile::SetOutFile( const char* outfname )
{
  fOutFileName = outfname; 
}

//_____________________________________________________________________________
bool THaDBFile::CopyDB(istream& in, ostream& out, streampos pos) {
  // Copy, starting at the current position through position "pos"
  // from "in" to "out"
  int ch;
  while ( (ch=in.get())!=EOF && in.good() && (pos < 0 || in.tellg()<pos) ) {
    out.put(ch);
  }
  if ( !in.good() && !in.eof() ) {
    //    Warning("THaDBFile::Copy","Something weird happened while copying...");
    return false;
  }
  return true;
}

//  READING A SINGLE VALUE
//_____________________________________________________________________________
Int_t THaDBFile::GetValue( const char* system, const char* attr,
			   Int_t& value, const TDatime& date)
{
  // Read an Int_t for system/attribute
  return ReadValue(system,attr,value,date);
}

//_____________________________________________________________________________
Int_t THaDBFile::GetValue( const char* system, const char* attr,
			   Double_t& value, const TDatime& date)
{
  // Read an Double_t for system/attribute
  return ReadValue(system,attr,value,date);
}

//_____________________________________________________________________________
Int_t THaDBFile::GetValue( const char* system, const char* attr,
			   string& value, const TDatime& date)
{
  // Read a string for system/attribute

  DbString dbstr;
  Int_t ret = ReadValue(system,attr,dbstr,date);
  value = dbstr.fString;
  return ret;
}

//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::ReadValue( const char* systemC, const char* attrC,
			    T& value, TDatime date)
{
  // Read in a single value, using the istream >> operator.
  // Actually implements the reading functions.

  // look through the file, looking for the latest date stamp
  // that is still before 'date'

  ISSTREAM from(db_contents.c_str());
  //  ifstream from(fInFileName.c_str());
  string system(systemC);
  string attr(attrC);

  if ( from.good() && FindEntry(system,attr,from,date) ) {
    // found an entry
    
    from >> value;
    return 1;
  }

  return 0;
}


//  READING AN ARRAY INTO A VECTOR
//_____________________________________________________________________________
Int_t THaDBFile::GetArray( const char* system, const char* attr,
			   vector<Int_t>& array, const TDatime& date)
{
  // Read in a set of Int_t's in to a vector.
  
  return ReadArray(system,attr,array,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::GetArray( const char* system, const char* attr,
			   vector<Double_t>& array, const TDatime& date)
{
  // Read in a set of Double_t's in to a vector.
  
  return ReadArray(system,attr,array,date);
}

//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::ReadArray( const char* systemC, const char* attrC,
			    vector<T>& array, TDatime date)
{
  // Read in a series of values, using the istream >> operator
  // They are returned in a vector, with the return value and the
  // array.size() equal to the number of constants read in.
  //
  // Since "array" may be resized, anticipate the possiblity of it moving. 
  // Also, for efficiency .reserve'ing the appropriate space for the array
  // before the call is recommended.

  ISSTREAM from(db_contents.c_str());
  //  ifstream from(fInFileName.c_str());
  string system(systemC);
  string attr(attrC);


  Int_t s=array.size();
  //  vector<T>::size_type s=array.size();
  
  Int_t cnt=0;
  
  if ( from.good() && FindEntry(system,attr,from,date) ) {
    while ( from.good() && find_constant(from) ) {
      
      // We must have some constants!
      if (cnt>=s) {
	array.resize(cnt+10);
	s=array.size();
      }
      from >> array[cnt];
      cnt++;
    }
  }
  
  array.resize(cnt); // only keep it as large as necessary
  return cnt;
}


//  READING AN ARRAY INTO A C-style ARRAY
//_____________________________________________________________________________
Int_t THaDBFile::GetArray( const char* system, const char* attr,
			   Int_t* array, Int_t size, const TDatime& date)
{
  // Read in a set of Int_t's in to a C-style array.
  
  return ReadArray(system,attr,array,size,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::GetArray( const char* system, const char* attr,
			   Double_t* array, Int_t size, const TDatime& date)
{
  // Read in a set of Double_t's in to a vector.
  
  return ReadArray(system,attr,array,size,date);
}

//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::ReadArray( const char* systemC, const char* attrC,
			   T* array, Int_t size, TDatime date )
{
  // Read in a series of values, using the istream >> operator
  // They are returned in a C-style array, with the return value equal
  // to the number of constants read in.
  //
  // No resizing is done, so only 'size' elements may be stored.

  ISSTREAM from(db_contents.c_str());
  //  ifstream from(fInFileName.c_str());
  string system(systemC);
  string attr(attrC);

  Int_t cnt=0;

  if ( from.good() && FindEntry(system,attr,from,date) ) {
    while ( from.good() && find_constant(from) ) {
      // We must have some constants!

      // If the array is full, do not read in any more
      if (cnt>=size) {
	break;
      }
      
      from >> array[cnt];
      cnt++;
    }
  }

  return cnt;
}


//  READING A MATRIX INTO A two-dimensional vector
//_____________________________________________________________________________
Int_t THaDBFile::GetMatrix( const char* system, const char* attr,
			    vector<vector<Int_t> >& matrix,
			    const TDatime& date)
{
  // Read in a set of Int_t's in to a 2-d matrix
  
  return ReadMatrix(system,attr,matrix,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::GetMatrix( const char* system, const char* attr,
			    vector<vector<Double_t> >& matrix,
			    const TDatime& date)
{
  // Read in a set of Double_t's in to a 2-d matrix
  
  return ReadMatrix(system,attr,matrix,date);
}


//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::ReadMatrix( const char* systemC, const char* attrC, 
			     vector<vector<T> >& rows, TDatime date )
{
  // Reads in a matrix of values, broken by '\n'. Each line in the matrix
  // must begin with whitespace
  ISSTREAM from(db_contents.c_str());
  //  ifstream from(fInFileName.c_str());
  string system(systemC);
  string attr(attrC);
  
  vector<T> v;
  T tmp;
  
  // Clear out the matrix vectors, first
  typename vector<vector<T> >::size_type vi;
  // Make certain we are starting with a fresh matrix
  for (vi = 0; vi<rows.size(); vi++) {
    rows[vi].clear();
  }
  rows.clear();
  
  // this differs from a 'normal' vector in that here, each line
  // gets its own row in the matrix
  if ( FindEntry(system,attr,from,date) ) {
    // Start collecting a new row
    while ( from.good() && find_constant(from) ) {
      v.clear();

      // collect a row, breaking on a '\n'
      while ( from.good() && find_constant(from,1) ) {
	from >> tmp;
	v.push_back(tmp);
      }
      
      if (v.size()>0) {
	rows.push_back(v);
      }
    }
  }
  return rows.size();
}



//    WRITING A SINGLE VALUE TO THE FILE
//_____________________________________________________________________________
Int_t THaDBFile::PutValue( const char* system, const char* attr,
			   const Int_t& value, const TDatime& date )
{
  // Save the single Int_t value, corresponding to system/attribute
  return WriteValue(system,attr,value,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::PutValue( const char* system, const char* attr,
			   const Double_t& value, const TDatime& date )
{
  // Save the single Double_t value, corresponding to system/attribute
  return WriteValue(system,attr,value,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::PutValue( const char* system, const char* attr,
			   const string& value, const TDatime& date )
{
  // Save the single string value, corresponding to system/attribute

  DbString dbstr(value);

  return WriteValue(system,attr,dbstr,date);
}
  
//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::WriteValue( const char* system, const char* attr,
			     const T& value, TDatime date)
{
  // Write out a single value, using the ostream << operator
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db_contents.c_str());
  OSSTREAM to;

  // Find the last location with a date earlier or equal to the current date
  string sysJNK("JUNK");
  string attJNK("JUNK");
  
  FindEntry(sysJNK,attJNK,from,fnd_date);

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

  ASSIGN_SSTREAM(db_contents,to);

  modified = 1;
  
  return 1;
}


//    WRITING A VECTOR TO THE FILE
//_____________________________________________________________________________
Int_t THaDBFile::PutArray( const char* system, const char* attr,
			   const vector<Int_t>& array, const TDatime& date )
{
  // Save the vector of Int_t 's corresponding to system/attribute
  return WriteArray(system,attr,array,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::PutArray( const char* system, const char* attr,
			   const vector<Double_t>& array, const TDatime& date )
{
  // Save the vector of Double_t 's, corresponding to system/attribute
  return WriteArray(system,attr,array,date);
}

//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::WriteArray( const char* system, const char* attr,
			     const vector<T>& v, TDatime date)
{
  // Write out an array, using the ostream << operator
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db_contents.c_str());
  OSSTREAM to;
  
  // Find the last location with a date (or equal to) the current date
  string sysJNK("JUNK");
  string attJNK("JUNK");
  
  FindEntry(sysJNK,attJNK,from,fnd_date);

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
  
  ASSIGN_SSTREAM(db_contents,to);

  modified = 1;
  
  return nwrit;
}


//    WRITING A C-style array TO THE FILE
//_____________________________________________________________________________
Int_t THaDBFile::PutArray( const char* system, const char* attr,
			   const Int_t* array, Int_t size,
			   const TDatime& date )
{
  // Save the vector of Int_t 's corresponding to system/attribute
  return WriteArray(system,attr,array,size,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::PutArray( const char* system, const char* attr,
			   const Double_t* array, Int_t size,
			   const TDatime& date )
{
  // Save the vector of Double_t 's, corresponding to system/attribute
  return WriteArray(system,attr,array,size,date);
}

//_____________________________________________________________________________
template<class T>
Int_t THaDBFile::WriteArray( const char* system, const char* attr,
			     const T* array, Int_t size, 
			     TDatime date)
{

  // Write out the sequence of values to the database file. Can also
  // write out scalers (size = 0 or 1)

  if (size == 0) size = 1;
  
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db_contents.c_str());
  OSSTREAM to;

  // Find the last location with a date (or equal to) the current date
  string sysJNK("JUNK");
  string attJNK("JUNK");
  
  FindEntry(sysJNK,attJNK,from,fnd_date);

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
  streampos oldpos = to.tellp();
  to << system << " " << attr << ' ';
  
  Int_t nwrit = 0;

  ios::fmtflags o_flag = to.flags();
  int o_precision = to.precision();
  
  to.precision(6);
  to.unsetf(ios::floatfield);
  
  Int_t i;
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
  
  ASSIGN_SSTREAM(db_contents,to);

  modified = 1;

  return nwrit;
}


//    WRITE A MATRIX, BUILT OF VECTORS
//_____________________________________________________________________________
Int_t THaDBFile::PutMatrix( const char* system, const char* name,
			    const vector<vector<Double_t> >& rows,
			    const TDatime& date )
{
  // Write out a matrix of Double_t 's
  return WriteMatrix(system,name,rows,date);
}
//_____________________________________________________________________________
Int_t THaDBFile::PutMatrix( const char* system, const char* name,
			    const vector<vector<Int_t> >& rows,
			    const TDatime& date )
{
  // Write out a matrix of Int_t 's
  return WriteMatrix(system,name,rows,date);
}

//_____________________________________________________________________________
template <class T>
Int_t THaDBFile::WriteMatrix( const char* system, const char* attr,
			      const vector<vector<T> >& matrix,
			      TDatime date )
{
  // Write out a matrix of arbitrary type, just needs the ostream<< operator
  // prepared. Each line corresponds to a row in the matrix
  // For now, each of these will copy from the old file, writing
  // to the new one

  TDatime fnd_date(date);

  ISSTREAM from(db_contents.c_str());
  OSSTREAM to;

  // Find the last location with a date (or equal to) the current date
  string sysJNK("JUNK");
  string attJNK("JUNK");
  
  FindEntry(sysJNK,attJNK,from,fnd_date);

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
  to << system << " " << attr << '\n';
  
  Int_t nwrit = 0;

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
  
  ASSIGN_SSTREAM(db_contents,to);

  modified = 1;
  
  return nwrit;
}

//_____________________________________________________________________________
bool THaDBFile::find_constant(istream& from, int linebreak) {
  // A Utility routine. Look through the stream 'from' 
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
bool THaDBFile::NextLine( istream& from ) {
  // look at the beginning of a new line and determine if it is the beginning
  // of a record, or just blank space or comments
  int ci=from.peek();
  if ( ci == '#' || isspace(ci) ) {
    // Found a comment or are in the middle of a sequence of constants.
    // Ignore until the next '\n'
    while ( (ci=from.get(),from.good()) && ci!='\n');
    return true;
  }
  return false;
}

//_____________________________________________________________________________
void THaDBFile::WriteDate( ostream& to, const TDatime& date ) {
  // Write a line that states the date of the future entries

  to << '\n';
  to << "--------[ ";
  to << date.AsSQLString() << " ]\n" << endl;
}

//_____________________________________________________________________________
bool THaDBFile::IsDate( istream& from, streampos& pos, TDatime& date ) {
  // if the next line is a date entry, save that information
  // (position before the date and the date itself)
  int ci;

  if ( (ci=from.peek()) =='-' ) {
    pos = from.tellg()+(streampos)1;
    // this should be a date
    string line;
    getline(from,line);
    ssiz_t lbrk = line.find('[');
    if( lbrk == string::npos || lbrk >= line.size()-12 ) return 0;
    ssiz_t rbrk = line.find(']',lbrk);
    if( rbrk == string::npos || rbrk <= lbrk+11 ) return 0;
    Int_t yy, mm, dd, hh, mi, ss;
    if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%d-%d-%d %d:%d:%d",
		&yy, &mm, &dd, &hh, &mi, &ss) != 6) {
      Warning("THaDBFile::IsDate", 
	      "Invalid date tag %s", line.c_str());
    }
    date.Set(yy,mm,dd,hh,mi,ss);
    return true;
  }
  return false;
}


//_____________________________________________________________________________
bool THaDBFile::FindEntry( string& system, string& attr, istream& from,
			   TDatime& date ) {
  // Find the line beginning with the desired key.
  // we are always beginning at the start of a line, never
  // in the middle of one
  
  // system and attr are always set to what was actually found!
  
  string sys_in;
  string att_in;
  ssiz_t sys_len = system.length();
  ssiz_t attr_len = attr.length();
  
  bool sys_match;
  bool att_match;

  streampos data_pos=0;
  
  TDatime prevdate(950101,0);

  TDatime curdate(950101,0);
  TDatime datetmp(curdate);
  TDatime fnddate;
  
  streampos curdate_pos=0;

  from.clear();
  
  // if a wild-card character '*' is the last part of the name, then match to only
  // the first part of the string
  
  sys_len =  ( system[sys_len-1] == '*' ? sys_len-1  : 0 );
  attr_len = (  attr[attr_len-1] == '*' ? attr_len-1 : 0 );

  while ( from.good() && curdate<=date ) {

    // look for a line that does NOT begin with a space or '#'
    if ( NextLine(from) ) continue;

    // If we passed the target date, stop looping and use what we have
    if ( IsDate(from,curdate_pos,datetmp) ) {
      prevdate = curdate;
      curdate  = datetmp;
      
      continue;
    }

    // We have a line that should be the beginning of a system/attribute.
    // cannot grab the whole line because the wanted values could
    // be on the same line.

    from >> sys_in;
    from >> att_in;

    if ( !from.good() ) break;
    
    // check against part or the whole string, to find the system and attribute
    sys_match = sys_in==system ||
      ( sys_len && sys_in. STR_COMPARE(0,sys_len,system) ==0 );
    
    if ( sys_match ) {
      att_match =  att_in==attr ||
	( attr_len && att_in. STR_COMPARE(0,attr_len,attr)==0 );
      if ( att_match ) {
	attr = att_in;
	system = sys_in;

	data_pos = from.tellg();
	fnddate  = curdate;
      }      
    }

    // We are in a set of constants. Discard the rest of the line
    // and let the top part of the routine concern itself with
    // finding the next Key.
    NextLine(from);
    
  }

  if ( data_pos ) { 
    from.clear();
    from.seekg(data_pos);
    date = fnddate;
    return true;
  }

  // Only reach here if the key was NOT found in valid time frame
  
  // If end-of-file found, use last read in date and set pointer to the end
  // of the file
  if ( from.eof() ) {
    date = curdate;
    from.seekg(0,ios::end);
  } else {
    from.seekg(curdate_pos); // seek to just before 'too late'
    date = prevdate;
  }

  
  return false;
}

//_____________________________________________________________________________
Int_t THaDBFile::GetMatrix( const char* systemC, vector<string>& mtr_name, 
			    vector<vector<Double_t> >& mtr_rows, const TDatime &date )
{
  // Read in the transport matrix from the text-file database.
  // Return each entry as correlated vectors of string and floats
  //  
  // Read in the matrix as a group, all at once
  // 

  TDatime orig_date(date);

  ISSTREAM from(db_contents.c_str());

//    ifstream from(fInFileName.c_str());

  string system(systemC);
  string attr_save("matrix_*");
  
  vector<Double_t> v;
  Double_t tmp;
  
  // Clear out the matrix vectors, first
  mtr_name.clear();
  
  vector<vector<Double_t> >::size_type vi;

  // Make certain we are starting with a fresh matrix
  for (vi = 0; vi<mtr_rows.size(); vi++) {
    mtr_rows[vi].clear();
  }
  mtr_rows.clear();
  
  string attr(attr_save);
  
  while ( from.good() && FindEntry(system,attr,from,orig_date) ) {
    v.clear();
    while ( from.good() && find_constant(from) ) {
      from >> tmp;
      v.push_back(tmp);
    }
    
    if (v.size()>0) {
      mtr_name.push_back(attr);
      mtr_rows.push_back(v);
      attr = attr_save;
    }
  }
  return mtr_rows.size();
}
  
  

//_____________________________________________________________________________
Int_t THaDBFile::PutMatrix( const char* system, 
			    const vector<string>& mtr_name, 
			    const vector<vector<Double_t> >& mtr_rows,
			    const TDatime& date )
{
  // Write out the transport matrix to the text-file database.
  // They are passed in as correlated vectors in mtr_name, mtr_rows

  TDatime fnd_date(date);

  ISSTREAM from(db_contents.c_str());
  OSSTREAM to;
  
  // Find the last location with a date (or equal to) the current date
  string sysJNK("JUNK");
  string attJNK("JUNK");
  
  FindEntry(sysJNK,attJNK,from,fnd_date);

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

  Int_t nwrit = 0;
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

  ASSIGN_SSTREAM(db_contents,to);

  modified = 1;

  return nwrit;
}

//_____________________________________________________________________________
std::ostream& operator<<(std::ostream& out, THaDBFile& db) {
  out << db.db_contents << endl;
  return out;
}

//_____________________________________________________________________________
Int_t THaDBFile::LoadDetMap(const TDatime& date) {
  // Load the complete detector map into memory, from the
  // text configuration file given by the DAQ/Espace
  
  
  int linecnt = 0;
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
void THaDBFile::LoadDetCfgFile( const char* detcfg ) {
  // Load ( and append ) the detector configuration from given filename
  
  if (detcfg)   fDetFile = detcfg;
  TDatime now;
  LoadDetMap(now);
}

//_____________________________________________________________________________
void THaDBFile::SetCalibFile( const char* calib ) {
  // Set the filename from which to read calibration information
  
  if (calib) fInFileName = calib;
}

//_____________________________________________________________________________
Int_t THaDBFile::PutDetMap( const TDatime& date ) {
  // write the detectormap out to a default file
  ofstream out("out_detmap.config");
  unsigned int i;
  for (i=0; i<fDetectorMap.size(); i++) {
    out << fDetectorMap[i] << endl;
  }

  return 0;
}


//_____________________________________________________________________________
Int_t THaDBFile::LoadValues ( const char* system, const TagDef* list,
			      const TDatime& date )
{
  // Load a number of entries from the database.
  // For array entries, the number of elements to be read in
  // must be given, and the memory already allocated
  //
  
  const TagDef *ti = list;
  Int_t cnt=0;
  Int_t this_cnt=0;

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
	Error("THaDBFile","Invalid type to read %s %s",system,ti->name);
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
	Error("THaDBFile","Invalid type to read %s %s",system,ti->name);
	break;
      }
    }
    
    if (this_cnt<=0) {
      if ( ti->fatal ) {
	Fatal("THaDBFile","Could not find %s %s in database",system,ti->name);
      }
    }

    cnt += this_cnt;
    ti++;
  }
  
  return cnt;
}

//_____________________________________________________________________________
Int_t  THaDBFile::StoreValues( const char* system, const TagDef* list,
		    const TDatime& date )
{
  // Store the set of values into the database.
  // For array entries, the number of elements to be written out
  // must be given
  
  const TagDef *ti = list;
  Int_t cnt=0;
  Int_t this_cnt=0;

  while ( ti && ti->name ) {
    switch ( ti->type ) {
    case kDouble:
      this_cnt = WriteArray( system, ti->name, static_cast<Double_t*>(ti->var),
			     ti->expected, date );
      break;
    case kInt:
      this_cnt = WriteArray( system, ti->name, static_cast<Int_t*>(ti->var),
			     ti->expected, date );
      break;
    default:
      Error("THaDBFile","Invalid type to write %s %s",system,ti->name);
      break;
    }
    cnt += this_cnt;
    ti++;
  }
  
  return cnt;
}

  
//_____________________________________________________________________________
void  THaDBFile::SetDescription(const char* description)
{
  // Set the description to be written out with the next Write
  fDescription = description;
}

ClassImp(THaDBFile)

////////////////////////////////////////////////////////////////////////////////
