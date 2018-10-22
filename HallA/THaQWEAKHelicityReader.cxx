//*-- Author :    Ole Hansen    August 2006
// Extracted from Bob Michaels' THaHelicity CVS 1.19
////////////////////////////////////////////////////////////////////////
//
// THaQWEAKHelicityReader
//
////////////////////////////////////////////////////////////////////////

#include "THaQWEAKHelicityReader.h"
#include "THaEvData.h"
#include "TMath.h"
#include "TError.h"
#include "VarDef.h"
#include "THaAnalysisObject.h"   // For LoadDB
#include <iostream>
#include <vector>
#include "TH1F.h"

using namespace std;

//____________________________________________________________________
THaQWEAKHelicityReader::THaQWEAKHelicityReader()
  : fPatternTir(0), fHelicityTir(0), fTSettleTir(0), fTimeStampTir(0),
    fOldTimeStampTir(0), fIRing(0),
    fQWEAKDebug(0),      // Debug level
    fHaveROCs(kFALSE),   // Required ROCs are defined
    fNegGate(kFALSE)     // Invert polarity of gate, so that 0=active
{
  // Default constructor
  
  memset( fROCinfo, 0, 3*sizeof(ROCinfo) );
  for( Int_t i = 0; i < NHISTR; ++i )
    fHistoR[i] = 0;
}
//____________________________________________________________________
THaQWEAKHelicityReader::~THaQWEAKHelicityReader() 
{
  // Destructor

  // Histograms will be deleted by ROOT
  // for( Int_t i = 0; i < NHISTR; ++i ) {
  //   delete fHistoR[i];
  // }
}
//____________________________________________________________________
void THaQWEAKHelicityReader::Print() 
{
  cout<<"================================================\n";
  cout<<" THaQWEAKHelicityReader::Print() \n";
  cout<<endl;
  cout<<"fPatternTir, fHelicityTir, fTSettleTir="<< fPatternTir
      <<" , "<<fHelicityTir<<" , "<<fTSettleTir<<endl;
  cout<<"fTimeStampTir ="<<fTimeStampTir<<endl;
  cout<<"fIRing="<<fIRing<<endl<<endl;
  for (int j=0;j<fIRing;j++)
    {
      cout<<j<<"Pattern, helicity, time, T3, U3, T5, T10="
	  <<fHelicityRing[j]<<" , "
	  <<fPatternRing[j]<<" , "
	  <<fTimeStampRing[j]<<" , "
	  <<fT3Ring[j]<<" , "
	  <<fU3Ring[j]<<" , "
	  <<fT5Ring[j]<<" , "
	  <<fT10Ring[j]<<endl;	
    }
  if(fIRing==0)
    {
     cout<<0<<"Pattern, helicity, time, T3, U3, T5, T10="
	  <<fHelicityRing[0]<<" , "
	  <<fPatternRing[0]<<" , "
	  <<fTimeStampRing[0]<<" , "
	  <<fT3Ring[0]<<" , "
	  <<fU3Ring[0]<<" , "
	  <<fT5Ring[0]<<" , "
	  <<fT10Ring[0]<<endl;     
    }      
  cout<<"================================================\n";

  return;
}

//____________________________________________________________________
void THaQWEAKHelicityReader::Clear( Option_t* ) 
{
  fPatternTir=3;
  fHelicityTir=3;
  fTSettleTir=3;
  fTimeStampTir=0;
  fIRing=0;
  for(int i=0;i<kHelRingDepth;i++)
    {
      fHelicityRing[i]=3;
      fPatternRing[i]=3;
      fTimeStampRing[i]=0; 
      fT3Ring[i]=0;
      fU3Ring[i]=0;
      fT5Ring[i]=0;
      fT10Ring[i]=0;
    }

}

//____________________________________________________________________
Int_t THaQWEAKHelicityReader::FindWord( const THaEvData& evdata, 
				     const ROCinfo& info )
// find the index of the word we are looking for given a header
// or simply return the index already stored in ROC info (in that case
// the header stored in ROC info needs to be 0 
{
  Int_t len = evdata.GetRocLength(info.roc);
  if (len <= 4) 
    return -1;

  Int_t i;
  if( info.header == 0 )
    i = info.index;
  else {
    for( i=0; i<len &&
	   (evdata.GetRawData(info.roc, i) & 0xffff000) != info.header;
	 ++i) {}
    i += info.index;
  }
  return (i < len) ? i : -1;
}

//_____________________________________________________________________________
Int_t THaQWEAKHelicityReader::ReadDatabase( const char* /*dbfilename*/,
					    const char* /*prefix*/,
					    const TDatime& /*date*/,
					    int /*debug_flag*/ )
{
  // Read parameters from database:  ROC info (detector map), QWEAK delay value
  // TODO: for now I will bypass this call and just hardcode the data I need
  //  static const char* const here = "THaQWEAKHelicityReader::ReadDatabase";

  // for now bypass the reading from the data base

  SetROCinfo(kHel,11,0,3);
  SetROCinfo(kTime,11,0,4);
  SetROCinfo(kRing,11,0,0);

  return THaAnalysisObject::kOK;
}

//_____________________________________________________________________________
void THaQWEAKHelicityReader::Begin()
{ 
  // static const char* const here = "THaQWEAKHelicityReader::Begin";
  // cout<<here<<endl;

  fHistoR[0]=new TH1F("hel.Pattern.TIR","hel.Pattern.TIR",5,-0.75, 1.75);
  fHistoR[1]=new TH1F("hel.TSettle.TIR","hel.TSettle.TIR",5,-0.75, 1.75);
  fHistoR[2]=new TH1F("hel.Reported.Helicity.TIR","hel.Reported.Helicity.TIR"
		     ,5,-0.75, 1.75);
  fHistoR[3]=new TH1F("hel.dTimestamp.TIR","hel.dTimestamp.TIR",1000,-50,49950);  
  fHistoR[4]=new TH1F("hel.Pattern.Ring","hel.Pattern.Ring",5,-0.75, 1.75);
  fHistoR[5]=new TH1F("hel.Reported.Helicity.Ring","hel.Reported.Helicity.Ring"
		     ,5,-0.75, 1.75);
  fHistoR[6]=new TH1F("hel.Timestamp.Ring","hel.Timestamp.Ring",100,-10,490);
  fHistoR[7]=new TH1F("hel.T3.Ring","hel.T3.Ring",53,-1.5, 50.5);
  fHistoR[8]=new TH1F("hel.U3.Ring","hel.U3.Ring",100,-0.5, 99.5);
  fHistoR[9]=new TH1F("hel.T5.Ring","hel.T5.Ring",53,-1.5, 50.5);
  fHistoR[10]=new TH1F("hel.T10.Ring","hel.T10.Ring",53,-1.5, 50.5);
  fHistoR[11]=new TH1F("hel.NRing","hel.NRing",503,-1.5, 501.5);

  return;
}
//____________________________________________________________________
void THaQWEAKHelicityReader::End()
{
  // static const char* const here = "THaQWEAKHelicityReader::End";
  // cout<<here<<endl;

  for(int i=0;i<NHISTR;i++)
    fHistoR[i]->Write();

  return;
}
//____________________________________________________________________
Int_t THaQWEAKHelicityReader::ReadData( const THaEvData& evdata ) 
{
  // Obtain the present data from the event for QWEAK helicity mode.

  static const char* here = "THaQWEAKHelicityReader::ReadData";

  //  std::cout<<" kHel, kTime, kRing="<< kHel<<" "<<kTime<<" "<<kRing<<endl;
  //     for (int jk=0; jk<3; jk++)
  //     {
  //       std::cout<<" which="<<jk
  // 	       <<" roc="<<fROCinfo[jk].roc
  // 	       <<" header="<<fROCinfo[jk].header
  // 	       <<" index="<<fROCinfo[jk].index 
  // 	       <<endl;
  //     }
  
  //  std::cout<<" fHaveROCs="<<fHaveROCs<<endl;
  if( !fHaveROCs ) {
    ::Error( here, "ROC data (detector map) not properly set up." );
    return -1;
  }
  Int_t hroc = fROCinfo[kHel].roc;
  Int_t len = evdata.GetRocLength(hroc);
  if (len <= 4) 
    return -1;

  Int_t ihel = FindWord( evdata, fROCinfo[kHel] );
  if (ihel < 0) {
    ::Error( here , "Cannot find helicity" );
    return -1;
  }
  Int_t data = evdata.GetRawData( hroc, ihel );  
  fPatternTir =(data & 0x20)>>5;
  fHelicityTir=(data & 0x10)>>4;
  fTSettleTir =(data & 0x40)>>6;
 
  hroc=fROCinfo[kTime].roc;
  len = evdata.GetRocLength(hroc);
  if (len <= 4) 
    {
      ::Error( here, "length of roc event not matching expection ");
      return -1;
    }
  Int_t itime = FindWord (evdata, fROCinfo[kTime] );
  if (itime < 0) {
    ::Error( here, "Cannot find timestamp" );
    return -1;
  }
  
  fTimeStampTir=static_cast<UInt_t> (evdata.GetRawData( hroc, itime ));  
  
  // Invert the gate polarity if requested
  //  if( fNegGate )
  //  fGate = !fGate;
  
  hroc=fROCinfo[kRing].roc;
  len = evdata.GetRocLength(hroc);
  if (len <= 4) 
    {
      ::Error( here, "length of roc event not matching expection (message 2)");
      //  std::cout<<" len ="<<len<<endl;
      //      std::cout<<"kRING="<<kRing<<" hroc="<<hroc<<endl;
      return -1;
    }
  Int_t index=0;
  while (index < len && fIRing == 0) 
    {
      Int_t header=evdata.GetRawData(hroc,index++);
      if ((header & 0xffff0000) == 0xfb1b0000) 
	{
	  fIRing = header & 0x3ff;
	}
    }
   
  //  std::cout<<" fIRing ="<<fIRing<<" index="<<index<<endl;


  if(fIRing>kHelRingDepth)
    {
      ::Error( here, "Ring depth to large ");
      return -1;
    }
  for(int i=0;i<fIRing; i++)
    {
      fTimeStampRing[i]=evdata.GetRawData(hroc,index++);
      data=evdata.GetRawData(hroc,index++);
      fHelicityRing[i]=(data & 0x1);
      fPatternRing[i]= (data & 0x10)>>4;
      fT3Ring[i]=evdata.GetRawData(hroc,index++);
      fU3Ring[i]=evdata.GetRawData(hroc,index++);
      fT5Ring[i]=evdata.GetRawData(hroc,index++);
      fT10Ring[i]=evdata.GetRawData(hroc,index++);
    }

  //    Print();
  
  FillHisto(); 

  return 0;
}
//____________________________________________________________________
void THaQWEAKHelicityReader::FillHisto()
{
  // static const char* here = "THaQWEAKHelicityReader::FillHisto";
  // cout<<here<<endl;

  fHistoR[0]->Fill(fPatternTir);
  fHistoR[1]->Fill(fTSettleTir);
  fHistoR[2]->Fill(fHelicityTir);
  fHistoR[3]->Fill(fTimeStampTir-fOldTimeStampTir);
  fOldTimeStampTir=fTimeStampTir;
  for(int i=0;i<fIRing;i++)
    {
      fHistoR[4]->Fill(fPatternRing[i]);
      fHistoR[5]->Fill(fHelicityRing[i]);
      fHistoR[6]->Fill(fTimeStampRing[i]);
      fHistoR[7]->Fill(fT3Ring[i]);
      fHistoR[8]->Fill(fU3Ring[i]);
      fHistoR[9]->Fill(fT5Ring[i]);
      fHistoR[10]->Fill(fT10Ring[i]);
    }
  fHistoR[11]->Fill(fIRing);

  return;
}


//TODO: this should not be needed once LoadDB can fill fROCinfo directly
//____________________________________________________________________
Int_t THaQWEAKHelicityReader::SetROCinfo( EROC which, Int_t roc,
				       Int_t header, Int_t index )
{

  // Define source and offset of data.  Normally called by ReadDatabase
  // of the detector that is a THaQWEAKHelicityReader.
  //
  // "which" is one of  { kHel, kTime, kROC2, kROC3 }.
  // You must define at least the kHel and kTime ROCs.
  // Returns <0 if parameter error, 0 if success

  if( which<kHel || which>kROC3 )
    return -1;
  if( roc <= 0 || roc > 255 )
    return -2;

  fROCinfo[which].roc    = roc;
  fROCinfo[which].header = header;
  fROCinfo[which].index  = index;

  fHaveROCs = ( fROCinfo[kHel].roc > 0 && fROCinfo[kTime].roc > 0 );
  
  return 0;
}

//____________________________________________________________________
ClassImp(THaQWEAKHelicityReader)

