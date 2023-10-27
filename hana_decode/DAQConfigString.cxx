//*-- Author :    Ole Hansen   28/08/22

//////////////////////////////////////////////////////////////////////////
//
// DAQConfigString
//
// Helper class to support DAQ configuration info.
//
// Used in CodaDecoder and THaRunParameters.
// Filled in CodaDecoder::daqConfigDecode.
//
//////////////////////////////////////////////////////////////////////////

#include "DAQConfigString.h"
#include "Textvars.h"   // for Podd::vsplit
#include <sstream>

using namespace std;

//_____________________________________________________________________________
size_t DAQConfigString::parse()
{
  // Parse DAQ configuration text into key/value pairs stored in fKeyVal map.
  // The key is the first field of a non-comment line. The value is the rest of
  // the line. Whitespace between value fields is collapsed into single spaces.

  istringstream istr(text_);
  string line;
  while( getline(istr, line) ) {
    auto pos = line.find('#');
    if( pos != string::npos )
      line.erase(pos);
    if( line.find_first_not_of(" \t") == string::npos )
      continue;
    auto items = Podd::vsplit(line);
    if( !items.empty() ) {
      string& key = items[0];
      string val;
      val.reserve(line.size());
      for( size_t j = 1, e = items.size(); j < e; ++j ) {
        val.append(items[j]);
        if( j + 1 != e )
          val.append(" ");
      }
      if( val != "end" )
        keyval_.emplace(std::move(key), std::move(val));
    }
  }
  return keyval_.size();
}

//_____________________________________________________________________________
ClassImp(DAQConfigString)
