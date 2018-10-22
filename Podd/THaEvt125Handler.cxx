////////////////////////////////////////////////////////////////////
//
//   THaEvt125Handler
//   author  Robert Michaels (rom@jlab.org), Jan 2017
//
//   Example of an Event Type Handler for event type 125,
//   the hall C prestart event.
//
//   It was tested on some hms data.  However, note that I don't know what 
//   these data mean yet and I presume the data structure is under development; 
//   someone will need to modify this class (and the comments).
//
//   To use as a plugin with your own modifications, you can do this in
//   your analysis script
//    
//   gHaEvtHandlers->Add (new THaEvt125Handler("hallcpre","for evtype 125"));
//
//   Global variables are defined in Init.  You can see them in Podd, as
//     analyzer [2] gHaVars->Print()
//
//      OBJ: THaVar	HCvar1	Hall C event type 125 variable 1
//      OBJ: THaVar	HCvar2	Hall C event type 125 variable 2
//      OBJ: THaVar	HCvar3	Hall C event type 125 variable 3
//      OBJ: THaVar	HCvar4	Hall C event type 125 variable 4
//
//   The data can be added to the ROOT Tree "T" using the output
//   definition file.  Then you can see them, for example as follows
// 
//      T->Scan("HCvar1:HCvar2:HCvar3:HCvar4")
//
/////////////////////////////////////////////////////////////////////

#include "THaEvt125Handler.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

THaEvt125Handler::THaEvt125Handler(const char *name, const char* description)
  : THaEvtTypeHandler(name,description), NVars(0), dvars(0)
{
}

THaEvt125Handler::~THaEvt125Handler()
{
}

Float_t THaEvt125Handler::GetData(const std::string& tag)
{
  // A public method which other classes may use
  if (theDataMap.find(tag) == theDataMap.end()) 
    return 0;
  return theDataMap[tag];
}


Int_t THaEvt125Handler::Analyze(THaEvData *evdata) 
{

  Bool_t ldebug = true;  // FIXME: use fDebug
  UInt_t index;
  const Int_t startidx = 3;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  if (ldebug) cout << "------------------\n  Event type 125 \n\n"<<endl;

  for (Int_t i = 0; i < evdata->GetEvLength(); i++) {

    if (ldebug) cout << "data["<<dec<<i<<"] =  0x"<<hex<<evdata->GetRawData(i)<<"  = decimal "<<dec<<evdata->GetRawData(i)<<endl;

// This is a fake example of how to decode.  Modify it as you wish,
// and then change these comments.
// The data in "dvars" appears as global variables.

    if (i >= startidx) {
      index = i-startidx;
      if (index < NVars) dvars[index] = evdata->GetRawData(i);
    }

  }      

  return 1;
}


THaAnalysisObject::EStatus THaEvt125Handler::Init(const TDatime&)
{

  cout << "Howdy !  We are initializing THaEvt125Handler !!   name =   "<<fName<<endl;

  eventtypes.push_back(125);  // what events to look for

// dvars is a member of this class.
// The index of the dvars array track the array of global variables created.
// This is just a fake example with 4 variables.
// Please change these comments when you modify this class.

  NVars = 4;
  dvars = new Double_t[NVars];
  memset(dvars, 0, NVars*sizeof(Double_t));
  if (gHaVars) {
      cout << "EvtHandler:: Have gHaVars.  Good thing. "<<gHaVars<<endl;
  } else {
      cout << "EvtHandler:: No gHaVars ?!  Well, that is a problem !!"<<endl;
      return kInitError;
  }
  const Int_t* count = 0;
  char cname[80];
  char cdescription[80];
  for (UInt_t i = 0; i < NVars; i++) {
    sprintf(cname,"HCvar%d",i+1);
    sprintf(cdescription,"Hall C event type 125 variable %d",i+1);
    gHaVars->DefineByType(cname, cdescription, &dvars[i], kDouble, count);
  }


  fStatus = kOK;
  return kOK;
}

ClassImp(THaEvt125Handler)
