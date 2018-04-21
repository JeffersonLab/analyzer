#ifndef Podd_THaString_h_
#define Podd_THaString_h_

//**********************************************************************
//
//       THaString.h  (interface)
//
// Useful string functions
//
////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace THaString {
  // case insensitive compare
  int CmpNoCase( const string&, const string& );

  // case insensitive find
  string::size_type FindNoCase( string data, string chunk );

  // split on whitespace
  vector<string> Split( const string& );

  // conversion to unsigned interpreting as hex
  unsigned int Hex( const string& );

  // return lower case copy
  string    ToLower( const string& );

  // return upper case copy
  string    ToUpper( const string& );

  // convert this string to lower case
  void      Lower( string& );

  // convert this string to upper case
  void      Upper( string& );
}

#endif
