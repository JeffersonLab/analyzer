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
  THaTotalShower( const char* name, const char* description = "",
		  THaApparatus* a = NULL );
  THaTotalShower( const char* name, const char* shower_name,
		  const char* preshower_name, const char* description = "",
		  THaApparatus* a = NULL );
  virtual ~THaTotalShower();

  virtual void       Clear( Option_t* ="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Float_t    GetE() const           { return fE; }
	  Int_t      GetID() const          { return fID; }
      	  THaShower* GetShower() const      { return fShower; }
	  THaShower* GetPreShower() const   { return fPreShower; }
  virtual EStatus    Init( const TDatime& run_time );
  virtual void       SetApparatus( THaApparatus* );

protected:

  // Subdetectors
  THaShower* fShower;      // Shower subdetector
  THaShower* fPreShower;   // Preshower subdetector

  // Parameters
  Float_t    fMaxDx;       // Maximum dx between shower and preshower centers
  Float_t    fMaxDy;       // Maximum dx between shower and preshower centers

  // Per event data
  Float_t    fE;           // Total shower energy
  Int_t      fID;          // ID of Presh and Shower coincidence

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

private:
  void           Setup( const char* name,  const char* desc, 
			const char* shnam, const char* psnam,
			THaApparatus* app, bool mode );

  ClassDef(THaTotalShower,0)    //A total shower detector (shower plus preshower)
};

///////////////////////////////////////////////////////////////////////////////

#endif
