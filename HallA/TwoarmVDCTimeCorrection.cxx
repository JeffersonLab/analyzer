//*-- Author :    Ole Hansen   18-Oct-19

//////////////////////////////////////////////////////////////////////////
//
// HallA::TwoarmVDCTimeCorrection
//
// Calculates a time correction for the VDC based on a generic formula.
// Runs after Decode.
//
//////////////////////////////////////////////////////////////////////////

#include "TwoarmVDCTimeCorrection.h"
#include "THaAnalyzer.h"

namespace HallA {

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::TwoarmVDCTimeCorrection( const char* name,
                                                  const char* description,
                                                  const char* det1,
                                                  const char* det2 )
  : InterStageModule(name, description, THaAnalyzer::kDecode ),
    fName1(det1), fName2(det2), fDet1(nullptr), fDet2(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::~TwoarmVDCTimeCorrection()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void TwoarmVDCTimeCorrection::Clear( Option_t* opt )
{
  InterStageModule::Clear(opt);

}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::Process( const THaEvData& data )
{
  return 0;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::DefineVariables( THaAnalysisObject::EMode mode )
{
  return THaAnalysisObject::DefineVariables(mode);
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::ReadDatabase( const TDatime& date )
{
  return THaAnalysisObject::ReadDatabase(date);
}

//_____________________________________________________________________________

} // namespace HallA

ClassImp(HallA::TwoarmVDCTimeCorrection)

