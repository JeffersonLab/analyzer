////////////////////////////////////////////////////////////////////
//
//   UserEvtHandler
//   author  Robert Michaels (rom@jlab.org), Jan 2017
//
//   Example of an Event Type Handler.
//   This class handles events of the type specified in the vector eventtypes.
//   It was tested on some EPICS data in hall A, and so it is a primitive
//   EPICS event type handler.
//
//   The idea would be to copy this and modify it for your purposes.
//
//   To use as a plugin with your own modifications, you can do this in
//   your analysis script
//     gHaEvtHandlers->Add (new UserEvtHandler("example1","for evtype 131"));
//
//   The data specified in the dataKeys array appear in Podd's global variables.
//   and can be sent to the ROOT output, and plotted or analyzed from the
//   ROOT Tree "T".
//   Example:  T->Draw("IPM1H04B.YPOS:fEvtHdr.fEvtNum")
//
/////////////////////////////////////////////////////////////////////

#include "UserEvtHandler.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include <cstdlib>   // for atof
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;

static const Int_t MAXDATA=20000;

UserEvtHandler::UserEvtHandler(const char *name, const char* description)
  : THaEvtTypeHandler(name,description), dvars(0)
{
}

UserEvtHandler::~UserEvtHandler()
{
}

// GetData is a public method which other classes may use
Float_t UserEvtHandler::GetData(const std::string& tag) const
{
  map<string, Float_t>::const_iterator elem = theDataMap.find(tag);
  if (elem == theDataMap.end())
    return kBig;

  return elem->second;
}


Int_t UserEvtHandler::Analyze(THaEvData *evdata)
{

  assert( fStatus == kOK );  // should never get here if Init() failed
  assert( dataKeys.size() == 0 || dvars != 0 ); // else logic error in Init()

#ifdef WITH_DEBUG
  if( fDebug < 1 ) fDebug = 1; // force debug messages for now (testing)
#endif

  if ( !IsMyEvent(evdata->GetEvType()) )
    return -1;

  UInt_t evbuffer[MAXDATA];

  if (evdata->GetEvLength() >= MAXDATA)  {
    Error( Here("UserEvtHandler::Analyze"), "need a bigger buffer!" );
    return 1;
  }

// Copy the buffer.  If the events are infrequent this causes no harm.
  for (Int_t i = 0; i < evdata->GetEvLength(); i++)
	 evbuffer[i] = evdata->GetRawData(i);

  char* cbuff = (char*)evbuffer;
  size_t len = sizeof(int)*(evbuffer[0]+1);
#ifdef WITH_DEBUG
  if (fDebug>1) cout << "Evt Handler Data, len = "<<len<<endl;
#endif
  if( len<16 )
    return 0;
  // The first 16 bytes of the buffer are the event header
  len -= 16;
  cbuff += 16;

  // Copy data to internal string buffer
  string buf( cbuff, len );

  // The first line is the time stamp
  string date;
  istringstream ib(buf);

  string line;
  while( getline(ib,line) ) {
#ifdef WITH_DEBUG
    if(fDebug) cout << "data line : "<<line<<endl;
#endif
    istringstream il(line);
    string wtag, wval, sunit;
    il >> wtag;
    if( wtag.empty() || wtag[0] == 0 ) continue;
    istringstream::pos_type spos = il.tellg();
    il >> wval >> sunit;
    Double_t dval;
    istringstream iv(wval);
    if( !(iv >> dval) ) {
      string::size_type lpos = line.find_first_not_of(" \t",spos);
      wval = ( lpos != string::npos ) ? line.substr(lpos) : "";
      dval = 0;
      sunit.clear();
    }
#ifdef WITH_DEBUG
    if(fDebug) cout << "wtag = "<<wtag<<"   wval = "<<wval
		      << "   dval = "<<dval<<"   sunit = "<<sunit<<endl;
#endif
    if (theDataMap.find(wtag) != theDataMap.end())
	 theDataMap[wtag] = (Float_t)atof(wval.c_str());

  }

  // Fill global variables
#ifdef WITH_DEBUG
  if (fDebug) cout << "---------------------------------------------"<<endl<<endl;
#endif

  for (UInt_t i=0; i < dataKeys.size(); i++) {
    dvars[i] = GetData(dataKeys[i]);

#ifdef WITH_DEBUG
    if (fDebug)
      cout << "GetData ::  key " << i << "   " << dataKeys[i]
	   << "   data = " << GetData(dataKeys[i])
	   << endl;
#endif
  }

  return 1;
}


THaAnalysisObject::EStatus UserEvtHandler::Init(const TDatime& /* date */ )
{

#ifdef WITH_DEBUG
  if( fDebug>1 )
    cout << "Howdy !  We are initializing UserEvtHandler !!   name =   "
	 <<fName<<endl;
#endif

  eventtypes.push_back(131);  // what events to look for

  // data keys to look for in this fun example
  dataKeys.push_back("hac_bcm_average");
  dataKeys.push_back("HacL_Q2_HP3458A:IOUT");
  dataKeys.push_back("hac_bcm_dvm2_read");
  dataKeys.push_back("MCZ1H04VM");
  dataKeys.push_back("IPM1H04B.YPOS");

  // initialize map elements to -1 (means not found yet)
  for (UInt_t i=0; i < dataKeys.size(); i++) {
    if(theDataMap.insert(make_pair(dataKeys[i],-1)).second == false) {
      Warning( Here("UserEvtHandler::Init"), "Element %s already defined",
	       dataKeys[i].c_str() );
    }
  }

  // After adding the dataKeys to the global variables (using lines below)
  // one can obtain them in the ROOT output using lines like this in the
  // output definition file.  like, T->Draw("IPM1H04B.YPOS:fEvtHdr.fEvtNum")
  // (Careful:  variables with ":" in the name don't plot well, i.e. T->Draw()
  // has a problem with the ":".  Also arithmetic characters.)
  //    variable hac_bcm_average
  //    variable IPM1H04B.YPOS

  UInt_t Nvars = dataKeys.size();
  if (Nvars > 0) {
    dvars = new Double_t[Nvars];  // dvars is a member of this class
		 // the index of the dvars array tracks the index of dataKeys
    memset(dvars, 0, Nvars*sizeof(Double_t));
    if (gHaVars) {
#ifdef WITH_DEBUG
      if( fDebug>1 )
	cout << "EvtHandler:: Have gHaVars.  Good thing. "<<gHaVars<<endl;
#endif
    } else {
      Error( Here("UserEvtHandler::Init"),
	     "No gHaVars ?!  Well, that is a problem !!" );
      return kInitError;
    }
    const Int_t* count = 0;
    for (UInt_t i = 0; i < Nvars; i++) {
       gHaVars->DefineByType(dataKeys[i].c_str(), "epics data",
			  &dvars[i], kDouble, count);
    }
  }

  fStatus = kOK;
  return kOK;
}

ClassImp(UserEvtHandler)
