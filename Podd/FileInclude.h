#ifndef Podd_FileInclude_h_
#define Podd_FileInclude_h_

//////////////////////////////////////////////////////////////////////////
//
// FileInclude.h
//
// Helper functions for supporting '#include' directives in
// configuration files
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>
#include <string_view>

namespace Podd {

  inline constexpr std::string_view kIncTag     = "#include";
  inline constexpr std::string_view kWhiteSpace = " \t";

  Int_t GetIncludeFileName( const std::string& line, std::string& incfile );
  Int_t CheckIncludeFilePath( std::string& incfile );

}

#endif
