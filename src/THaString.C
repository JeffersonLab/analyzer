////////////////////////////////////////////////////////////////////////
//
//       THaString.C  (implementation)
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

//_____________________________________________________________________________
int 
THaString::CmpNoCase (const THaString& s)
{
  // Case insensitive compare.  Returns -1 if "less", 0 if equal, 1 if
  // "greater".

  string::const_iterator p =  begin();
  string::const_iterator p2 = s.begin();

  while (p != this->end() && p2 != s.end())
    {
      if (toupper(*p) != toupper(*p2))
	return (toupper(*p) < toupper(*p2)) ? -1 : 1;
      ++p;
      ++p2;
    }

  return (s.size() == size()) ? 0 : (size() < s.size()) ? -1 : 1;
}

//_____________________________________________________________________________
vector<THaString> 
THaString::Split()
{
  // Split on whitespace.
  istrstream ist(c_str());
  string w;
  vector<THaString> v;

  while (ist >> w)
    v.push_back(w);
  return v;
}


//_____________________________________________________________________________
UInt_t 
THaString::Hex()
{
  // Conversion to unsigned interpreting as hex.
  istrstream ist(c_str());
  UInt_t in;
  ist >> hex >> in;
  return in;
}

THaString 
THaString::ToLower ()
{
  // Conversion to lower case.
  THaString::const_iterator p = begin();
  THaString result("");
  while (p != end()) 
    {
      result += tolower(*p);
      ++p;
    }
  return result;
}

//_____________________________________________________________________________
THaString 
THaString::ToUpper ()
{
 // Conversion to lower case.
  THaString::const_iterator p = begin();
  THaString result("");
  while (p != end()) 
    {
      result += toupper(*p);
      ++p;
    }
  return result;
}








