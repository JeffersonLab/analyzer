#ifndef Podd_OptionalType_h_
#define Podd_OptionalType_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::optional                                                            //
//                                                                           //
// Support for optional type for pre-C++17 compilers                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DataType.h"

#if __cplusplus >= 201703L
#include <optional>
template <typename T>
using Optional_t = std::optional<T>;
#else
#include "optional.hpp"
template <typename T>
using Optional_t = std::experimental::optional<T>;
using std::experimental::nullopt;
using std::experimental::make_optional;

#endif /* __cplusplus >= 201703L */

using OptInt_t  = Optional_t<Int_t>;
using OptUInt_t = Optional_t<UInt_t>;
using OptData_t = Optional_t<Data_t>;

#endif //Podd_OptionalType_h_
