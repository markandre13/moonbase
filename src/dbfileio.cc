/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1993,95 Mark-André Hopf                                    	*
 *  filename: dbfileio.cc                                                   *
 *  relation: database.cc                                                   *
 *  author  : Mark-André Hopf                                               *
 *  date    : 30.04.1996                                                    *
 *                                                                          *
 ****************************************************************************/

//#include <typeinfo>
//#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <moonbase/owndef.hh>
#include <moonbase/dbheader.hh>
#include <moonbase/dbmemoio.hh>
#include <moonbase/dblcklst.hh>
#include <moonbase/dbfileio.hh>
#include <moonbase/dbdata.hh>
#include <moonbase/moonbase.hh>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/****************************************************************************
 *                                                                          *
 * Konstruktor                                                              *
 *                                                                          *
 ****************************************************************************/
dbFileIO::dbFileIO() : dbHeader()
{
  buffer=NULL;                      // noch kein Buffer
  bIgnoreDeleted=false;
  bBufferModified=false;
  ulRecNo.Init(this);
  ulRecNo=0;
}

/****************************************************************************
 *                                                                          *
 * Destruktor                                                               *
 *                                                                          *
 ****************************************************************************/
dbFileIO::~dbFileIO()
{
  if (handle)                     // Datei wurde nicht geschlossen
  {
    fileclose();
  }
}

/****************************************************************************
 *                                                                          *
 * void RecNoFunc()                                                         *
 * Diese Routine wird immer aufgerufen, wenn ulRecNo ein Wert zugewiesen    *
 * wird.                                                                    *
 *                                                                          *
 ****************************************************************************/
void dbFileIO::RecNoFunc()
{
  bRecNoChanged=true;							// flag setzen, daß sich ulRecNo geändert hat
  if (!handle) return;
	// Wenn in den Buffer geschrieben wurde, dessen Inhalt speichern und den
	// Schreibschutz auf den dazugehörigen Record entfernen
  if (bLockedByWrite && savebuffer() && unlock(ulBufferRec) )
  {
    bSureBufferIsLocked =false;
    bLockedByWrite      =false;
  }
}

/****************************************************************************
 *                                                                          *
 * Die Datei öffnen                                                         *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::fileopen(const char* name)
{
  ERRORMSG("dbFileIO.fileopen: entry\n");
  if (handle)
    THROW("dbFileIO.fileopen(): FATAL! File is already open");
  if (buffer)
    THROW("dbHeader::fileopen(): FATAL! Found old buffer");

  if (!dbHeader::fileopen(name)) return false;

	create_buffer();

  // Memodatei öffnen falls nötig
	WORD i;
  for(i=1; i<=FldCount(); i++)
  {
    if (FieldType(i)=='M')
    {
      memoopen(Dbf());
      return true;
    }
  }
	return true;
}

void dbFileIO::create_buffer()
{
	ERRORMSG1("dbFileIO.create_buffer(): RecCount()=%lu\n",RecCount());
  // Den Datensatzbuffer anlegen:
  buffer=new char[RecSize()+1]; // Buffer für einen Datensatz reservieren

  // Flags für den Buffer setzen
  bSureBufferIsLocked = false;
  bLockedByWrite      = false;
  bBufferModified     = false;
  ulRecNo=1;                    // (dies setzt auch bRecNoChanged)
}

bool dbFileIO::create_begin(const char *file, WORD fieldcount)
{
	if (buffer)
	{
		printf("dbFileIO.create_begin(): FATAL! buffer still exists\n");
		exit(1);
	}
//		THROW("dbFileIO.create_begin(): FATAL! buffer still exists");
	return dbHeader::create_begin(file, fieldcount);
}

bool dbFileIO::create_end()
{
	if (dbHeader::create_end())
	{
		create_buffer();
		return true;
	}
	return false;
}

/****************************************************************************
 *                                                                          *
 * Die Datei schliessen                                                     *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::fileclose()
{
  register bool bStatus;

  ERRORMSG("dbFileIO.fileclose: entry\n");
  if (buffer)
  {
    seterror("dbFileIO.fileclose: delete[] buffer");
    savebuffer();
    delete[] buffer;
    buffer=NULL;
  }
  seterror("dbFileIO.fileclose: UnlockAll");
  UnlockAll();

  // memoclose();

  bStatus=dbHeader::fileclose();
  seterror("dbFileIO.fileclose: exit");
  return bStatus;
}

/****************************************************************************
 *                                                                          *
 * readrecord                                                               *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::readrecord(ULONG nr, char* buffer)
{
  ERRORMSG ("dbFileIO.readrecord: entered\n");
  if (!handle)
    THROW("dbFileIO.readrecord(): FATAL! Invaild handle");

  memset(buffer,' ',RecSize()); // clear buffer in case of errors...

  if ( Is_it_EOF(ulRecNo) )
  {
    seterror("dbFileIO.readrecord: EOF");
    return false;
  }

  ERRORMSG3("dbFileIO.readrecord: lseek(%i,getoff(%i-1)=%lu,SEEK_SET)\n",handle,nr,getoff(nr-1));
  if (lseek(handle, getoff(nr), SEEK_SET)==-1L) // Dateizeiger setzen
		THROW("dbFileIO.readrecord(): lseek failed");
  ERRORMSG2("dbFileIO.readrecord: read(%i,buffer,%i)\n",handle, RecSize());
  if (read(handle, buffer, RecSize())!=RecSize()) // Datensatz einlesen
    THROW("dbFileIO.readrecord(): read failed");
  buffer[RecSize()]=0;
  ERRORMSG1("dbFileIO.readrecord: read :\"%s\"\n",buffer);
  return true;
}

/****************************************************************************
 *                                                                          *
 * writerecord                                                              *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::writerecord(ULONG nr, char* buffer)
{
	if ( Is_it_EOF(nr) )
		THROW("dbFileIO.writerecord(): EOF");
	
	/*
  if ( Is_it_EOF(ulRecNo) )
		THROW("dbFileIO.writerecord(): EOF");
	*/
	
  if (lseek(handle, getoff(nr), SEEK_SET)==-1L) // Dateizeiger setzen
    THROW("dbFileIO.writerecord(): lseek failed");

  ERRORMSG2("dbFileIO.writerecord: write(%i,buffer,%i)\n",handle, RecSize());
  ERRORMSG1("                      buffer=\"%s\"",buffer);
  if (write(handle, buffer, RecSize())!=RecSize()) // Datensatz einlesen
		THROW("dbFileIO.writerecord(): write failed");

  return true;
}

/****************************************************************************
 *                                                                          *
 * loadbuffer                                                               *
 * Laedt den aktuellen Datensatz in den Buffer. Wenn ein vorheriger Buffer- *
 * inhalt nicht gespeichert wurde, dann geht dieser verlohren!              *
 * true  : buffer enthält die gelesenen Daten                               *
 * false : buffer enthält einen leeren String                               *
 *         - EOF bzw. ulRecNo ausserhalb des zulaessigen Bereichs           *
 *         - read via readrecord funktionierte nicht                        *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::loadbuffer()
{
  ERRORMSG("dbFileIO: loadbuffer\n");
  if (!handle)
    THROW("dbFileIO.loadbuffer: FATAL! handle==0\n");

  if (!readrecord(ulRecNo, buffer))
    return false;

  if (bLockedByWrite && ulBufferRec!=ulRecNo)
    unlock(ulBufferRec);
  ulBufferRec=ulRecNo;
  bRecNoChanged       = false;
  bSureBufferIsLocked = false;
  bLockedByWrite      = false;
  bBufferModified     = false;
  return true;
}

/****************************************************************************
 *                                                                          *
 * savebuffer                                                               *
 * Den Buffer in die db-Datei schreiben wenn er geaendert wurde.            *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::savebuffer()
{
  if (bBufferModified)            // buffer was modified & is locked
  {
  	#ifdef DEBUG2
		printf("dbFileIO::savebuffer(): saving record %lu\n",ulBufferRec);
		#endif
    if (!writerecord(ulBufferRec, buffer))
      return false;
    bBufferModified=false;        // the buffer is not modified anymore
  }
  return true;
}

/****************************************************************************
 *                                                                          *
 * Den Buffer aktualisieren und true zurueckliefern, wenn sich dadurch der  *
 * Bufferinhalt veraendert hat                                              *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::updatebuffer()
{
  if (handle && bRecNoChanged)
  {
    savebuffer();
    loadbuffer();
    return true;
  }
  return false;
}

/****************************************************************************
 *                                                                          *
 * String in den FileIO Buffer kopieren                                     *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::writefield(WORD field,const char* src)
{
  if (!handle)
   	THROW("dbFileIO.writefield: FATAL! handle==0");
  if (RecCount()==0)
  	THROW("dbFileIO::writefield: db-file is empty");
  if (field>FldCount())
    THROW("dbFileIO::writefield: field number is out of range");

  updatebuffer();

  // if (!bSureBufferIsLocked)
  if (!locklist.is(ulBufferRec))
  {
    if (!RLock())
      THROW("dbFileIO::writefield: record is locked by another user");
    bSureBufferIsLocked=true;
    bLockedByWrite     =true;
  }

  WORD len=strlen(src);
  if (len>FieldLen(field)) len=FieldLen(field);

  memset(buffer+FieldOfs(field), ' ', FieldLen(field));
  memcpy(buffer+FieldOfs(field), src, len);

  bBufferModified=true;
  return true;
}

/****************************************************************************
 *                                                                          *
 * String aus dem FileIO Buffer lesen                                       *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::readfield(WORD field, char* dst, short maxlen)
{
  ERRORMSG("dbFileIO.readfield: entered\n");
  if (!handle)
    THROW("dbFileIO::readfield(): FATAL! handle==0\n");
  if (field>FldCount())
    THROW("dbFileIO::readfield(): field number is out of range");
  updatebuffer();
	ERRORMSG2("readfield: file:%s line:%i\n",__FILE__,__LINE__);
  WORD len=FieldLen(field);
  if (len>maxlen) len=maxlen;

  ERRORMSG1("dbFileIO.readfield: buffer = \"%s\"\n",buffer);
  ERRORMSG1("                    field  = %i\n"    ,field);
  ERRORMSG1("                    offset = %lu\n"   , (ULONG)FieldOfs(field));
  ERRORMSG1("                    len    = %u\n",(WORD) len);

  memcpy(dst, buffer+FieldOfs(field), len);
  dst[len]='\0';

  ERRORMSG1("                    dst    = \"%s\"",dst);

  return true;
}

/****************************************************************************
 *                                                                          *
 * Deleted                                                                  *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Deleted()
{
  if (!handle) return false;
  updatebuffer();
  return (buffer[0]=='*')?true:false;
}

/****************************************************************************
 *                                                                          *
 * Delete                                                                   *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Delete()
{
  if (!handle)
  {
    seterror("Keine db-Datei geoeffnet");
    return false;
  }
  return set_delete_flag(true);
}

/****************************************************************************
 *                                                                          *
 * Recall                                                                   *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Recall()
{
  if (!handle)
  {
    seterror("Keine db-Datei geoeffnet");
    return false;
  }
  return set_delete_flag(false);
}

/****************************************************************************
 *                                                                          *
 * set_delete_flag                                                          *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::set_delete_flag(bool del)
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbFileIO.set_delete_flag: FATAL! Invaild handle\n");
    exit(1);
  }
  #endif
  updatebuffer();

  // if (!bSureBufferIsLocked)
  if (!locklist.is(ulBufferRec))
  {
    if (!RLock())
    {
      seterror("dbFileIO.writefield: record is locked by another user");
      return false;
    }
    bSureBufferIsLocked=true;
    bLockedByWrite     =true;
  }

  // set flag
  buffer[0] = del?'*':' ';
  bBufferModified=true;

  return true;
}

bool dbFileIO::SetDeleted(bool on)
{
  bIgnoreDeleted=on;
  return false;
}

/****************************************************************************
 *                                                                          *
 * COPY MEMO <Name des Memofeldes> TO <Dateiname> [ADDITIVE]                *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::CopyMemoTo(const char *field, const char *file, bool add)
{
  WORD nr;
  ULONG u;

  if (!handle)
  {
    seterror("Keine db-Datei geöffnet");
    return false;
  }

  if ( (nr=FieldNo(field)) == 0 )
    return false;

  if (FieldType(nr)!='M')
  {
    seterror("Feld muss ein Memofeld sein");
    return false;
  }

  char sBlockNr[11];
  readfield(nr, sBlockNr, 10);
  u=atoi(sBlockNr);

  return CopyMemoToFile(u,file,add);
}

/****************************************************************************
 *                                                                          *
 * Skip, Go, Bof, Eof, RecNo                                                *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Skip(long dir)
{
  if (!handle)
  	THROW("dbFileIO::Skip(): no db-file opened");

  // no movement, just take care of flags (calls 'RecNoFunc')
  if (dir==0){ulRecNo=ulRecNo;return true;}

  // check EOF
  if ( Is_it_EOF(ulRecNo) && dir>0 )
  {
    seterror("reached end of db-file");
    return false;
  }
  // check BOF and prevent overflow
  if ( (dir<0) && (ULONG(-dir))>ulRecNo )
  {
    seterror("reached beginning of db-file\n");
    ulRecNo=0;
    return false;
  }

  if (!bIgnoreDeleted)
  {
    // DELETED OFF
    ULONG pos=ulRecNo+dir;
    ulRecNo=pos;
    if ( Is_it_EOF(pos) )
    {
	    seterror("reached end of db-file");
      ulRecNo=RecCount()+1;
      return false;
    }
  }
  else
  {
    // DELETED ON
    long step = dir<0?-1L:1L;
    if (dir>0)
    {
      step=1L;
    }
    else
    {
      step=-1L;
      dir=-dir;
    }

    while(dir>0)
    {
      ulRecNo=ulRecNo+step;       // move
      if (ulRecNo==0)             // test position
      {
		    seterror("reached beginning of db-file\n");
        return false;
      }
      if ( Is_it_EOF(ulRecNo) )
      {
		    seterror("reached end of db-file");
        return false;
      }
      if (!Deleted()) dir--;
    }
  }

  return true;
}

/****************************************************************************
 *                                                                          *
 * Go                                                                       *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Go(ULONG pos)
{
  if (!handle)
  {
    seterror("Keine db-Datei geoeffnet");
    return false;
  }
  if ( Is_it_EOF(pos) )
  {
    seterror("Kein gueltiger Datensatz");
    return false;
  }
  ulRecNo=pos;
  return true;
}

/****************************************************************************
 *                                                                          *
 * AppendBlank                                                              *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::AppendBlank()
{
	ERRORMSG2(__FILE__", %lu: begin AppendBlank with RecCount()=%lu\n",(ULONG)__LINE__,RecCount());
  if (!handle)
  {
    seterror("Keine db-Datei geoeffnet");
    return false;
  }

  ULONG nr=add_locked_record();
	ERRORMSG3(__FILE__", %lu:  added, nr=%lu, RecCount()=%lu\n",(ULONG)__LINE__,nr,RecCount());
  if (nr==0) return false;
  ulRecNo=nr;

  savebuffer();               // save old buffer
	ERRORMSG2(__FILE__", %lu:  saved old buffer, RecCount()=%lu\n",(ULONG)__LINE__,RecCount());
  ulBufferRec=nr;
  bSureBufferIsLocked=true;
  bLockedByWrite=true;
  locklist.add(nr);
  memset(buffer,' ',RecSize());
  bRecNoChanged=false;        // no need to load the buffer
	ERRORMSG2(__FILE__", %lu: end AppendBlank with RecCount()=%lu\n",(ULONG)__LINE__,RecCount());
	return true;
}

/****************************************************************************
 *                                                                          *
 * Bof                                                                      *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Bof()
{
  if (!handle) return true;
  return ulRecNo?false:true;
}

/****************************************************************************
 *                                                                          *
 * Eof                                                                      *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Eof()
{
  if (!handle) return true;
  return Is_it_EOF(ulRecNo)?true:false;
}

/****************************************************************************
 *                                                                          *
 * RecNo                                                                    *
 *                                                                          *
 ****************************************************************************/
ULONG dbFileIO::RecNo()
{
  if (!handle) return 0L;
  return ulRecNo?ulRecNo:1L;
}

/****************************************************************************
 *                                                                          *
 * Einen Datensatz sperren                                                  *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::lock(ULONG nr)
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbFileIO.lock: FATAL! Invalid handle\n");
    exit(1);
  }
  #endif
  if (locklist.is(nr)) return true;
  #ifdef __MSDOS__
    if (::lock(handle, getoff(nr), RecSize())==-1)
    {
      seterror("dbFileIO.lock: record is already locked by other user");
      return false;
    }
  #else
    struct flock lck;
    lck.l_type   = F_WRLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = getoff(nr);
    lck.l_len    = RecSize();
    if (fcntl(handle, F_SETLK, &lck))
    {
      seterror("dbFileIO.lock: record is already locked by other user");
      return false;
    }
  #endif
  locklist.add(nr);
  return true;
}

/****************************************************************************
 *                                                                          *
 * Einen Datensatz freigeben                                                *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::unlock(ULONG nr)
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbFileIO.unlock: FATAL! Invaild handle\n");
    exit(1);
  }
  #endif
  if (locklist.is(nr))
  {
    #ifdef __MSDOS__
      if (::unlock(handle, getoff(nr), RecSize())==-1)
      {
        seterror("dbFileIO.unlock: can't free a lock i own");
        return false;
      }
    #else
      struct flock lck;
      lck.l_type   = F_UNLCK;
      lck.l_whence = SEEK_SET;
      lck.l_start  = getoff(nr);
      lck.l_len    = RecSize();
      if (fcntl(handle, F_SETLK, &lck))
      {
        seterror("dbFileIO.unlock: can't free a lock i own");
        return false;
      }
    #endif
    locklist.del(nr);
  }
  return true;
}

/****************************************************************************
 *                                                                          *
 * RLock()                                                                  *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::RLock()
{
  if (!handle) return true;
  if (lock(ulRecNo))
  {
    if (ulRecNo==ulBufferRec) bLockedByWrite=false;
    return true;
  }
  return false;
}

/****************************************************************************
 *                                                                          *
 * Unlock                                                                   *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::Unlock()
{
  if (!handle) return true;
  if (ulRecNo==ulBufferRec)
  {
    if (bBufferModified)
      if (!savebuffer()) return false;
    bSureBufferIsLocked =false;
    bLockedByWrite      =false;
    bBufferModified     =false;
  }
  return unlock(ulRecNo);
}

/****************************************************************************
 *                                                                          *
 * UnlockAll                                                                *
 *                                                                          *
 ****************************************************************************/
bool dbFileIO::UnlockAll()
{
  // pay attention to the buffer !!!
  // save it and set the flags!!!
  seterror("dbFileIO.UnlockAll: i still do not exist... :( ");
  return false;
}
