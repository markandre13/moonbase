/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1993,95 Mark-André Hopf                                    *
 *  filename: dbmemoio.cpp                                                  *
 *  relation: dbfileio.cpp                                                  *
 *  author  : Mark-André Hopf                                              *
 *  date    : 12.04.1995                                                    *
 *                                                                          *
 ****************************************************************************/

//#include <typeinfo>
#include <cstddef>
#include <cstring>
#include <moonbase/owndef.hh>
#include <moonbase/dbheader.hh>
#include <moonbase/dbmemoio.hh>
#include <moonbase/dblcklst.hh>
#include <moonbase/dbfileio.hh>
#include <moonbase/dbdata.hh>
#include <moonbase/moonbase.hh>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

dbMemoIO::~dbMemoIO()
{
	if (fdMemo) close(fdMemo);
}

bool dbMemoIO::memoopen(const char *dbfile)
{
  // Dateinamen erzeugen
  if (filename!=NULL) delete[] filename;
  size_t len=strlen(dbfile);
  filename=new char[len+1];
  strcpy(filename,dbfile);
  filename[len-1]='t';          // Endung ".dbf" in ".dbt" abaendern

  // Datei oeffnen (ReadWrite,Binary,Shared)
  #ifdef __MSDOS__
    fdMemo=::open(filename, O_RDWR|O_BINARY|SH_DENYNONE);
  #else
    fdMemo=::open(filename, O_RDWR|O_SYNC);
  #endif
  if (fdMemo==-1)
  {
    seterror("no memofile found");
    fdMemo=0;
    delete[] filename;
    filename=NULL;
    return false;
  }

  // Buffer erzeugen
  if (!buffer)
  {
    buffer=new unsigned char[512];
  }

  // Das waere zunaechst einmal alles...
  return true;
}

bool dbMemoIO::memoclose()
{
  if (fdMemo) close(fdMemo);
  if (buffer)
  {
    delete[] buffer;
    buffer=NULL;
  }
  if (filename!=NULL) delete[] filename;
  return true;
}

bool dbMemoIO::CopyMemoToFile(ULONG nr, const char* file, bool additive)
{
  ULONG len;
  int fd=0;

  if (!nr){seterror("memofield is empty");return false;}
  if (!fdMemo){seterror("no memofile opened"); return false; }

  if (!loadblock(nr)) goto rderror;

  if (additive)
  {
    if ( (fd=open(file, O_RDWR|O_APPEND)) == -1 )
    {
      seterror();
      if (errno!=ENOENT) return false;
      fd=0;
    }
  }

  if (fd==0)
  {
    if ( (fd=open(file, O_CREAT|O_TRUNC|O_RDWR, S_IREAD|S_IWRITE)) == -1 )
    {
      seterror();
      return false;
    }
  }

  len=NoEndianLong(buffer+4);

  // write 1st block
  register ULONG l;
  l= (len>512UL) ? 512UL-8UL : len-8UL;   // length of the 1st blocks text
  if (write(fd, buffer+8UL, l)!=(int)l) goto wrerror;
  len-=l+8UL;

  // write rest
  while(len>0UL)
  {
    if (!loadblock(++nr)) goto rderror;
    l=(len>512UL)?512UL:len;
    if (write(fd, buffer+8UL, l)!=(int)l) goto wrerror;
    len-=l;
  }

  close(fd);
  return true;

  wrerror:
  seterror();
  if (fd) close(fd);
  return false;

  rderror:
  seterror();
  if (fd) close(fd);
  return false;
}

bool dbMemoIO::loadblock(ULONG nr)
{
  if (lseek(fdMemo, nr*0x200UL, SEEK_SET)==-1L)
  {
    seterror();
    return false;
  }
  if (read(fdMemo, buffer, 0x200)!=0x200)
  {
    seterror();
    return false;
  }
  return true;
}
