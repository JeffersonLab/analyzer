//*-- Author :    Ole Hansen   24-Feb-03

#ifndef ROOT_THaScalerGroup
#define ROOT_THaScalerGroup

/////////////////////////////////////////////////////////////////////
//
//   THaScalerGroup
//
/////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "THaScaler.h"

class THaScalerGroup : public THaAnalysisObject {

public:

  THaScalerGroup( const char* Bankgroup );
  virtual ~THaScalerGroup();

  virtual EStatus Init( const TDatime& date );

  Int_t LoadData(const THaEvData& evdata)
    { return fScaler ? fScaler->LoadData( evdata ) : SCAL_ERROR; }

  THaScaler*   GetScalerObj()   { return fScaler; }
  virtual void PrintSummary()   { if( fScaler ) fScaler->PrintSummary(); }

protected:

  THaScaler* fScaler;    // The actual scaler object

  virtual void MakePrefix() { THaAnalysisObject::MakePrefix( NULL ); }

  ClassDef(THaScalerGroup,0)  // Scaler data
};

#endif
