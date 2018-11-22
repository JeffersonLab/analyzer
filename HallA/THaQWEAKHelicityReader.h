#ifndef Podd_THaQWEAKHelicityReader_h_
#define Podd_THaQWEAKHelicityReader_h_

//////////////////////////////////////////////////////////////////////////
//
// THaQWEAKHelicityReader
//
// Routines for decoding QWEAK helicity hardware
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

class THaEvData;
class TDatime;
class TH1F;

class THaQWEAKHelicityReader {
  
public:
  THaQWEAKHelicityReader();
  virtual ~THaQWEAKHelicityReader();
  
  // when an event trigger the acquisition the reported helicity, T_settle and Pattern sync
  //signals are recorded by the an Input Register.
  UInt_t GetPatternTir() const{return fPatternTir;};
  UInt_t GetHelicityTir() const{return fHelicityTir;};
  UInt_t GetTSettleTir() const{return fTSettleTir;};
  UInt_t GetTimeStampTir() const{return fTimeStampTir;};
  // in parrallel, these signals are recorded in the ring buffer of the helicity gated scaler
  // the maximum depth of this ring is for now 500.
  UInt_t GetRingDepth() const{return fIRing;}; // how many events are in the ring
  // I should have the depth of the ring defined in the data base
  // let call it kHelRingDepth for now
  void Print();

  struct ROCinfo {
    Int_t  roc;               // ROC to read out
    Int_t  header;            // Headers to search for (0 = ignore)
    Int_t  index;             // Index into buffer
  };
  
protected:

  // Used by ReadDatabase
  enum EROC { kHel = 0, kTime, kRing, kROC3 };
  Int_t SetROCinfo( EROC which, Int_t roc, Int_t header, Int_t index );

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t ReadData( const THaEvData& evdata );
  Int_t         ReadDatabase( const char* dbfilename, const char* prefix,
			      const TDatime& date, int debug_flag = 0 );
  void Begin();
  void FillHisto();
  void End();

  Int_t fPatternTir;
  Int_t fHelicityTir;
  Int_t fTSettleTir;
  Int_t fTimeStampTir;
  Int_t fOldTimeStampTir;
  Int_t fIRing;
  // the ring map is defined in 
  // http://www.jlab.org/~adaq/halog/html/1011_archive/101101100943.html
  // this map is valid after November 1rst
  enum { kHelRingDepth = 500 };
  Int_t fHelicityRing[kHelRingDepth];
  Int_t fPatternRing[kHelRingDepth];
  Int_t fTimeStampRing[kHelRingDepth];
  Int_t fT3Ring[kHelRingDepth];
  Int_t fU3Ring[kHelRingDepth];
  Int_t fT5Ring[kHelRingDepth];
  Int_t fT10Ring[kHelRingDepth];

  // ROC information
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).
  ROCinfo  fROCinfo[kROC3+1]; // for now only work with one roc (roc 11),
  //should be extended to read helicity info from adc 

  Int_t    fQWEAKDebug;          // Debug level
  Bool_t   fHaveROCs;         // Required ROCs are defined
  Bool_t   fNegGate;          // Invert polarity of gate, TO DO implement this functionality
  static const Int_t NHISTR = 12;
  TH1F*    fHistoR[12];  // Histograms

private:

  static Int_t FindWord( const THaEvData& evdata, const ROCinfo& info );

  ClassDef(THaQWEAKHelicityReader,0) // Helper class for reading QWEAK helicity data

};

#endif

