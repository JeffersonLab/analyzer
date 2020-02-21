#ifndef Podd_OptionalType_h_
#define Podd_OptionalType_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::optional                                                            //
//                                                                           //
// Support for optional type for pre-C++17 compilers                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if __cplusplus >= 201701L
#include <optional>
template <typename T>
using Optional_t = std::optional<T>;
#else
#include "optional.hpp"
template <typename T>
using Optional_t = std::experimental::optional<T>;
using std::experimental::nullopt;
using std::experimental::make_optional;

#endif /* __cplusplus >= 201701L */

#endif //Podd_OptionalType_h_
