#ifndef ROOT_THaString
#define ROOT_THaString

//**********************************************************************
//
//       THaString.h  (interface)
//
// Authors:  R. Holmes, A. Vacheret, R. Michaels 
//
// Derives from STL string; provides additional methods.  No
// additional storage is defined, so strings and THaStrings can be
// converted back and forth as needed; e.g. to convert a string to
// lowercase you can do something like
//
//      string mixedstring ("MixedCaseString");
//      THaString temp = mixedstring;
//      string lowerstring = temp.ToLower();
//
////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>
#include <vector>

//typedef unsigned UInt_t;  // take out when done debugging!!!!!!

class THaString : public std::string {
public:

  // Constructors/destructors/operators

  THaString () {};
  THaString (const std::string& s): std::string(s) {};
  THaString (const char* c): std::string(c) {};
  virtual ~THaString() {};

  // Major functions

  int       CmpNoCase (const THaString& s) const; // case insensitive compare
  std::vector<THaString> Split() const;   // split on whitespace
  UInt_t    Hex() const;      // conversion to unsigned interpreting as hex
  THaString ToLower() const;  // return lower case copy
  THaString ToUpper() const;  // return upper case copy
  void      Lower();          // convert this string to lower case
  void      Upper();          // convert this tring to upper case

  ClassDef(THaString, 0)   // Improved string class

};

#endif
