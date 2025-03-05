/////////////////////////////////////////////////////////////////////
//
//   VETROCtdcModule
//   JLab VETROC TDC Module
//
/////////////////////////////////////////////////////////////////////
//
//  v0 - Marco Carmignotto - 2017/11/10
//    * One module only
//    * Calibration NOT included - shifts and phantom peaks exits
//
/////////////////////////////////////////////////////////////////////

#include "VETROCtdcModule.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

double evtM = 0;
Bool_t saveForCalib = 0;

using namespace std;

namespace Decoder {

//  const Int_t NTDCCHAN = 192;
//  const Int_t MAXHIT   = 100;

  Module::TypeIter_t VETROCtdcModule::fgThisType =
    DoRegister( ModuleType( "Decoder::VETROCtdcModule" , 1984 ));

VETROCtdcModule::VETROCtdcModule(Int_t crate, Int_t slot) : VmeModule(crate, slot) {
  fDebugFile=0;
  Init();
}

VETROCtdcModule::~VETROCtdcModule() {
  if (fNumHitsP) delete [] fNumHitsP;
  if (fNumHitsN) delete [] fNumHitsN;
  if (fTdcDataP) delete [] fTdcDataP;
  if (fTdcDataN) delete [] fTdcDataN;
  if (fTdcFineP) delete [] fTdcFineP;
  if (fTdcFineN) delete [] fTdcFineN;
}

void VETROCtdcModule::Init() {
  fNumHitsP = new Int_t[NTDCCHAN];
  fNumHitsN = new Int_t[NTDCCHAN];
  fTdcDataP = new Int_t[NTDCCHAN*MAXHIT];
  fTdcDataN = new Int_t[NTDCCHAN*MAXHIT];
  fTdcFineP = new Int_t[NTDCCHAN*MAXHIT];
  fTdcFineN = new Int_t[NTDCCHAN*MAXHIT];
  fDebugFile=0;
  Clear();
  IsInit = kTRUE;
  fName = "VETROC";
  fNumChan = NTDCCHAN;
  fWdcntMask=0;
  SetResolution(1);
  if (fModelNum == 6401) SetResolution(0);
}


Bool_t VETROCtdcModule::IsSlot(UInt_t rdata)
{
  if (fDebugFile)
    *fDebugFile << "is VETROCtdc slot ? "<<hex<<fHeader
		<<"  "<<fHeaderMask<<"  "<<rdata<<dec<<endl;
  return ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader));
}

void VETROCtdcModule::Clear(const Option_t* opt) {
  VmeModule::Clear(opt);
  memset(fNumHitsP, 0, NTDCCHAN*sizeof(Int_t));
  memset(fNumHitsN, 0, NTDCCHAN*sizeof(Int_t));
  memset(fTdcDataP, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
  memset(fTdcDataN, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
  memset(fTdcFineP, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
  memset(fTdcFineN, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
}


Int_t VETROCtdcModule::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len) {
// the 4-arg version of LoadSlot.  Let it call the 3-arg version.
// I'm not sure we need both (historical)

    return LoadSlot(sldat, evbuffer+pos, evbuffer+pos+len);

  }

Int_t VETROCtdcModule::GetHit(Int_t chan, Int_t nhit, Bool_t edge) {
	if(chan<0 || chan>NTDCCHAN) {
		cout << "VETROC: Warning! Channel " << chan << " out of range for tdc. Ignoring call of GetHit." << endl;
		return 0;
	}
	if(nhit<0 || nhit>MAXHIT) {
		cout << "VETROC: Warning! Asking for hit number out of range [0, " << MAXHIT << "]. Ignoring call of GetHit.";
		return 0;
	}
	else {
		if(edge==0) return fTdcDataP[chan*int(MAXHIT)+nhit];
		else return fTdcDataN[chan*int(MAXHIT)+nhit];
	}
}

Int_t VETROCtdcModule::GetFine(Int_t chan, Int_t nhit, Bool_t edge) {
	if(chan<0 || chan>NTDCCHAN) {
		cout << "VETROC: Warning! Channel " << chan << " out of range for tdc. Ignoring call of GetHit." << endl;
		return 0;
	}
	if(nhit<0 || nhit>MAXHIT) {
		cout << "VETROC: Warning! Asking for hit number out of range [0, " << MAXHIT << "]. Ignoring call of GetHit.";
		return 0;
	}
	else {
		if(edge==0) return fTdcFineP[chan*int(MAXHIT)+nhit];
		else return fTdcFineN[chan*int(MAXHIT)+nhit];
	}
}

Int_t VETROCtdcModule::GetNumHits(Int_t chan, Bool_t edge) {
	if(chan<0 || chan>NTDCCHAN) {
		cout << "VETROC: Warning! Channel " << chan << " out of range for tdc. Ignoring call of GetNumHits." << endl;
		return 0;
	}
	else {
		if(edge==0) return fNumHitsP[chan];
		else return fNumHitsN[chan];
	}
}

Int_t VETROCtdcModule::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop) {
// This is the 3-arg version of LoadSlot
// Note, this increments evbuffer
  if (fDebugFile) *fDebugFile << "VETROCtdcModule:: loadslot "<<endl;
  fWordsSeen = 0;

  // Calibration done for channels in interval 32..159 (front connectors used for tritium experiment). We assume the mean value of 109.59 for channels without hits (0..31, 64..95, 112, 125, 128..159). If hits are taken in these uncalibrated channels, contact Marco Carmignotto to calibrate them.
  Double_t calib[] = {109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 108.692, 111.3, 108.693, 107.581, 111.098, 109.696, 110.547, 108.024, 110.363, 109.077, 110.602, 110.959, 108.255, 109.094, 110.287, 108.294, 109.021, 108.374, 110.268, 110.549, 108.376, 109.967, 109.53, 109.278, 110.677, 110.63, 110.41, 108.088, 110.328, 109.193, 110.385, 109.293, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 108.695, 111.572, 109.718, 109.533, 110.881, 109.287, 110.085, 109.681, 110.643, 108.379, 111.042, 109.314, 108.862, 109.535, 110.432, 109.608, 109.59, 112.308, 110.623, 109.797, 108.208, 109.421, 110.103, 108.121, 110.43, 111.211, 108.219, 108.094, 109.344, 109.59, 109.183, 112.637, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59, 109.59}; // note that we have 160 elements here, although the module has more channels accessible from the back of the crate.

   // look at all the data
   const UInt_t *loc = evbuffer;
   Int_t fDebug=2;

   // VETROC
   const UInt_t VET_BlkHeader = 1<<31;
   const UInt_t VET_BlkHeader_mask = 0xF8000000;
   UInt_t VET_BlkSlot;
   const UInt_t VET_slot_mask      = 0x07C00000;
   const UInt_t VET_id = 0x9;
   const UInt_t VET_id_mask        = 0x003C0000;
   UInt_t VET_BlkNum;
   const UInt_t VET_BlkNum_mask    = 0x0003FF00;
   UInt_t VET_BlkLevel;
   const UInt_t VET_BlkLevel_mask  = 0x000000FF;

   // For checks
   Bool_t gotBlkHeader=0;
   Bool_t gotEvtHeader=0;

   UInt_t evtNum, evtSlot;
   UInt_t timeA=0;
   UInt_t timeB=0;
   Int_t t_hit;

   // To neglect "mirror" hits (feature of the VETROC that save a hit twice if the fine value was on the extremes)
   Int_t t_previous=-1;
   Int_t ch_previous=-1;
   Int_t fine_previous=-1;

   // To save for calibration
   ofstream fcalib;
   if(saveForCalib) {
      fcalib.open("fine_for_calib.txt", std::ofstream::out | std::ofstream::app);
   }

   if(fDebug > 1 && fDebugFile!=0) *fDebugFile<< "Debug of VETROC data, fResol =  "<<fResol<<"  model num  "<<fModelNum<<endl;
   while ( loc <= pstop && IsSlot(*loc) ) {
cout << "";

		// Check for BlkHeader and board ID to start decoding
		if(((*loc)&VET_BlkHeader_mask)==VET_BlkHeader && ((*loc)&VET_id_mask)>>18==VET_id) {
			gotBlkHeader=1;

			// Not doing anything now with slot read from BLOCK_HEADER
			VET_BlkSlot = ((*loc)&VET_slot_mask)>>22; // is this correct?

			// Not doing anything now with Block#
			VET_BlkNum = ((*loc)&VET_BlkNum_mask)>>8; // is this correct?

			// Check if blocklevel is 1 (all this decoder is doing so far)
			VET_BlkLevel = ((*loc)&VET_BlkLevel_mask);
			if(VET_BlkLevel != 1) {
				cout << "VETROC - Decoder not ready to analyze Block level (" << VET_BlkLevel << ") different of 1." << endl;
  				return fWordsSeen;
			}
			loc++;
			continue; // this was a BlkHeader, so make sure not to check for event in this word
		}
		// Do we have gotBlkHeader already? So we are readying data!
		if(gotBlkHeader) {

			// Check the word bits 31..27 to decide what is that
			UInt_t head = (*loc)>>27; // Word bit 31..27
			
			// Check for event header
			if(head == 0x12) { // Event header
					gotEvtHeader=1;
					evtNum = (*loc) & 0x003FFFFF; // doesn't seems right... ignoring for now.
					evtSlot = ((*loc)&VET_slot_mask)>>22; // is this correct?
					//cout << "Event: " << Form("%x",(*loc))<< evtNum << endl;
					t_previous = -1;
					ch_previous = -1;
					fine_previous = -1;
			}

			// Make sure we have an event header
			if(gotEvtHeader==0) {
				cout << "VETROC - Missing event header. Aborting analysis." << endl;
				return fWordsSeen;
			}

			// Check for the next types of head
			switch(head) {
				case 0x12: break; //Event header - already treated above.
				case 0x13: { // TimeTrg#1
					timeA = (*loc) & 0x00FFFFFF;
					break;
				}
				case 0x00: { // TimeTrg#2
					timeB = (*loc) & 0x00FFFFFF;
					tEVT = (((double)timeA) + ((double)timeB)*((double)0x00FFFFFF))*4.0e-9; // seconds
					break;
				}
				case 0x11: { // BlockTrailer
					gotBlkHeader=0;
					gotEvtHeader=0;
					break;
				}
				case 0x17: { // Data
					Int_t group  = ((*loc) & 0x07000000)>>24;
					Int_t chan   = ((*loc) & 0x00F80000)>>19;
					Int_t edgeD  = ((*loc) & 0x00040000)>>18;
					Int_t coarse = ((*loc) & 0x0003FF00)>>8;
					Int_t two_ns = ((*loc) & 0x00000080)>>7;
					Int_t fine   = ((*loc) & 0x0000007F);
					//cout << Form("Group: %d - Chan: %d - Edge: %d - Coarse: %d - 2n: %d - Fine: %d",group,chan,edge,coarse,two_ns,fine) << endl;
					
					Int_t ch = (group*32 + chan);
					// FIXME: calibration done for channels 0..159 only (used for tritium experiments), so we have to break the line below
					if(ch<160) t_hit = coarse*4000 + two_ns*2000 + fine*2000.0/calib[ch]; //ps (if change the units for other than ps, correct condition for mirror hit identification below)
					else t_hit = coarse*4000 + two_ns*2000 + fine*2000.0/calib[0]; //ps

					// make sure it is not a mirror hit
					if(ch==ch_previous && fabs(t_hit-t_previous)<500.0) { // mirror hit, ignore it
						// Print on screen to check
						//cout << "Mirror: ch=" << ch << "\t dt=" << (t_hit-t_previous) << endl;
						if(saveForCalib) fcalib << ch << ", " << fine_previous << ", " << fine << endl;
					}
					else { 
						if(edgeD==0) {
		      				Int_t idx = ch*MAXHIT + fNumHitsP[ch];
							if (fNumHitsP[ch]<MAXHIT) {
								fTdcDataP[idx] = t_hit;
								fTdcFineP[idx] = fine;
							}
							else cout << "VETROC: warning. Too many hits in channel " << ch << ". Ignoring if hitNumber greater than " << MAXHIT << "." << endl;
	 						(fNumHitsP[ch])++;
						}
						else {
	      					Int_t idx = ch*MAXHIT + fNumHitsN[ch];
							if (fNumHitsN[ch]<MAXHIT) {
								fTdcDataN[idx] = t_hit;
								fTdcFineN[idx] = fine;
							}
							else cout << "VETROC: warning. Too many hits in channel " << ch << ". Ignoring if hitNumber greater than " << MAXHIT << "." << endl;
 							(fNumHitsN[ch])++;
						}
					}
					
					t_previous = t_hit;
					ch_previous = ch;
					fine_previous = fine;
					
					fWordsSeen++;

					//cout << "ch=" << ch << " - idx=" << idx << " - data= " << fTdcDataN[idx] << endl;
					break;
				}
				default: {
					// Filler_A and Filler_B will not appear, since they are after Block Trailer
					cout << "VETROC - Problem. Word not identified: " << Form("%x",*loc) << " - stopping to decode VETROC." << endl;
					return fWordsSeen;
				}
			}
		}
        loc++;
	}

	// Make sure we got block trailer
	//if(gotBlkHeader==1) cout << "VETROC: Warning! Event finished without BlockTrailer. Problem saving data?" << endl;

	if(saveForCalib) fcalib.close();

  return fWordsSeen;
}

}

ClassImp(Decoder::VETROCtdcModule)
