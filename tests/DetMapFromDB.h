#ifndef Podd_Tests_DetMapFromDB_h_
#define Podd_Tests_DetMapFromDB_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::DetMapFromDB test object for DetMap_t                        //
//                                                                           //
// Author: Ole Hansen 22-Mar-2026                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"  // for Rtypes, ClassDefOverride
class TDatime;

namespace Podd::Tests {

class DetMapFromDB : public THaDetector {

public:
  explicit DetMapFromDB( const char* name,
    const char* description = "Read detector map from file test" );
  DetMapFromDB() = default;
  ~DetMapFromDB() override;

  Int_t GetNumber() const { return fNumber; }

  DetMapFromDB( const DetMapFromDB& rhs ) = delete;
  DetMapFromDB( DetMapFromDB&& rhs ) = delete;
  DetMapFromDB& operator=( const DetMapFromDB& rhs ) = delete;
  DetMapFromDB& operator=( DetMapFromDB&& rhs ) = delete;

protected:
  Int_t fNumber{-1};  // some random number

  Int_t ReadDatabase( const TDatime& date ) override;
  // Do not read the run database
  Int_t ReadRunDatabase( const TDatime& ) override { return kOK; }

  ClassDefOverride(DetMapFromDB,1)  // Pseudo detetor for testing reading of detector map
};

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

#endif
