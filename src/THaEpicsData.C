// *-- Author :    Chris  Behre   July 2000

//////////////////////////////////////////////////////////////////////////
//
// THaEpicsData
// 
// Decodes some useful EPICS data for Hall A, in particular:
//
//    Beam position readouts Xa/Ya/Xb/Yb ("IPM1H04A.XPOS" etc.)
//    Average current ("hac_bcm_average")
//
// This is an extremely preliminary version of the EPICS data handler.
//
//////////////////////////////////////////////////////////////////////////

#include "THaEpicsData.h"
#include "THaEvData.h"
#include "TDatime.h"
#include "VarDef.h"
#include "VarType.h"

ClassImp(THaEpicsData)

//______________________________________________________________________________
THaEpicsData::THaEpicsData( const char* name, const char* description,
			    THaApparatus* apparatus )
  : THaDetector(name,description,apparatus)
{
  // Constructor
}

//______________________________________________________________________________
THaAnalysisObject::EStatus THaEpicsData::Init( const TDatime& run_time )
{
  // Initialize. 
  // After 30-Nov-2000, use IPM1H04A/B instead of IPM1H03A/B for the positions.

  if( THaDetector::Init( run_time ) )
    return fStatus;

  //FIXME: Get from database
  Int_t date = run_time.GetDate();

  if( date < 20001201 ) {
    fEpicsTags[0] = "IPM1H03A.XPOS";
    fEpicsTags[1] = "IPM1H03A.YPOS";
    fEpicsTags[2] = "IPM1H03B.XPOS";
    fEpicsTags[3] = "IPM1H03B.YPOS";
  } else {
    fEpicsTags[0] = "IPM1H04A.XPOS";
    fEpicsTags[1] = "IPM1H04A.YPOS";
    fEpicsTags[2] = "IPM1H04B.XPOS";
    fEpicsTags[3] = "IPM1H04B.YPOS";
  }
  fEpicsTags[4] = "hac_bcm_average";
    
  return fStatus = kOK;
}


//_____________________________________________________________________________
Int_t THaEpicsData::DefineVariables( EMode mode )
{
  // Define global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "Xposa",  "EPICS Xa position", "fXposa" },
    { "Yposa",  "EPICS Ya position", "fYposa" },
    { "Xposb",  "EPICS Xb position", "fXposb" },
    { "Yposb",  "EPICS Yb position", "fYposb" },
    { "avgcur", "EPICS avg current", "favgcur" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//______________________________________________________________________________
THaEpicsData::~THaEpicsData()
{
  // Destructor.

  RemoveVariables();
}

//______________________________________________________________________________
Int_t THaEpicsData::Decode( const THaEvData& evdata )
{
  // Decode EPICS data.
  // 

  fXposa  =  evdata.GetEpicsData( fEpicsTags[0].Data() );
  fYposa  =  evdata.GetEpicsData( fEpicsTags[1].Data() );
  fXposb  =  evdata.GetEpicsData( fEpicsTags[2].Data() );
  fYposb  =  evdata.GetEpicsData( fEpicsTags[3].Data() );  
  favgcur =  evdata.GetEpicsData( fEpicsTags[4].Data() );

  return 5;
}
 





















