// owndef.hh

//#define DEBUG
#define SECURE

#ifndef __OWNDEF_H
#define __OWNDEF_H

// Muß momentan noch die alte Abbruchmethode verwenden, weil es Probleme mit
// GCC 2.7.2 gibt
#define THROW(a) {printf("throw %s\n",a);exit(1);}
//#define THROW(txt) {FILE *f=fopen("error.pipe","w");fprintf(f,"%s\n",txt);fclose(f);exit(1);}
//#define THROW(txt) throw exception(txt);

/****************************************************************************
 *																																					*
 * Typen                                                                    *
 *																																					*
 ****************************************************************************/
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  ULONG;

/***************************************************************************
 *                                                                         *
 * Funktionen für die Endian-Aufhebung                                     *
 *                                                                         *
 ***************************************************************************/
inline WORD NoEndianWord(BYTE* ptr)
{
  register WORD n;
  n=(*ptr)+(*(ptr+1)<<8);
  return n;
}

inline ULONG NoEndianLong(BYTE* ptr)
{
  register ULONG n;
  n=NoEndianWord(ptr)+(NoEndianWord(ptr+2)<<16);
  return n;
}

inline void WrNoEndianWord(WORD n, BYTE* ptr)
{
  *(ptr  ) = n & 0xFF;
  n>>=8;
  *(ptr+1) = n;
}

inline void WrNoEndianLong(ULONG n, BYTE* ptr)
{
  *(ptr  ) = n & 0xFF;
  n>>=8;
  *(ptr+1) = n & 0xFF;
  n>>=8;
  *(ptr+2) = n & 0xFF;
  n>>=8;
  *(ptr+3) = n;
}

// Header, die für DATABASE nötig sind

#ifdef __MSDOS__
  #include <mem.h>
  #include <io.h>
  #include <share.h>
#else
	#include "config.h"
	#include <errno.h>
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
#endif

#ifdef DEBUG
	#ifndef SECURE
		#define SECURE
	#endif
#endif

#ifdef SECURE
	#include <stdio.h>
	#include <stdlib.h>
#endif	

#ifdef DEBUG
  #define ERRORMSG(out1) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1);fclose(stream);}
  #define ERRORMSG1(out1,out2) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2);fclose(stream);}
  #define ERRORMSG2(out1,out2,out3) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2,out3);fclose(stream);}
  #define ERRORMSG3(out1,out2,out3,out4) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2,out3,out4);fclose(stream);}
  #define ERRORMSG4(out1,out2,out3,out4,out5) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2,out3,out4,out5);fclose(stream);}
  #define ERRORMSG5(out1,out2,out3,out4,out5,out6) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2,out3,out4,out5,out6);fclose(stream);}
  #define ERRORMSG6(out1,out2,out3,out4,out5,out6,out7) {FILE *stream;stream=fopen("database.log","a");fprintf(stream,out1,out2,out3,out4,out5,out6,out7);fclose(stream);}
#else
  #define ERRORMSG(out1)
  #define ERRORMSG1(out1,out2)
  #define ERRORMSG2(out1,out2,out3)
  #define ERRORMSG3(out1,out2,out3,out4)
  #define ERRORMSG4(out1,out2,out3,out4,out5)
  #define ERRORMSG5(out1,out2,out3,out4,out5,out6)
  #define ERRORMSG6(out1,out2,out3,out4,out5,out6,out7)
#endif

/****************************************************************************
 *                                                                          *
 * dbError                                                                  *
 *                                                                          *
 ****************************************************************************/
class dbError
{
  private:
    const char *text;			// Zeiger auf den Text des letzten Fehlers
		//char buffer[255];		// für set syserror
  protected:
    dbError() {text="none";}
    void seterror(const char *t){ERRORMSG1("%s\n",t);text=t;}
		#ifdef __MSDOS__
    void seterror(){text=sys_errlist[errno];ERRORMSG(text);}
		#else
		void seterror(){text=strerror(errno);ERRORMSG(text);}
		#endif
  public:
    const char *lasterror(){return text;}
};

inline char upcase(char a)
{
  if (a>='a' && a<='z')
    return a+=('A'-'a');
  return a;
}

#endif
