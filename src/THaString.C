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
#include <algorithm>
#include <cctype>

#ifdef HAS_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

using namespace std;

//_____________________________________________________________________________
int THaString::CmpNoCase (const THaString& s) const
{
  // Case insensitive compare.  Returns -1 if "less", 0 if equal, 1 if
  // "greater".

  string::const_iterator p =  begin();
  string::const_iterator p2 = s.begin();

  while (p != end() && p2 != s.end()) {
    if (toupper(*p) != toupper(*p2))
      return (toupper(*p) < toupper(*p2)) ? -1 : 1;
    ++p;
    ++p2;
  }

  return (s.size() == size()) ? 0 : (size() < s.size()) ? -1 : 1;
}

//_____________________________________________________________________________
vector<THaString> THaString::Split() const
{
  // Split on whitespace.
#ifdef HAS_SSTREAM
  istringstream ist(c_str());
#else
  istrstream ist(c_str());
#endif
  string w;
  vector<THaString> v;

  while (ist >> w)
    v.push_back(w);
  return v;
}

//_____________________________________________________________________________
UInt_t THaString::Hex() const
{
  // Conversion to unsigned interpreting as hex.
#ifdef HAS_SSTREAM
  istringstream ist(c_str());
#else
  istrstream ist(c_str());
#endif
  UInt_t in;
  ist >> hex >> in;
  return in;
}

//_____________________________________________________________________________
THaString THaString::ToLower() const
{
  // Return copy of this string converted to lower case.

  THaString result(*this);
  // The bizarre cast here is needed for newer gccs
  transform( begin(), end(), result.begin(), (int(*)(int))tolower );
  return result;
}

//_____________________________________________________________________________
THaString THaString::ToUpper() const
{
  // Return copy of this string converted to upper case.
  THaString result(*this);
  transform( begin(), end(), result.begin(), (int(*)(int))toupper );
  return result;
}

//_____________________________________________________________________________
void THaString::Lower()
{
  // Convert this string to lower case

  transform( begin(), end(), begin(), (int(*)(int))tolower );
  return;
}

//_____________________________________________________________________________
void THaString::Upper()
{
  // Convert this string to upper case
  transform( begin(), end(), begin(), (int(*)(int))toupper );
  return;
}

//_____________________________________________________________________________
ClassImp(THaString)
