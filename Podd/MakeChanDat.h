//*-- Author :    Ole Hansen   08-Dec-25
//

#ifndef PODD_MAKECHANDAT_H
#define PODD_MAKECHANDAT_H

#include "THaDetectorBase.h" // includes ChannelData.h, THaAnalysisObject.h
#include <utility>
#include <memory>

namespace Podd {

//_____________________________________________________________________________
template<class ChanDat>
std::pair<std::unique_ptr<ChanDat>, Int_t>
MakeChanDat( const TDatime& date, THaDetectorBase* det )
{
  // Instantiate a ChanDat object and read its configuration from the database.
  // 'date' is the initialization time, 'det', the owning detector class.
  // Return value:
  //  ret.first:   unique_ptr to ChanDat object
  //  ret.second:  return value from ReadConfig (a ReadDatabase EStatus)

  std::pair<std::unique_ptr<ChanDat>, Int_t> ret{nullptr, THaAnalysisObject::kFileError};
  if( !det )
    return ret;

  // Create new ChanDat object
  auto chandat = std::make_unique<ChanDat>(
    det->GetPrefixName().Data(), det->GetTitle(), det->GetNelem());

  // Configure ChanDat using the detector's database. However, don't bother
  // if the concrete ChanDat class doesn't implement its own database reader.
  if constexpr (!std::is_same_v
    <decltype(&ChanDat::ReadConfig),decltype(&Podd::ChannelData::ReadConfig)>)
  {
    FILE* file = det->OpenFile(date);
    if( !file )
      return ret;
    auto err = chandat->ReadConfig(file, date, det->GetPrefix());
    (void) fclose(file);
    if( err ) {
      ret.second = err;
      return ret;
    }
  }

  return {std::move(chandat), THaAnalysisObject::kOK};
}

} // namespace Podd

#endif //PODD_MAKECHANDAT_H
