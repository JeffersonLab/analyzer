#ifndef Podd_DAQConfigString_h_
#define Podd_DAQConfigString_h_

//////////////////////////////////////////////////////////////////////////
//
// DAQConfigString
//
// Helper class to support DAQ configuration info.
//
//////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <map>
#include <utility>  // std::move

//_____________________________________________________________________________
struct DAQConfigString {
  DAQConfigString() : crate_{0}, evtyp_{0} {}
  DAQConfigString( unsigned crate, std::string text )
    : crate_{crate}
    , evtyp_{0}
    , text_{std::move(text)}
  { parse(); }

  void   clear();
  size_t parse();

  unsigned     crate_;
  unsigned     evtyp_;
  std::string  text_;
  std::map<std::string, std::string>  keyval_;
} __attribute__((aligned(64)));

//_____________________________________________________________________________
inline void DAQConfigString::clear()
{
  crate_ = evtyp_ = 0;
  text_.clear(); text_.shrink_to_fit();
  keyval_.clear();
}

//_____________________________________________________________________________
#endif //Podd_DAQConfigString_h_
