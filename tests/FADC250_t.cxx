//*-- Author :    Ole Hansen   16-Apr-26
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// FADC250_t                                                                 //
//                                                                           //
// Test Fadc250Module                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "Fadc250Module.h"
#include "PoddTests.h"
#include "TError.h"                      // for gErrorIgnoreLevel, kBreak
#include <algorithm>                     // for find
#include <iostream>                      // for cout, streambuf, stringstream
#include <memory>                        // for unique_ptr, make_unique
#include <sstream>                       // for stringstream, stringbuf
#include <string>                        // for string, to_string
#include <string_view>                   // for operator""sv, string_view
#include <vector>                        // for vector, operator==
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iterator>

using namespace std;
namespace fs = std::filesystem;

// namespace {
// struct cout_redirect { // NOLINT(*-special-member-functions)
//   explicit cout_redirect( std::streambuf* newbuf )
//     : savebuf_{std::cout.rdbuf(newbuf)} {}
//   ~cout_redirect() { std::cout.rdbuf(savebuf_); }
// private:
//   std::streambuf* savebuf_;
// };
// }

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("FADC250 decoding", "[Decoder]") // NOLINT(*-function-cognitive-complexity)
{
  // Read example event data
  fs::path fpath = fs::path(PODD_TESTS_DIR) / "fadc250.txt";
  ifstream istr(fpath);
  REQUIRE(istr.is_open());

  vector<uint32_t> evbuf;
  evbuf.reserve(256);
  copy(istream_iterator<uint32_t>(istr >> hex),
       istream_iterator<uint32_t>(), back_inserter(evbuf));
  istr.close();
  size_t nwords = evbuf.size();
  REQUIRE(nwords == 133);

  // Create test object
  auto fadc3  = make_unique<Decoder::Fadc250Module>(6,3);
  auto fadc17 = make_unique<Decoder::Fadc250Module>(6,17);
  fadc3-> Init("debug=2");
  fadc17->Init("debug=2");

  // Decode module in slot 3
  auto ret = fadc3->LoadBank(nullptr, evbuf.data(), 0, evbuf.size());
  CHECK(ret == 66 );
  //TODO check data (samples, time stamp)

  ret = fadc17->LoadBank(nullptr, evbuf.data(), 0, evbuf.size());
  CHECK(ret == 66 );
  //TODO check data (samples, time stamp)

}
