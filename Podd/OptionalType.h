#ifndef Podd_OptionalType_h_
#define Podd_OptionalType_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// optional                                                                  //
//                                                                           //
// Typedefs to simplify std::optional usage                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DataType.h"

#include <optional>
template <typename T>
using Optional_t = std::optional<T>;

using OptInt_t  = Optional_t<Int_t>;
using OptUInt_t = Optional_t<UInt_t>;
using OptData_t = Optional_t<Data_t>;

#endif //Podd_OptionalType_h_
