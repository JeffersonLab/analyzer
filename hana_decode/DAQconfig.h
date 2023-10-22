#ifndef Podd_DAQconfig_h_
#define Podd_DAQconfig_h_

//////////////////////////////////////////////////////////////////////////
//
// DAQconfig, DAQInfoExtra
//
// Helper classes to support DAQ configuration info.
//
//////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <map>
#include <utility>  // std::move

//_____________________________________________________________________________
struct DAQconfig {
  DAQconfig() : crate_(0) {}
  DAQconfig( unsigned crate, std::string text )
    : crate_{crate}
    , text_{std::move(text)}
  { parse(); }

  // for std::find
  bool   operator==( unsigned crate ) const        { return crate == crate_; }
  // for std::sort
  bool   operator< ( const DAQconfig& rhs ) const  { return crate_ <  rhs.crate_; }

  void   clear();
  size_t parse();

  unsigned     crate_;
  std::string  text_;
  std::map<std::string, std::string>  keyval_;
} __attribute__((aligned(64)));

//_____________________________________________________________________________
inline void DAQconfig::clear()
{
  crate_ = 0;
  text_.clear(); text_.shrink_to_fit();
  keyval_.clear();
}

//_____________________________________________________________________________
#endif //Podd_DAQconfig_h_
