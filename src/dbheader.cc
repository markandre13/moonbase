/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994 Mark-André Hopf                                        *
 *  filename: dbheader.cpp                                                  *
 *  relation: database.cpp                                                  *
 *  author  : Mark-André Hopf                                               *
 *  date    : 24.04.1995                                                    *
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __MSDOS__
  #include <dos.h>
#endif

/***************************************************************************
 *                                                                         *
 * Datenbank öffnen und den Header laden                                   *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::fileopen(const char *file)
{
  ERRORMSG("dbHeader.fileopen: entry\n");

  #ifdef SECURE
  if (handle)
  {
    printf("dbHeader.fileopen: FATAL! A db-file is already open\n");
    exit(1);
  }
  if (fdCreate)
  {
  	printf("dbHeader.fileopen: FATAL! A db-file is opened for creation\n");
  	exit(1);
  }
  #endif

  // Dateinamen anlegen
  set_filename(file);

  // Datei öffnen
  ERRORMSG("dbHeader.fileopen: open\n");
  #ifdef __MSDOS__
    handle=open(filename, O_RDWR|O_BINARY|SH_DENYNONE);
  #else
    handle=open(filename, O_RDWR|O_SYNC);
  #endif
  if (handle==-1)
  {
//    seterror("dbHeader.fileopen: open failed");
		seterror();
    handle=0;
    delete[] filename;
    filename=NULL;
    ERRORMSG("dbHeader.fileopen: exit\n");
    return false;
  }

  // Header laden
  ERRORMSG("dbHeader.fileopen: loadheader\n");
  if (!loadheader())
  {
    close(handle);
    handle=0;
    delete[] filename;
    filename=NULL;
    ERRORMSG("dbHeader.fileopen: exit\n");
    return false;
  }

  // Fertig
  ERRORMSG("dbHeader.fileopen: exit\n");
  return true;
}

/***************************************************************************
 *                                                                         *
 * Datenbank schließen und vorher evtl. den Header speichern               *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::fileclose()
{
  ERRORMSG("dbHeader.fileclose: entry\n");
  if (!handle)
  {
    ERRORMSG("dbHeader.fileclose: exit\n");
    return true;           // schon geschlossen
  }
  // hier evtl. noch ein neues Datum in den Header schreiben
  ERRORMSG("dbHeader.fileclose: close\n");
  close(handle);
  handle=0;

  // Den Filenamen beseitigen
  if (filename)
  {
    ERRORMSG("dbHeader.fileclose: delete filename\n");
    delete[] filename;
    filename=NULL;
  }

  // Die Felddefinitionen beseitigen
  if (fields)
  {
    ERRORMSG("dbHeader.fileclose: delete fields\n");
    delete[] fields;
    fields=NULL;
  }

  // Die Feldindexeinträge beseitigen
  if (fieldindex)
  {
    ERRORMSG("dbHeader.fileclose: delete fieldindex\n");
    delete[] fieldindex;
    fieldindex=NULL;
  }

  ERRORMSG("dbHeader.fileclose: exit\n");
  return true;
}

/***************************************************************************
 *                                                                         *
 * Header laden                                                            *
 * - Nicht fuer ein aktualisieren des Header im Speicher verwenden         *
 * - Nur zum Aufruf durch fileopen gedacht                                 *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::loadheader()
{
  // ULONG sizeof_defHeader=32;
  int i;
  ptrdiff_t a;
  ULONG len;

  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.loadheader: FATAL! No valid file handle\n");
    exit(1);
  }
  if (fields)
  {
    printf("dbHeader.loadheader: FATAL! Found old fields\n");
    exit(1);
  }
  if (fieldindex)
  {
    printf("dbHeader.loadheader: FATAL! Found old fieldindex\n");
    exit(1);
  }
  #endif

  // Ersten Teil des Headers einlesen
  if ( !hdr_read(handle, &header, sizeof(defHeader)) )
  {
    seterror("dbHeader.loadheader: 1st read failed");
    return false;
  }

  // BigEndian-LittleEndian Unabhängigkeit sicherstellen
  header.RecCount  =NoEndianLong((BYTE*)&header.RecCount);
  header.HeaderSize=NoEndianWord((BYTE*)&header.HeaderSize);
  header.RecSize   =NoEndianWord((BYTE*)&header.RecSize);

  // Anzahl Felder errechnen
  nFldCount = (header.HeaderSize-sizeof(defHeader)) / (ULONG)sizeof(defField);

  ERRORMSG1("dbheader.loadheader: nFldCount = %lu\n",(ULONG)nFldCount);

  // Speicher für die Felddefinitionen reservieren
  if (  (!(fields     = new defField[nFldCount]))
      ||(!(fieldindex = new ptrdiff_t[nFldCount])) )
    { seterror("dbHeader.loadheader: new failed"); return false; }

  // Zweiten Teil des Headers, die Felddefinitionen, einlesen
  len = sizeof(defField)*nFldCount;
  if( !hdr_read(handle, fields, len) )
    { seterror("dbHeader.loadhader: 2nd read failed"); return false;}

  // Dateioffset des ersten Datensatzes fuer andere Klassen notieren
  ulFieldOffset=(ULONG) header.HeaderSize;
  #ifdef SECURE
    if (ulFieldOffset!=sizeof(defHeader)+sizeof(defField)*nFldCount+1)
    {
      printf("dbHeader.loadheader: FATAL! Wrong calculations\n");
      exit(1);
    }
  #endif

  // Liste der Offsets aller Felder im Datensatz erstellen
  a=1;  // (1.Zeichen ist die Loeschmakierung, wird uebergangen)
  for (i=0; i<nFldCount; i++)
  {
    fieldindex[i]=a;
    a+=fields[i].Length;
  }

  return true;
}

/***************************************************************************
 *                                                                         *
 * create_begin                                                            *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::create_begin(const char *file, WORD fieldcount)
{
	#ifdef SECURE
		if (handle)
		{
			printf("dbHeader.create_begin: FATAL! file is already open\n"); 
			exit(1);
		}
		if (fdCreate)
		{
			printf("dbHeader.create_begin: FATAL! file is already opened for creation\n");
			exit(1);
		}
  if (fields)
  {
    printf("dbHeader.create_begin: FATAL! Found old fields\n");
    exit(1);
  }
  if (fieldindex)
  {
    printf("dbHeader.create_begin: FATAL! Found old fieldindex\n");
    exit(1);
  }
	#endif
	
	if (!fieldcount)
	{
		seterror("dbHeader.create_begin: db-file with no fields is nonsense");
		return false;
	}
	
	set_filename(file);
	if ((fdCreate=open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE))==-1)
	{
		seterror("dbHeader.create_begin: open failed");
		goto error02;
	}

	// Schreibsperre setzen
  struct flock lck;
  lck.l_type   = F_WRLCK;
  lck.l_whence = SEEK_SET;
  lck.l_start  = 0;
  lck.l_len    = 0;					// 0 bedeutet bis zum jetzigen & zukünftigen Dateiende
  if (fcntl(handle, F_SETLK, &lck))
  {
    seterror("dbFileIO.create_begin: can't lock new file");
    goto error01;
  }

	// Anzahl zu erzeugender & bereits erzeugter Felder merken
	nFldCount				=	fieldcount;
	nCreateFldAdded	=	0;
	header.RecSize	= 1;			// Jetzige Länge ist für die Löschmakierung

  // Speicher für die Felddefinitionen reservieren
  if (  (!(fields     = new defField[nFldCount]))
      ||(!(fieldindex = new ptrdiff_t[nFldCount])) )
    { seterror("dbHeader.create_begin: new failed"); goto error01; }

	return true;
	
	error01:
		close(fdCreate);
	error02:
		fdCreate=0;
		delete[] filename;
		filename=NULL;	
		return false;    
}

/***************************************************************************
 *                                                                         *
 * create_add                                                              *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::create_add(const char *name, char type, BYTE len, BYTE dec)
{
	if (!fdCreate)
	{
		seterror("dbHeader.create_add: no file opened for creation");
		return false;
	}
	if (nCreateFldAdded>=nFldCount)
	{
		seterror("dbHeader.create_add: all fields were added, can't add anymore");
		return false;
	}
	
	// Headerdaten in das zugehörige Feld eintragen (Überprüfung & Anpassung fehlt!)
	memset(&fields[nCreateFldAdded], 0, sizeof(defField));
	strncpy(fields[nCreateFldAdded].Name, name, 10);
	fields[nCreateFldAdded].Type		=type; 
	fields[nCreateFldAdded].Length	=len; 
	fields[nCreateFldAdded].Decimals=dec; 
	nCreateFldAdded++;

	header.RecSize+=len;
	
	return true;
}

/***************************************************************************
 *                                                                         *
 * create_end                                                              *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::create_end()
{
	if (!fdCreate)
	{
		seterror("dbHeader.create_end: no file opended for creation");
		return false;
	}
	if (nCreateFldAdded!=nFldCount)
	{
		seterror("dbHeader.create_end: definition incomplete, can't finish creation");
		return false;
	}
	
	// Weitere Headerdaten eintragen
  ULONG len = sizeof(defField)*nFldCount;

  header.Version 		= 3;			// 0x3:dBASE III file format (+0x80 = with .DBT file)  
  header.Year				= 0;
	header.Month			= 0;
	header.Day				= 0;
	header.RecCount		= 0;
	header.HeaderSize	= sizeof(defHeader)+len+1;
	header.Transaction= 0;
	header.Crypted		= 0;
	header.MDXexist		= 0;

  ulFieldOffset=(ULONG) header.HeaderSize;
  // Liste der Offsets aller Felder im Datensatz erstellen
  ptrdiff_t a=1;  // (1.Zeichen ist die Löschmakierung und wird übergangen)
  for (int i=0; i<nFldCount; i++)
  {
    fieldindex[i]=a;
    a+=fields[i].Length;
  }
	
	// Ersten Teil des Headers schreiben
  WrNoEndianLong(header.RecCount	, (BYTE*)&header.RecCount);
  WrNoEndianWord(header.HeaderSize, (BYTE*)&header.HeaderSize);
  WrNoEndianWord(header.RecSize		, (BYTE*)&header.RecSize);
  if(write(fdCreate, &header, sizeof(defHeader))!=sizeof(defHeader) )
  {
  	seterror("dbHeader.create_end: 1st write failed");
  	goto error01;
  }
  header.RecCount  =NoEndianLong((BYTE*)&header.RecCount);
  header.HeaderSize=NoEndianWord((BYTE*)&header.HeaderSize);
  header.RecSize   =NoEndianWord((BYTE*)&header.RecSize);
  
  // Zweiten Teil des Headers schreiben
  if(write(fdCreate, fields, len)!=(int)len )
  {
  	seterror("dbHeader.create_end: 2nd write failed");
  	goto error01;
  }
  
  // Die Endmakierungen anfügen
  char end_of_header[2];
  end_of_header[0]=0x0D;	// Ende des Headers
  end_of_header[1]=0x1A;	// Ende der Datensätze
  if(write(fdCreate,end_of_header, 2)!=2 )
  {
  	seterror("dbHeader.create_end: 3rd write failed");
  	goto error01;
  }
	
	// Sperre aufheben und mit der Datei ein reguläres Arbeiten ermöglichen
  struct flock lck;
  lck.l_type   = F_UNLCK;
  lck.l_whence = SEEK_SET;
  lck.l_start  = 0;
  lck.l_len    = 0;					// 0 bedeutet bis zum jetzigen & zukünftigen Dateiende
  if (fcntl(handle, F_SETLK, &lck))
  {
    seterror("dbFileIO.create_begin: can't unlock new file");
    return false;
  }
  
  // Das Dateihandle freigeben
  handle=fdCreate;
  fdCreate=0;
  
  return true;			// dbFileIO muß jetzt noch die dbDatas erzeugen
  
  error01:
  	close(fdCreate);
  	fdCreate=0;
  	return false;
}
/***************************************************************************
 *                                                                         *
 * Neuen 'filename' setzen und evtl. noch die Endung ".dbf" anhaengen,     *
 * falls sie noch nicht vorhanden ist.                                     *
 *                                                                         *
 ***************************************************************************/
void dbHeader::set_filename(const char *newname)
{
  if (filename)
  {
    ERRORMSG("dbHeader.set_filename: delete filename\n");
    delete[] filename;
  }

  #ifdef __MSDOS__
    strlwr(newname);
  #endif

  size_t len=strlen(newname);
  if (strcmp(newname+len-4, ".dbf")==0)
  {
    filename=new char[len+1];
    ERRORMSG1("dbHeader.set_filename: filename = new char[%u]\n",(unsigned)len+1);
    strcpy(filename,newname);
  }
  else
  {
    filename=new char[len+5];
    ERRORMSG1("dbHeader.set_filename: filename = new char[%u]\n",(unsigned)len+5);
    strcpy(filename,newname);
    strcpy(filename+len,".dbf");
  }
}

/***************************************************************************
 *                                                                         *
 * Daten aus dem Header lesen, sobald niemand mehr in den Header schreibt  *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::hdr_read(int fd, void *buf, unsigned len)
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.hdr_read: FATAL! No valid file handle\n");
    exit(1);
  }
  #endif

  #ifdef __MSDOS__
    while(read(fd, buf, len)!=len)
    {
      if(errno!=EACCES) return false;
      sleep(1);       // wait a second, then try again
    }
    return true;
  #else
    // set read lock
    struct flock lck;
    lck.l_type   = F_RDLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);
    if (fcntl(handle, F_SETLKW, &lck))
    {
      seterror("dbFileIO.lock: deadlock");
      return false;
    }
    lck.l_type   = F_UNLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);

    if(read(fd, buf, len)!=(int)len)
    {
      seterror();
      fcntl(handle, F_SETLK, &lck);
      return false;
    }

    // clear read lock
    if (fcntl(handle, F_SETLK, &lck))
    {
      seterror();
      return false;
    }

    return true;
  #endif
}

/*****************************************************************************
 *                                                                           *
 * Add a new blank record and lock it.                                       *
 *                                                                           *
 *****************************************************************************/
ULONG dbHeader::add_locked_record()
{
	ERRORMSG2(__FILE__", %lu:  begin add_locked_record(), header.RecCount=%lu\n",(ULONG)__LINE__,header.RecCount);
  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.add_locked_record: FATAL! No valid file handle\n");
    exit(1);
  }
  #endif

  char *empty=new char[RecSize()+1];  // leerer Record + Endmakierung
  memset(empty,' ',RecSize());
  empty[RecSize()]=0x1A;              // Endmakierung der db-Datei
  unsigned char buffer[4];
  ULONG n;

  // wait for write lock
  if (!hdr_lock()) goto error;
  // read present RecCount
  if (lseek(handle,4UL,SEEK_SET)==-1) goto error;
  if (read(handle, buffer, 4)!=4) goto error;
  n=NoEndianLong(buffer);
	ERRORMSG2(__FILE__", %lu:   read RecCount from file: n=%lu\n",(ULONG)__LINE__,n);

  // update own RecCount
  if (n>header.RecCount) header.RecCount=n;
	ERRORMSG2(__FILE__", %lu:   updated RecCount from file, header.RecCount=%lu\n",(ULONG)__LINE__,header.RecCount);

  // inc RecCount
  header.RecCount++;
	ERRORMSG2(__FILE__", %lu:   RecCount++, header.RecCount=%lu\n",(ULONG)__LINE__,header.RecCount);

  // write new RecCount
  WrNoEndianLong(header.RecCount, buffer);
	#ifdef DEBUG
		ERRORMSG2(__FILE__", %lu:   %lu steht im Buffer\n",(ULONG)__LINE__,NoEndianLong(buffer));
	#endif
  if (lseek(handle,4UL,SEEK_SET)==-1) goto error;
  if (write(handle, buffer, 4)!=4) goto error;
	#ifdef DEBUG
  	if (lseek(handle,4UL,SEEK_SET)==-1) goto error;
  	if (read(handle, buffer, 4)!=4) goto error;
		ERRORMSG2(__FILE__", %lu:   %lu steht nach schreiben&lesen im Buffer\n",(ULONG)__LINE__,NoEndianLong(buffer));
	#endif

  // add new record (SEEK_SET is better than SEEK_END if file is corrupt)
  if (lseek(handle,getoff(header.RecCount),SEEK_SET)==-1L) goto error;
  if (write(handle, empty, RecSize()+1)!=RecSize()+1) goto error;

  // lock new record
  #ifdef __MSDOS__
    if (lock(handle, getoff(header.RecCount), RecSize())==-1) goto error;
  #else
    struct flock lck;
    lck.l_type   = F_WRLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = getoff(header.RecCount);
    lck.l_len    = RecSize();
    if (fcntl(handle, F_SETLK, &lck)) goto error;
  #endif

  // clear header lock
  hdr_unlock();

  delete[] empty;
	ERRORMSG2(__FILE__", %lu:   returning header.RecCount=%lu\n",(ULONG)__LINE__,header.RecCount);
  return header.RecCount;

 error:
  perror("dbHeader::add_locked_record");
  seterror();
  hdr_unlock();
  delete[] empty;
  return 0;
}

/***************************************************************************
 *                                                                         *
 * header.RecCount erneut aus der Datenbank lesen                          *
 *                                                                         *
 ***************************************************************************/
void dbHeader::update_reccount()
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.update_reccount: FATAL! No valid file handle\n");
    exit(1);
  }
  #endif
  
  // Auf Leseberechtigung warten
  #ifndef __MSDOS__
    struct flock lck;
    lck.l_type   = F_RDLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);
    if (fcntl(handle, F_SETLKW, &lck)) return;
  #endif
	
	// RecCount einlesen
	unsigned char buffer[5];	
  if (lseek(handle,4UL,SEEK_SET)==-1) return;
  if (read(handle, buffer, 4)!=4) return;
	ERRORMSG2(__FILE__", %lu, update_recount: header.RecCount=%lu\n",__LINE__,header.RecCount);
  #ifndef SECURE
  	header.RecCount=NoEndianLong(buffer);
	#else
  	ULONG n=NoEndianLong(buffer);
  	if (n<header.RecCount)
  	{
  		printf("dbHeader.update_reccount: FATAL! RecCount decreased\n");
  		exit(1);
  	}
  	header.RecCount=n;
	#endif
	ERRORMSG2(__FILE__", %lu, update_recount: header.RecCount=%lu\n",__LINE__,header.RecCount);

  // Lesesperre aufheben
  #ifndef __MSDOS__
    lck.l_type   = F_UNLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);
    fcntl(handle, F_SETLKW, &lck);
  #endif
}

/***************************************************************************
 *                                                                         *
 * Schreibsperre fuer den Header setzen und warten, wenn noetig            *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::hdr_lock()
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.hdr_lock: FATAL! No valid file handle\n");
    exit(1);
  }
  #endif
  #ifdef __MSDOS__
    // wait for write access to header
    while(lock(handle,0, sizeof(defHeader)))
    {
      sleep(1);
    }
    return true;
  #else
    struct flock lck;
    lck.l_type   = F_WRLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);
    if (fcntl(handle, F_SETLKW, &lck))
    {
      seterror("dbFileIO.lock: deadlock");
      return false;
    }
    return true;
  #endif
}

/***************************************************************************
 *                                                                         *
 * Schreibsperre für den Header entfernen                                  *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::hdr_unlock()
{
  #ifdef SECURE
  if (!handle)
  {
    printf("dbHeader.hdr_unlock: FATAL! No valid file handle\n");
    exit(1);
  }
  #endif
  #ifdef __MSDOS__
    unlock(handle,0, sizeof(defHeader));
    return true;
  #else
    struct flock lck;
    lck.l_type   = F_UNLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start  = 0;
    lck.l_len    = sizeof(defHeader);
    if (fcntl(handle, F_SETLK, &lck))
    {
      seterror();
      return false;
    }
    return true;
  #endif
}

/***************************************************************************
 *                                                                         *
 * Nummer zu einem Feldnamen bestimmen (0:nicht gefunden, 1,2,.. gefunden) *
 *                                                                         *
 ***************************************************************************/
WORD dbHeader::FieldNo(const char* name)
{
  if (handle)
  {
    for(WORD i=1; i<=nFldCount; i++)
    {
      if (stricmp(name,Field(i))==0)
        return i;
    }
  }
  #ifdef GERMAN
  seterror("Feldname nicht gefunden");
  #else
  seterror("Didn't found fieldname");
  #endif
  return 0;
}

/***************************************************************************
 *                                                                         *
 * Struktur der dBASE Datei ausgeben                                       *
 * (entspricht der dBase Anweisung 'LIST STRUCTURE'                        *
 *                                                                         *
 ***************************************************************************/
bool dbHeader::ListStructure()
{
	#ifdef GERMAN
  if(!handle)
  {
    seterror("Keine dB-Datei geöffnet");
    return false;
  }
  printf("Datensatzformat der dB-Datei    : %s\n",filename);
  printf("Anzahl der Datensätze           : %-7lu\n",RecCount());
  printf("Datum der letzten Aktualisierung: %02u.%02u.%02u\n",header.Day,
                                                              header.Month,
                                                              header.Year);
  printf("Feld    Feldname   Typ       Länge    Dez    Index\n");
  for (int i=0; i<FldCount(); i++)
  {
    printf("%5u   %-10s %-10s %5u    %3u        %c\n",
      i+1,
      fields[i].Name,
      TypeName(fields[i].Type),
      fields[i].Length,
      fields[i].Decimals,
      'N');
  };
  printf("*Gesamt*                      %5u\n",header.RecSize);
  #else
  if(!handle)
  {
    seterror("no db-file opened");
    return false;
  }
  printf("Format of db-file               : %s\n",filename);
  printf("Number of records               : %-7lu\n",RecCount());
  printf("Date of last write access       : %02u.%02u.%02u\n",header.Day,
                                                              header.Month,
                                                              header.Year);
  printf("Field   Fieldname  Typ       Lenght    Dec    Index\n");
  for (int i=0; i<FldCount(); i++)
  {
    printf("%5u   %-10s %-10s %5u    %3u        %c\n",
      i+1,
      fields[i].Name,
      TypeName(fields[i].Type),
      fields[i].Length,
      fields[i].Decimals,
      'N');
  };
  printf("*Total*                       %5u\n",header.RecSize);
	#endif
  return true;
}

/***************************************************************************
 *                                                                         *
 * Liefert den Namen zu einer Typkennung                                   *
 *                                                                         *
 ***************************************************************************/
const char* dbHeader::TypeName(char type)
{
	#ifdef GERMAN
  const char* name[]={"CZeichen",
        			        "NNumerisch",
              			  "FGleit",
              			  "DDatum",
                			"LLogisch",
               				"MMemo",
                			"UUndefiniert"};
  #else
  const char* name[]={"CChar",
        			        "NNumeric",
              			  "FFloat",
              			  "DDate",
                			"LLogical",
               				"MMemo",
                			"UUndefined"};
	#endif  
  char a=upcase(type);

  for (int i=0; i<7; i++)
  {
    if (a==*(name[i]))
      return name[i]+1;
  }
  return NULL;
}
