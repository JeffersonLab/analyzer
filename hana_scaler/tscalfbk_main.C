//--------------------------------------------------------
//  tscalfbk_main.C
//
//  Use scalers to apply feedback on the charge asymmetry
//  R. Michaels   Nov 2004
//--------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <signal.h>
#include "THaScaler.h"

using namespace std;

THaScaler *scaler;
void AsyAccum();
int AsyAvg();
void ApplyFeedBack();
void LogMessage(const char* message); 
string GetTime();
float myatof(const char *cinput);
extern "C" void signalhandler(int s);

// file to store results
FILE *fd;
// which bcm to use for feedback
static string whichbcm = "bcm_u3";
// cut on beam current
static int bcmcut = 2000;  // This is ~0.5 uA on bcm_bu3
// interval of time between scaler readings
static int sleeptime = 10;   // typically 10
// number of events to accumulate for feedback
// (Note, nevt*sleeptime = feedback interval ~ 15 min)
static int nevt = 90;       // typically 90
// to debug(>0) or not(0)
static int debug = 0;
// Feedback slope (must be calibrated, hopefully stable)
static float fslope = 147;   // ppm/knob
// Feedback script
static string fbscript = "/adaqfs/home/adaq/qasy/feedback/apply_feedback";
// Present value of feedback data
static string fbdata = "/adaqfs/home/adaq/qasy/feedback/feedback.data";
// Min and max IA feedback
static double minia = 0;
static double maxia = 10;

static int evcnt, goodevt;

// asy data
double *asy;
double qasy_global = 0;
double signif_global = 0;

int main(int argc, char* argv[]) {

   signal(31, signalhandler);
   fd = fopen("/adaqfs/home/adaq/qasy/feedback/qasyfdbk.log","a+");

   asy = new double[nevt+10];

   scaler = new THaScaler("Left");
   if (scaler->Init() == -1) {
      cout << "Error initializing scalers"<<endl;  return 1;
   }

   if (debug >= 4) cout << "time = "<<GetTime()<<endl;

   char sleepcommand[100];
   sprintf(sleepcommand,"sleep %d",sleeptime);

   evcnt = 0;
   goodevt = 0;

   while ( scaler->LoadDataOnline() != SCAL_ERROR ) {  
     if (debug == 3) scaler->Print();
     evcnt++;
     if (evcnt >= 2) AsyAccum();   
     if (goodevt >= 2 && (evcnt % nevt) == 0) ApplyFeedBack();

     system(sleepcommand);

   }

   return 0;

}

void AsyAccum() {
  // Accumulate charge asymmetries
   double bcm = scaler->GetBcmRate(whichbcm.c_str());  
   if (debug >= 2) cout << "bcm "<<bcm<<endl;
   if (bcm > bcmcut) {
     double sum = scaler->GetBcmRate(1,whichbcm.c_str()) + 
                  scaler->GetBcmRate(-1,whichbcm.c_str());
     if (sum > 0 && goodevt >= 0 && goodevt < nevt) {
       asy[goodevt++] = 1e6*  // ppm
                       (scaler->GetBcmRate(1,"bcm_u3") - 
                        scaler->GetBcmRate(-1,"bcm_u3"))/ sum;
       if (debug) {
         cout << "AsyAccum  goodevt "<<goodevt<<"   bcm = "<<bcm;
         cout << "   asy = "<<asy[goodevt-1]<< " ppm " <<endl;
       }
     }
   }
}


void ApplyFeedBack() {
  // Apply the feedback if possible and applicable
  // cuts:
  //     qasy_toobig = abs_value of qasy that is too big
  //     qasy_toosmall = abs_value of qasy that is too small
  //     signif_toosmall = significance too small

  double qasy_toobig, qasy_toosmall, signif_toosmall;
  qasy_toobig = 20000;  // 20,000 ppm = 2% is huge (something wrong !)
  qasy_toosmall = 50;   // 50 ppm is too small to apply feedback
  signif_toosmall = 2;  // if < 2 sigma, too small to apply feedback

  if (AsyAvg() == 1) {
    double absqasy = qasy_global;
    if (absqasy < 0) absqasy = -1*absqasy;
    if (absqasy > qasy_toobig) {
      LogMessage("ERROR: Asy too big to apply feedback\n");
      goto out1;
    }
    if (absqasy < qasy_toosmall) {
      LogMessage("No feedback applied; too small.\n");
      goto out1;
    }
    if (signif_global < signif_toosmall) {
      LogMessage("No feedback applied; not significant.\n");
      goto out1;
    }
    char command[100];
    float acorr = 0;
    if (fslope != 0) acorr = qasy_global / fslope;
    if (acorr == 0) {
      LogMessage("correction = 0\n");
      goto out1;
    }
    ifstream fbdatafile;
    fbdatafile.open(fbdata.c_str());
    if (!fbdatafile) {
      LogMessage("ERROR: ApplyFeedback:  No fbdata file \n");
      goto out1;
    }
    string sinput;
    float oldfdata = 0;
    float newfdata = 0;    
    while (getline(fbdatafile,sinput)) {
      oldfdata = myatof(sinput.c_str());
      if (debug) cout << "oldfdata = "<<oldfdata<<endl;
    }
    newfdata = oldfdata - acorr;
    if (newfdata < minia) newfdata = minia;
    if (newfdata > maxia) newfdata = maxia;
    sprintf(command,"  1  %4.2f \n",newfdata);
    string scommand = command;
    string syscall = fbscript + scommand;
    system(syscall.c_str());
    LogMessage("-->  FEEDBACK  APPLIED.   System call: \n");
    syscall += "\n";    
    LogMessage(syscall.c_str());
    
  }

out1:
  // restart averages
  goodevt = 0;
  for (int i = 0; i < nevt; i++) asy[i] = 0;

  return;
}

int AsyAvg() {
  // Average the asymmetries.
  if (goodevt < 2) return -1;
  qasy_global = 0;  signif_global = 0;
  double asysum, asysq, xcnt, asysig, asyavg, adiff, asize;
  asysum = 0;  asysq = 0;  xcnt = 0;
  for (int i = 0; i < goodevt; i++) {
    asysum += asy[i];
    asysq  += asy[i]*asy[i];
    xcnt += 1;
  }
  if (xcnt <= 0) {
    LogMessage("Zero accumulated counts\n");  return -1;
  }
  asyavg = asysum/xcnt;
  adiff = asysq/xcnt - asyavg*asyavg;
  if (adiff <= 0) {
     LogMessage("Sigma < 0 \n");  return -1;
  }
  asysig = sqrt(adiff);
  if (debug >= 2) cout << "1st pass "<<xcnt<<"  "<<asyavg<<"  "<<asysig<<endl;
  // Average again, now only if within 4 sigma
  asysum = 0;  asysq = 0;  xcnt = 0;
  for (int i = 0; i < goodevt; i++) {
    if (asy[i] > asyavg - 4*asysig && asy[i] < asyavg + 4*asysig) {
      asysum += asy[i];
      asysq  += asy[i]*asy[i];
      xcnt += 1;
    }
  }
  if (xcnt <= 0) {
    LogMessage("2nd pass no events \n");  return -1;
  }
  asyavg = asysum/xcnt;
  adiff = asysq/xcnt - asyavg*asyavg;
  if (adiff <= 0) {
     LogMessage("2nd pass sigma < 0 \n"); return -1;
  }
  asysig = sqrt(adiff)/sqrt(xcnt);  // asy in average
  if (debug >= 2) cout << "2nd pass "<<xcnt<<"  "<<asyavg<<"  "<<asysig<<endl;
  if (asysig == 0) {
    LogMessage("Sigma = 0 \n");  return -1;  
  }
  asize = asyavg / asysig;
  if (asize < 0) asize = -1*asize;
  qasy_global = asyavg;
  signif_global = asize;
  char result[100];
  sprintf(result,
      "   Qasy(ppm) = %-8.1f +/- %-8.1f   signif(sigma) = %-6.1f \n",
      qasy_global, asysig, signif_global); 
  string sresult = result;
  string message = GetTime() + sresult;
  LogMessage(message.c_str());
  return 1;
}

string GetTime() {
  int min, hour, day, month, year;
  time_t *ta,tp;
  struct tm *btm;
  ta = 0;
  tp = time(ta);
  btm = gmtime((const time_t*)&tp);
  year =  1900 + btm->tm_year;  
  month = 1 + btm->tm_mon;  
  day = btm->tm_mday;
  if (btm->tm_hour >= 0 && btm->tm_hour < 5) {
      hour = btm->tm_hour + 19;
  } else {
      hour = btm->tm_hour - 5;
  }
  min = btm->tm_min;  
  char result[100];
  sprintf(result,"Date(m-d-y)H:M = %d-%d-%d  %d:%d ",
        month,day,year,hour,min);
  if (debug >= 2) cout << result << endl;
  string sresult = result;
  return sresult;
}

void LogMessage(const char* message) {
  if (fd != NULL) fprintf(fd,message);
  fflush(fd);
  if (debug >= 2) cout << "Message:  "<<message<<endl;
}

void signalhandler(int sig) {  
// To deal with the signal "kill -31 pid"
  if (debug) cout << "Got kill signal.  Applying feedback last time "<<endl<<flush;
  if (AsyAvg() == 1) ApplyFeedBack();
  fclose(fd);
  exit(1);
}

float myatof(const char *cinput) {
// This does what atof is supposed to do.
// Standard atof doesn't really work so I wrote my own.
  string sinput = cinput;
  float result = 0;
  float sign = 1;
  result = atof(sinput.c_str());
  int pos = sinput.find("-");
  if (pos == 0) {
     sign = -1;
     result = atof(sinput.substr(1,sinput.length()).c_str());
  }      
  unsigned long iloc;
  iloc = sinput.find(".");
  if (iloc == string::npos) {
    return result;
  } else {
    string s1,s2;
    int i1 = 0;
    if (sign == -1) i1 = 1;
    s1 = sinput.substr(i1,iloc-i1);
    result = atof(s1.c_str());
    s2 = sinput.substr(iloc+1,sinput.length());
    int decsize = sinput.length()-iloc-1;
    if (decsize > 0) {
      float power = 1;
      for (int j = 0; j < decsize; j++) power *= 10;
      result += atof(s2.c_str())/power;
    } 
  }     
  return sign*result;
};

