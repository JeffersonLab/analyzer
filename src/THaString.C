//**********************************************************************
//
//       THaString.cc  (implementation)
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

#include "THaString.h"
#include <strstream>

ClassImp(THaString)

int 
THaString::CmpNoCase (const THaString& s)
{
  // Case insensitive compare.  Returns -1 if "less", 0 if equal, 1 if
  // "greater".

  string::const_iterator p = this->begin();
  string::const_iterator p2 = s.begin();

  while (p != this->end() && p2 != s.end())
    {
      if (toupper(*p) != toupper(*p2))
	return (toupper(*p) < toupper(*p2)) ? -1 : 1;
      ++p;
      ++p2;
    }

  return (s.size() == this->size()) ? 0 : (this->size() < s.size()) ? -1 : 1;
}

vector<THaString> 
THaString::Split()
{
  // Split on whitespace.
  istrstream ist(this->c_str());
  string w;
  vector<THaString> v;

  while (ist >> w)
    v.push_back(w);
  return v;
}


UInt_t 
THaString::Hex()
{
  // Conversion to unsigned interpreting as hex.
  istrstream ist(this->c_str());
  UInt_t in;
  ist >> hex >> in;
  return in;
}

THaString 
THaString::ToLower ()
{
  // Conversion to lower case.
  THaString::const_iterator p = this->begin();
  THaString result("");
  while (p != this->end()) 
    {
      result += tolower(*p);
      ++p;
    }
  return result;
}

THaString 
THaString::ToUpper ()
{
 // Conversion to lower case.
  THaString::const_iterator p = this->begin();
  THaString result("");
  while (p != this->end()) 
    {
      result += toupper(*p);
      ++p;
    }
  return result;
}








