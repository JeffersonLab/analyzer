////////////////////////////////////////////////////////////////////
//
//   THaEpics
//   Hall A EPICS data.
//
//   EPICS data come in two forms
//     (tag, value)   e.g. (IPM1H04B.XPOS,  0.204)
//   and
//     (tag, value, units)  e.g. (HELG0TSETTLEs, 500, usec)
//
//   All data are received as characters and are parsed.
//   'tags' remain characters, 'values' are either character 
//   or double, and 'units' are characters.
//   Data are stored in an STL map and retrievable by
//   'tag' (e.g. IPM1H04B.XPOS) and by proximity to
//   a physics event number (closest one is picked).
//
//   Replaces THaEpicsStack (obsolete)
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEpics.h"
#include <iostream>
#include <string>
#include <sstream>
#ifdef __GLIBC__
#include "Textvars.h"   // for Podd::Tokenize
#endif

using namespace std;

static int DEBUGL = 0;  // FIXME: -> fDebug member variable

namespace Decoder {

//_____________________________________________________________________________
void EpicsChan::MakeTime()
{
  // Convert dtime string (formatted as "%a %b %e %H:%M:%S %Z %Y", see
  // strftime(3)) to Unix time.

  struct tm ts{};
  string dt{dtime};
  timestamp = -1;
#ifdef __GLIBC__
  // Simple workaround for lack of time zone support in Linux's strptime.
  // This assumes that the current machine's local time zone is the same as
  // that of the time stamp. Doing better than this is complicated since the
  // time stamp encodes the time zone as a letter abbreviation (e.g. "EDT"),
  // not a numerical offset (e.g. -0400), and there seems to be no library
  // support under Linux for looking up the latter from the former.
  vector<string> tok;
  Podd::Tokenize(dtime, " ", tok);
  if( tok.size() != 6 )
    return;
  dt = tok[0]+" "+tok[1]+" "+tok[2]+" "+tok[3]+" "+tok[5];
  const char* r = strptime(dt.c_str(), "%a %b %e %H:%M:%S %Y", &ts);
  ts.tm_isdst = -1;
#else
  const char* r = strptime(dt.c_str(), "%a %b %e %H:%M:%S %Z %Y", &ts);
#endif
  if( !r || r-dt.c_str()-dt.length() != 0 )
    return;
  timestamp = mktime(&ts);
}

//_____________________________________________________________________________
void THaEpics::Print()
{
  cout << "\n\n====================== \n";
  cout << "Print of Epics Data : "<<endl;
  Int_t j = 0;
  for( auto& pm : epicsData ) {
    const vector<EpicsChan>& vepics = pm.second;
    const string& tag = pm.first;
    j++;
    cout << "\n\nEpics Var #" << j;
    cout << "   Var Name =  \""<<tag<<"\""<<endl;
    cout << "Size of epics vector "<<vepics.size();
    for( const auto& chan : vepics ) {
      cout << "\n Tag = " << chan.GetTag();
      cout << "   Evnum = " << chan.GetEvNum();
      cout << "   Date = " << chan.GetDate();
      cout << "   Timestamp = " << chan.GetTimeStamp();
      cout << "   Data = " << chan.GetData();
      cout << "   String = " << chan.GetString();
      cout << "   Units = " << chan.GetUnits();
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
Bool_t THaEpics::IsLoaded(const char* tag) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  return !ep.empty();
}

//_____________________________________________________________________________
Double_t THaEpics::GetData ( const char* tag, UInt_t event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  UInt_t k = FindEvent(ep, event);
  if( k == kMaxUInt ) return 0;
  return ep[k].GetData();
}

//_____________________________________________________________________________
string THaEpics::GetString ( const char* tag, UInt_t event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  UInt_t k = FindEvent(ep, event);
  if( k == kMaxUInt ) return "";
  return ep[k].GetString();
}

//_____________________________________________________________________________
time_t THaEpics::GetTimeStamp( const char* tag, UInt_t event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  UInt_t k = FindEvent(ep, event);
  if( k == kMaxUInt ) return 0;
  return ep[k].GetTimeStamp();
}

//_____________________________________________________________________________
vector<EpicsChan> THaEpics::GetChan(const char *tag) const
{
  // Return the vector of Epics data for 'tag' 
  // where 'tag' is the name of the Epics variable.
  auto pm = epicsData.find(string(tag));
  if (pm != epicsData.end())
    return pm->second;
  else
    return {};
}

//_____________________________________________________________________________
UInt_t THaEpics::FindEvent( const vector<EpicsChan>& ep, UInt_t event )
{
  // Return the index in the vector of Epics data 
  // nearest in event number to event 'event'.
  if (ep.empty())
    return kMaxUInt;
  UInt_t myidx = ep.size()-1;
  if (event == 0) return myidx;  // return last event 
  Long64_t min = 9999999;
  for( size_t k = 0; k < ep.size(); k++ ) {
    Long64_t diff = event - ep[k].GetEvNum();
    if( diff < 0 ) diff = -diff;
    if( diff < min ) {
      min = diff;
      myidx = k;
    }
  }
  return myidx;
}

//_____________________________________________________________________________
int THaEpics::LoadData( const UInt_t* evbuffer, UInt_t event)
{ 
  // load data from the event buffer 'evbuffer' 
  // for event nearest 'evnum'.

  const string::size_type MAX_VAL_LEN = 32;

  const char* cbuff = (const char*)evbuffer;
  size_t len = sizeof(UInt_t)*(evbuffer[0]+1);
  if (DEBUGL>1) cout << "Enter loadData, len = "<<len<<endl;
  if( len<16 ) return 0;
  // The first 16 bytes of the buffer are the event header
  len -= 16;
  cbuff += 16;
  
  // Copy data to internal string buffer
  string buf( cbuff, len );

  // The first line is the time stamp
  string date;
  istringstream ib(buf);
  if( !getline(ib,date) || date.size() < 16 ) {
    cerr << "Invalid time stamp for EPICS event at evnum = " << event << endl;
    return 0;
  }
  if(DEBUGL>1) cout << "Timestamp: " << date <<endl;

  string line;
  istringstream il, iv;
  while( getline(ib,line) ) {
    // Here we parse each line
    if(DEBUGL>2) cout << "epics line : "<<line<<endl;
    il.clear(); il.str(line);
    string wtag, wval, wunits;
    il >> wtag;
    if( wtag.empty() || wtag[0] == 0 ) continue;
    istringstream::pos_type spos = il.tellg();
    il >> wval;
    Double_t dval = 0;
    bool got_val = false;
    if( !wval.empty() && wval.length() <= MAX_VAL_LEN ) {
      iv.clear(); iv.str(wval);
      if( iv >> dval )
        got_val = true;
    }
    if( got_val ) {
      il >> wunits; // Assumes that wunits contains no whitespace
    } else {
      // Mimic the old behavior: if the string doesn't convert to a number,
      // then wval = rest of string after tag, dval = 0, sunit = empty
      string::size_type lpos = line.find_first_not_of(" \t",spos);
      wval = ( lpos != string::npos ) ? line.substr(lpos) : "";
      dval = 0;
    }
    if(DEBUGL>2) cout << "wtag = "<<wtag<<"   wval = "<<wval
		      << "   dval = "<<dval<<"   wunits = "<<wunits<<endl;

    // Add tag/value/units to the EPICS data.    
    epicsData[wtag].push_back( EpicsChan(wtag, date, event, wval, wunits, dval) );
  }
  if(DEBUGL) Print();
  return 1;
}

}

ClassImp(Decoder::THaEpics)
