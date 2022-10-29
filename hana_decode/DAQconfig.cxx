//*-- Author :    Ole Hansen   28/08/22

//////////////////////////////////////////////////////////////////////////
//
// DAQconfig, DAQInfoExtra
//
// Helper classes to support DAQ configuration info.
//
// Used in CodaDecoder and THaRunBase.
// Filled in CodaDecoder::daqConfigDecode.
//
//////////////////////////////////////////////////////////////////////////

#include "DAQconfig.h"
#include "Textvars.h"   // for Podd::vsplit
#include "TList.h"
#include <sstream>

using namespace std;

//_____________________________________________________________________________
DAQInfoExtra::DAQInfoExtra()
  : fMinScan(50)
{}

//_____________________________________________________________________________
size_t DAQconfig::parse( size_t i )
{
  // Parse string with index 'i' into key/value pairs stored in 'keyval' map.
  // The key is the first field of a non-comment line. The value is the rest of
  // the line. Whitespace between value fields is collapsed into single spaces.

  if( i >= strings.size() )
    return 0;
  istringstream istr(strings[i]);
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
        keyval.emplace(std::move(key), std::move(val));
    }
  }
  return keyval.size();
}

//_____________________________________________________________________________
void DAQInfoExtra::AddTo( TObject*& p, TObject* obj )
{
  // Add a DAQInfoExtra 'obj' to 'p', which is typically some class's fExtra
  // variable. If p is non-NULL and an object, convert it to a TList
  // which owns its objects and contains the existing object and the new
  // DAQInfoExtra. If it already is a TList, add the DAQInfoExtra object to it.

  auto* ifo = obj ? obj : new DAQInfoExtra;
  if( !p )
    p = ifo;
  else {
    auto* lst = dynamic_cast<TList*>(p);
    if( !lst ) {
      auto* q = p;
      p = lst = new TList;
      lst->SetOwner();
      lst->Add(q);
    }
    lst->Add(ifo);
  }
}

//_____________________________________________________________________________
DAQInfoExtra* DAQInfoExtra::GetExtraInfo( TObject* p )
{
  auto* ifo = dynamic_cast<DAQInfoExtra*>(p);
  if( !ifo ) {
    auto* lst = dynamic_cast<TList*>(p);
    if( lst ) {
      for( auto* obj: *lst ) {
        ifo = dynamic_cast<DAQInfoExtra*>(obj);
        if( ifo )
          break;
      }
    }
  }
  return ifo;
}

//_____________________________________________________________________________
DAQconfig* DAQInfoExtra::GetFrom( TObject* p )
{
  // Get DAQconnfig object from p, which can either be the object or a TList*.

  auto* ifo = GetExtraInfo(p);
  return ifo ? &ifo->fDAQconfig : nullptr;
}

//_____________________________________________________________________________
ClassImp(DAQconfig)
ClassImp(DAQInfoExtra)
