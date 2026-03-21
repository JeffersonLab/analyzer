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
#else
# include <catch2/catch.hpp>
#endif

#include "Textvars.h"                    // for Textvars, Tokenize, Trim, Rtypes
#include "TError.h"                      // for gErrorIgnoreLevel, kBreak
#include <algorithm>                     // for find
#include <iostream>                      // for cout, streambuf, stringstream
#include <memory>                        // for unique_ptr, make_unique
#include <sstream>                       // for stringstream, stringbuf
#include <string>                        // for string, to_string
#include <string_view>                   // for operator""sv, string_view
#include <vector>                        // for vector, operator==

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

TEST_CASE("Textvars string substitution", "[Utilities]") // NOLINT(*-function-cognitive-complexity)
{
  // Use private instance
  auto tvars = make_unique<Podd::Textvars>();

  // Fill replacement map with a handful of examples to test
  tvars->Add("key1", "value1");
  tvars->Add("ind1", "key1");
  tvars->AddVerbatim("key2", "value21, value22, value23  ");
  tvars->Add("key3", "value31,value32,value33");
  tvars->Add("x","xval");
  // Set() is just a synonym for Add()
  tvars->Set("key4", "value41, value42 ,   value43,a word here,value45  ");

  const vector<string> refs =
  {"value41", " value42 ", "   value43", "a word here", "value45  "};

  SECTION("Construction") {
    REQUIRE(tvars->Size() == 6);
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

    // Multiple replacements
    s = "multiples ${ind1}${${ind1}} ${x} ${key2} ";
    ret = tvars->Substitute(s);
    CHECK(ret == 0);
    CHECK(s == "multiples key1value1 xval value21, value22, value23   ");

    // Incomplete key delimiters
    s = "nothing replaced ${junk$$ more $hello";
    ret = tvars->Substitute(s);
    CHECK(ret == 0);
    CHECK(s == "nothing replaced ${junk$$ more $hello");

    // Multiple lines
    vector<string> lines = {
      "# Test data",
      "",
      "key1 = ${key1}",
      "key1 = ${${ind1}} and more",
      "",
      "# End of test",
    };
    auto ref = lines;
    ref[2] = "key1 = value1";
    ref[3] = "key1 = value1 and more";
    ret = tvars->Substitute(lines);
    CHECK(ret == 0);
    CHECK(lines.size() == 6);
    CHECK(lines == ref);
  }

  SECTION("Multi-value substitution") {
    // Multiple replacements for single key
    vector<string> lines{"key3 = ${key3}"};
    auto ret = tvars->Substitute(lines);
    CHECK(ret == 0);
    CHECK(lines.size() == 3);
    for( size_t i = 1; const auto& s: lines) {
      const string ref = "key3 = value3" + to_string(i);
      CHECK(s == ref);
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
    CHECK(tmap.contains("key2"));
    CHECK_FALSE(tmap.contains(" key1"));
    auto vals3 = tvars->GetArray("key4");
    for( size_t i = 0; const auto& s: vals3 ) {
      CHECK(s == refs[i]);
      ++i;
    }
    CHECK(tvars->GetArray("nokey").empty());
    auto names = tvars->GetNames();
    CHECK(names.size() == tvars->Size());
    CHECK(ranges::find(names, "ind1") != names.end() );
  }

  SECTION("Miscellaneous") {
    // Print()
    { string refstr = R"(Textvar:  ind1 = "key1"
Textvar:  key1 = "value1"
Textvar:  key2 = "value21, value22, value23  "
Textvar:  key3 = "value31","value32","value33"
Textvar:  key4 = "value41"," value42 ","   value43","a word here","value45  "
Textvar:     x = "xval"
)";
      stringstream buf;
      cout_redirect guard(buf.rdbuf());
      tvars->Print();
      CHECK(buf.str() == refstr);
    }

    // Remove()
    auto val1 = tvars->Remove("key1");
    CHECK(tvars->Size() == 5);
    CHECK(val1.size() == 1);
    CHECK(val1[0] == "value1");

    // Clear()
    tvars->Clear();
    CHECK(tvars->Size() == 0);
    CHECK(tvars->GetNames().empty());
    CHECK(tvars->GetArray("key2").empty());
  }

  SECTION("Failures")
  {
    // Suppress ROOT error message. TODO capture and check messages
    gErrorIgnoreLevel = kBreak;

    // Degenerate case: Empty replacement
    string s = "empty key ${key1}${}${key2}";
    string ref = s;
    Int_t ret = tvars->Substitute(s);
    CHECK(ret != 0);
    CHECK(s == ref); // unchanged

    // Nested empty
    s = "empty = ${${}}";
    ref = s;
    ret = tvars->Substitute(s);
    CHECK(ret != 0);
    CHECK(s == ref); // unchanged
  }
}

TEST_CASE("Textvars string utilities", "[Utilities]") // NOLINT(*-function-cognitive-complexity)
{
  SECTION("Tokenize")
  {
    vector<string> tokens;
    const vector<string> ref1{"a","b","c"};
    Podd::Tokenize("",",",tokens);
    CHECK(tokens.empty());
    Podd::Tokenize("a,b,c", ",", tokens);
    CHECK(tokens.size() == 3);
    CHECK(tokens == ref1);
    Podd::Tokenize("a,,b,c", ",", tokens);
    CHECK(tokens.size() == 3);
    CHECK(tokens == ref1);
    Podd::Tokenize("a,b,c,", ",", tokens);
    CHECK(tokens.size() == 3);
    CHECK(tokens == ref1);
    Podd::Tokenize("a,b,c,,", ",", tokens);
    CHECK(tokens.size() == 3);
    CHECK(tokens == ref1);
    Podd::Tokenize("a:b,;c::", ",:;", tokens);
    CHECK(tokens.size() == 3);
    CHECK(tokens == ref1);
    Podd::Tokenize("a,  ,b,c", ",", tokens);
    CHECK(tokens.size() == 4);
    CHECK(tokens == vector<string>({"a","  ", "b","c"}));

    vector<string_view> svtokens;
    const vector<string_view> ref2{"hello"sv, "my"sv, "dear"sv, "friend"sv};
    Podd::Tokenize("hello my dear friend", " \t\n\r", svtokens);
    CHECK(svtokens.size() == 4);
    CHECK(svtokens == ref2);
    Podd::Tokenize("\thello   \rmy\r dear friend\n\n", " \t\n\r", svtokens);
    CHECK(svtokens.size() == 4);
    CHECK(svtokens == ref2);
  }

  SECTION("Trim") {
    // Podd::Trim()
    string s;
    Podd::Trim(s);
    CHECK(s.empty());
    s = "word";
    Podd::Trim(s);
    CHECK(s == "word");
    s = "word    ";
    Podd::Trim(s);
    CHECK(s == "word");
    s = "word\n";
    Podd::Trim(s);
    CHECK(s == "word");
    s = "\tword";
    Podd::Trim(s);
    CHECK(s == "word");
    s = "\n   word";
    Podd::Trim(s);
    CHECK(s == "word");
    s = " many     words   here ";
    Podd::Trim(s);
    CHECK(s == "many     words   here");
  }
}
