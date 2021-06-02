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

namespace THaString {
  // case insensitive compare
  int CmpNoCase( const std::string&, const std::string& );

  // case insensitive find
  std::string::size_type FindNoCase( std::string data, std::string chunk );

  // split on whitespace
  std::vector<std::string> Split( const std::string& );

  // conversion to unsigned interpreting as hex
  unsigned int Hex( const std::string& );

  // return lower case copy
  std::string  ToLower( const std::string& );

  // return upper case copy
  std::string  ToUpper( const std::string& );

  // convert this string to lower case
  void      Lower( std::string& );

  // convert this string to upper case
  void      Upper( std::string& );
}

#endif
