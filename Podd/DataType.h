#ifndef Podd_DataType_h_
#define Podd_DataType_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Default data type for physics quantities                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <type_traits>

using Data_t = Double_t;   // Currently supported types: Float_t, Double_t

const Data_t kBig  = 1.e38; // default junk value

static_assert( std::is_same<Data_t, Float_t>::value ||
               std::is_same<Data_t, Double_t>::value,
               "Data_t must be Float_t or Double_t" );

#endif //Podd_DataType_h_
