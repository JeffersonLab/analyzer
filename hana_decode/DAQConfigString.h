#ifndef Podd_DAQConfigString_h_
#define Podd_DAQConfigString_h_

//////////////////////////////////////////////////////////////////////////
//
// DAQConfigString
//
// Helper class to support DAQ configuration info.
//
//////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <utility>  // std::move

//_____________________________________________________________________________
struct DAQConfigString {
  DAQConfigString() : evnum_{0}, evtyp_{0}, crate_{0} {}
  DAQConfigString( uint64_t evnum, uint32_t evtyp, uint32_t crate,
                   std::string text )
    : evnum_{evnum}
    , evtyp_{evtyp}
    , crate_{crate}
    , text_{std::move(text)}
  { parse(); }

  bool operator==( const DAQConfigString& rhs ) const;
  void   clear();
  size_t parse();

  uint64_t     evnum_;    // (Raw) event number holding these data
  uint32_t     evtyp_;    // DAQ event type
  uint32_t     crate_;    // Crate associated with these data
  std::string  text_;     // Raw text
  std::multimap<std::string, std::string>  keyval_; // Parsed text
} __attribute__((aligned(64)));

//_____________________________________________________________________________
inline bool DAQConfigString::operator==( const DAQConfigString& rhs ) const
{
  return evnum_ == rhs.evnum_ && evtyp_ == rhs.evtyp_ && crate_ == rhs.crate_
         && text_ == rhs.text_;
}

//_____________________________________________________________________________
inline void DAQConfigString::clear()
{
  crate_ = evtyp_ = evnum_ = 0;
  text_.clear(); text_.shrink_to_fit();
  keyval_.clear();
}

//_____________________________________________________________________________
#endif //Podd_DAQConfigString_h_
