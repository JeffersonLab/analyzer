//*-- Author :    Ole Hansen   26-Nov-25
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Textvars_t                                                                //
//                                                                           //
// Test Podd::Textvars                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
# include <catch2/generators/catch_generators.hpp>
# include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "Textvars.h"
#include <string>
#include <utility>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

namespace {
struct cout_redirect { // NOLINT(*-special-member-functions)
  explicit cout_redirect( std::streambuf* newbuf )
    : savebuf_{std::cout.rdbuf(newbuf)} {}
  ~cout_redirect() { std::cout.rdbuf(savebuf_); }
private:
  std::streambuf* savebuf_;
};
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Textvars string substitution", "[Database]") // NOLINT(*-function-cognitive-complexity)
{
  // Use private instance
  auto tvars = make_unique<Podd::Textvars>();

  // Fill replacement map with a handful of examples to test
  using Param = pair<string,string>;
  tvars->Add("key1", "value1");
  tvars->Add("ind1", "key1");
  tvars->AddVerbatim("key2", "value21, value22, value23  ");
  tvars->Add("key3", "value31,value32,value33");
  // Set() is just a synonym for Add()
  tvars->Set("key4", "value41, value42 ,   value43,a word here,value45  ");

  const vector<string> refs =
    {"value41", " value42 ", "   value43", "a word here", "value45  "};

  SECTION("Construction") {
    REQUIRE(tvars->Size() == 5);
  }

  SECTION("Single-value substitution") {
    // Basic replacement
    string s = "key1 = ${key1}";
    auto ret = tvars->Substitute(s);
    CHECK(ret == 0);
    CHECK(s == "key1 = value1");

    // Verbatim replacement string containing commas
    s = "key2 = ${key2}";
    ret = tvars->Substitute(s);
    CHECK(ret == 0);
    CHECK(s == "key2 = value21, value22, value23  ");

    // Nested replacement
    s = "nested key1 = ${${ind1}}";
    ret = tvars->Substitute(s);
    CHECK(ret == 0);
    CHECK(s == "nested key1 = value1");
  }

  SECTION("Multi-value substitution") {
    // Multiple replacements for single key
    vector<string> lines{"key3 = ${key3}"};
    auto ret = tvars->Substitute(lines);
    CHECK(ret == 0);
    CHECK(lines.size() == 3);
    for( size_t i = 1; const auto& s: lines) {
      const string ref = "key3 = value3";
      CHECK(s == ref + to_string(i));
      ++i;
    }

    // Check if whitespace preserved
    lines = {"key4 = ${key4}"};
    ret = tvars->Substitute(lines);
    CHECK(ret == 0);
    CHECK(lines.size() == 5);
    for( size_t i = 0; const auto& s: lines) {
      const string ref = "key4 = " + refs[i];
      CHECK(s == ref);
      ++i;
    }
  }

  SECTION("Get methods") {
    CHECK(tvars->Get("key1") == string("value1"));
    CHECK(tvars->GetNvalues("nosuch") == 0);
    CHECK(tvars->GetNvalues("key3") == 3);
    CHECK(tvars->Get("key3",2) == string("value33"));
    CHECK(tvars->Get("key3",3) == nullptr);
    const auto& tmap = tvars->GetAllStringsMap();
    CHECK(tmap.size() == tvars->Size());
    CHECK(tmap.find("key2") != tmap.end());
    CHECK_FALSE(tmap.find(" key1") != tmap.end());
    auto vals3 = tvars->GetArray("key4");
    for( size_t i = 0; const auto& s: vals3 ) {
      CHECK(s == refs[i]);
      ++i;
    }
    CHECK(tvars->GetArray("nokey").empty());
    auto names = tvars->GetNames();
    CHECK(names.size() == tvars->Size());
    CHECK(std::find(names.begin(), names.end(), "ind1") != names.end() );
  }

  SECTION("Miscellaneous") {
    // Print()
    { string refstr = R"(Textvar:  ind1 = "key1"
Textvar:  key1 = "value1"
Textvar:  key2 = "value21, value22, value23  "
Textvar:  key3 = "value31","value32","value33"
Textvar:  key4 = "value41"," value42 ","   value43","a word here","value45  "
)";
      stringstream buf;
      cout_redirect guard(buf.rdbuf());
      tvars->Print();
      CHECK(buf.str() == refstr);
    }

    // Remove()
    auto val1 = tvars->Remove("key1");
    CHECK(tvars->Size() == 4);
    CHECK(val1.size() == 1);
    CHECK(val1[0] == "value1");

    // Clear()
    tvars->Clear();
    CHECK(tvars->Size() == 0);
    CHECK(tvars->GetNames().empty());
    CHECK(tvars->GetArray("key2").empty());
  }
}
