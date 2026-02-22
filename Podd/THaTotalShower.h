#ifndef Podd_THaTotalShower_h_
#define Podd_THaTotalShower_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTotalShower                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"

class THaShower;

class THaTotalShower : public THaPidDetector {

public:
  explicit THaTotalShower( const char* name, const char* description = "",
                           THaApparatus* a = nullptr );
  THaTotalShower( const char* name, const char* shower_name,
		  const char* preshower_name, const char* description = "",
		  THaApparatus* a = nullptr );
  ~THaTotalShower() override;

  void       Clear( Option_t* ="" ) override;
  Int_t      Decode( const THaEvData& ) override;
  Int_t      CoarseProcess( TClonesArray& tracks ) override;
  Int_t      FineProcess( TClonesArray& tracks ) override;
  Data_t     GetE() const           { return fE; }
  Int_t      GetID() const          { return fID; }
  THaShower* GetShower() const      { return fShower; }
  THaShower* GetPreShower() const   { return fPreShower; }
  EStatus    Init( const TDatime& run_time ) override;
  void       SetApparatus( THaApparatus* ) override;

protected:

  // Subdetectors
  THaShower* fShower;      // Shower subdetector
  THaShower* fPreShower;   // Preshower subdetector

  // Parameters
  Data_t     fMaxDx;       // Maximum dx between shower and preshower centers
  Data_t     fMaxDy;       // Maximum dx between shower and preshower centers

  // Per event data
  Data_t     fE;           // Total shower energy
  Int_t      fID;          // ID of Presh and Shower coincidence

  Int_t  ReadDatabase( const TDatime& date ) override;
  Int_t  DefineVariables( EMode mode = kDefine ) override;

private:
  void           Setup( const char* name, const char* shower_name,
                        const char* preshower_name, const char* description,
                        THaApparatus* app, bool mode );

  ClassDefOverride(THaTotalShower,0)    //A total shower detector (shower plus preshower)
};

///////////////////////////////////////////////////////////////////////////////

#endif
