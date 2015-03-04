/////////////////////////////////////////////////////////////////////
//
// R. Michaels, March 2000
// Updated string_from_buffer, May 2007/
// THaUsrstrutils = USeR STRing UTILitieS.
// The code below is what is used by DAQ to interpret strings
// like prescale factors 
// Yes, this is mostly old-style C, but it
// has the advantage that the interpretation should
// be identical to what the DAQ used.  
//
/////////////////////////////////////////////////////////////////////

#define COMMENT_CHAR ';'

#include "THaUsrstrutils.h"
#include "TMath.h"
#include "TRegexp.h"
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

namespace Decoder {

const int           THaUsrstrutils::MAX     = 5000;
const unsigned long THaUsrstrutils::LONGMAX = 0x7FFFFFFF;
static const int DEBUG = 0;

int THaUsrstrutils::getflag(const char *s) const
{
  const char *pos,*val;

  getflagpos(s,&pos,&val);
  if(!pos) return(0);
  if(!val) return(1);
  return(2);
}

char* THaUsrstrutils::getstr(const char *s) const
{
  const char *pos,*val;
  const char *end;
  char *ret;
  int slen;

  getflagpos(s,&pos,&val);
  if(!val){
    return(0);
  }
  end = strchr(val,',');	/* value string ends at next keyword */
  if(end)
    slen = end - val;
  else				/* No more keywords, value is rest of string */
    slen = strlen(val);

  ret = new char[slen+1];
  strncpy(ret,val,slen);
  ret[slen] = '\0';
  return(ret);
}

unsigned int THaUsrstrutils::getint(const char *s) const
{
  char* sval = getstr(s);
  if(!sval) return(0);		/* Just return zero if no value string */
  unsigned int retval = strtol(sval,0,0);
  if(retval == LONGMAX && (sval[1]=='x' || sval[1]=='X')) {/* Probably hex */
     sscanf(sval,"%12x",&retval);
   }
  delete [] sval;
  return(retval);
}

void THaUsrstrutils::getflagpos_instring(const char *constr, const char *s,
					 const char **pos_ret, const char **val_ret)
{
  int slen = strlen(s);
  const char *pos = constr;

  while(pos) {
    pos = strstr(pos,s);
    if(pos) {			/* Make sure it is really the keyword */
      /* Make sure it is an isolated keyword */
      if((pos != constr && pos[-1] != ',') ||
	 (pos[slen] != '=' && pos[slen] != ',' && pos[slen] != '\0')) {
	pos += 1;	continue;
      } else break;		/* It's good */
    }
  }
  *pos_ret = pos;
  if(pos) {
    if(pos[slen] == '=') {
      *val_ret = pos + slen + 1;
    } else 
      *val_ret = 0;
  } else
    *val_ret = 0;
  return;
}
  
void THaUsrstrutils::string_from_evbuffer(const UInt_t* evbuffer, int nlen )
{
// Routine string_from_evbuffer loads the configstr from the event buffer.
// It has the same strengths and weaknesses as the DAQ code,
// hence the interpretation should be the same.
//
// nlen is the length of the string data (from CODA) in longwords (4 bytes)
//
// R. Michaels, updated to make the algorithm closely resemble the
//              usrstrutils.c used by trigger supervisor.

  size_t bufsize = sizeof(int)*nlen;
  char* ctemp = new char[bufsize+1];
  if( !ctemp ) {
    configstr = "";
    return;
  }
  strncpy( ctemp, reinterpret_cast<const char*>(evbuffer), bufsize );
  ctemp[bufsize] = 0;  // terminate C-string for sure
  string strbuff = ctemp;
  string strdynamic = strbuff;
  delete [] ctemp;
  string::size_type pos,pos1,pos2;
  vector<string> strlines;
  string sline,stemp;
  
  if (DEBUG >= 1) {
    cout << "Check THaUsrstrutils::string_from_buffer "<<endl;
    cout << "strbuff length "<<strbuff.length()<<endl;
    cout << "strbuff == \n"<<strbuff<<endl<<endl;
  }

// In order to make this as nearly identical to the online VME code,
// we first unpack the buffer into lines seperated by '\n'.  
// This is like the reading of the prescale.dat file done
// by fgets(s,255,fd) in usrstrutils.c
  pos=0;  pos1=0;  pos2=0;
  while (pos < strbuff.length()) {
     pos1 = strdynamic.find_first_of("\n",0);
     if (pos1 != string::npos) {
       pos2 = pos1;
     } else {   // It may happen that the last line has no '\n'
       pos2 = strdynamic.length(); 
     } 
     sline = strdynamic.substr(0,pos2);
     strlines.push_back(sline);
     pos += pos2 + 1;
     if (DEBUG >= 2) {
         cout << "pos = "<< pos << "   pos1 "<<pos1<<"  pos2 "<<pos2<<"  sline =  "<<sline<<"\n dynamic str  "<<strdynamic<<"  len "<<strdynamic.length()<<endl;
     }
     if (pos < strbuff.length()) {
        stemp = strdynamic.substr(pos2+1,pos2+strdynamic.length());
        strdynamic = stemp;
     } 
   }

// Next we loop over the lines of the 'prescale.dat file' and 
// use the exact same code as in usrstrutils.c
   char s[256], *flag_line=0;
   if (strlines.size() == 0) {
      configstr="";
      return;
   }
   for (int i=0; i<(Int_t)strlines.size(); i++) {
     strcpy(s,strlines[i].c_str());
     if (DEBUG >= 1) printf("prescale line[%d]  =  %s \n",i,s);
     char *arg;
     arg = strchr(s,COMMENT_CHAR);
     if(arg) *arg = '\0';       /* Blow away comments */
     arg = s;			/* Skip whitespace */
     while(*arg && isspace(*arg)){
	arg++;
     }
     if(*arg) {
	flag_line = arg;
	break;
     }
   }
   if (DEBUG >= 1) printf("flag_line = %s \n",flag_line);

   configstr = flag_line;

   return;
}
      
// This routine reads a file to load file_configustr.
// It is like what is used by the DAQ code to load prescale factors, etc.

void THaUsrstrutils::string_from_file(const char *ffile_name)
{
  FILE *fd;
  char s[256], *flag_line;

/* check that filename exists */
  configstr = "";
  fd = fopen(ffile_name,"r");
  if(fd == NULL) {
    printf("Failed to open usr flag file %s\n",ffile_name);  
  } else {
    /* Read till an uncommented line is found */
    flag_line = 0;
    while(fgets(s,255,fd)){
      char *arg;
      arg = strchr(s,COMMENT_CHAR);
      if(arg) *arg = '\0'; /* Blow away comments */
      arg = s;			/* Skip whitespace */
      while(*arg && isspace(*arg)){
	arg++;
      }
      if(*arg) {
	flag_line = arg;
	break;
      }
    }
    if(flag_line) {		/* We have a config usrstr */
      configstr = flag_line;
      if(DEBUG) cout << "configstr =  " << configstr << endl;
    }
    fclose(fd);
  }
}

}

ClassImp(Decoder::THaUsrstrutils)

