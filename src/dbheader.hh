/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994 Mark-Andre' Hopf                                       *
 *  filename: dbheader.h                                                    *
 *  relation: dbheader.cpp                                                  *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 24.04.1995                                                    *
 *                                                                          *
 ****************************************************************************/

#ifndef __DBHEADER_HH
#define __DBHEADER_HH

/****************************************************************************
 * dbHeader                                                                 *
 ****************************************************************************/
class dbHeader : virtual public dbError
{
  private:
    // Strukturen fuer die im Dateiheader von dBASE abgelegten Daten
    struct defHeader
    {
      BYTE  Version;            // (0) dBASE-Versionskennzeichen
      BYTE  Year;               // (1) Datum der letzten Änderung
      BYTE  Month;
      BYTE  Day;
      ULONG RecCount;           // (4) Anzahl der Datensätze
      WORD  HeaderSize;         // Länge des gesamten Dateivorspanns
      WORD  RecSize;            // Länge eines Datensatzes
      BYTE  reserved1[2];
      BYTE  Transaction;        // Transaktion abgeschlossen (1=nein)
      BYTE  Crypted;            // Datei verschlüsselt? (1=ja)
      BYTE  reserved2[12];
      BYTE  MDXexist;           // MDX-Datei angelegt? (1=ja)
      BYTE  reserved3[3];
    } header;

    struct defField
    {
      char  Name[11];           // nullterminierter String mit Feldname
      char  Type;               // Feldtype (C,N,F,D,L,M)
      BYTE  reserved1[4];
      BYTE  Length;             // Feldlänge
      BYTE  Decimals;           // Anzahl der Dezimalstellen
      BYTE  reserved2[14];
    } *fields;

    // aus den Headerdaten errechenbare Daten:
    WORD nFldCount;             // Anzahl Felder pro Record
    ULONG ulFieldOffset;        // Dateioffset des ersten Feldes
    ptrdiff_t *fieldindex;      // Tabelle mit Offset der Felder im Datensatz
                                // (Abzufragen mit FieldOfs(WORD))

    // zur Verwaltung des Headers nötige Daten & Funktionen
    void set_filename(const char *newname);
    char *filename;       			// Name der geöffneten Datei

    bool loadheader();          // Header aus geöffneter Datei lesen

    bool hdr_read(int fd, void *buf, unsigned len);
    // bool hdr_write(int fd, void *buf, unsigned len);
		void update_reccount();			// RecCount neu einlesen

    bool hdr_lock();            // wait for and set write lock
    bool hdr_unlock();          // clear write lock

		// für create_begin, create_add & create_end:
		int fdCreate;								// Dateihande, wenn ein neues Dateiformat definiert
																// wird
		WORD nCreateFldAdded;				// Anzahl der bereits hinzugefügten Felder beim
																// definieren eines Headers
  protected:
    dbHeader()
    {
      handle=fdCreate=0;
      filename=NULL;
      fields=NULL;
      fieldindex=NULL;
    }
    #ifdef SECURE
    ~dbHeader();
    #endif

    int handle;                 // Handle bzw. File Deskriptor der Datei, wenn
                                // die Datenbank geöffnet ist und benutzt
                                // werden kann, sonst NULL

    bool fileopen(const char *file);  // Datei öffnen
    bool fileclose();           // Datei schliessen

    bool create_begin(const char *file, WORD nFldCount);
    bool create_add(const char* name,char type,BYTE len, BYTE dec);
    bool create_end();
protected:
    bool Is_it_EOF(ULONG);      // EOF Variante, die unnoetiges Laden des
                                // Headers ueberfluessig macht
    ULONG getoff(ULONG record); // Dateioffset eines Datensatzes berechnen
    ptrdiff_t FieldOfs(WORD);   // Liefert Datensatzoffset zu Feldnummer

    ULONG add_locked_record();  // Fügt einen neuen leeren Datensatz an die
                                // db-Datei an und sperrt ihn, liefert die
                                // Nummer des neuen Records bei Erfolg, sonst
                                // 0. Fehler bei der Sperrung werden nicht
                                // mitgeteilt
  public:
    const char* Dbf();          // Liefert den Namen der dBASE Datei
    WORD FldCount();            // Liefert Anzahl der Felder pro Datensatz
    ULONG RecCount();           // Liefert Anzahl der Datensätze in der Datei
    WORD RecSize();             // Liefert Datensatzgroesse in Byte
    const char* Field(WORD nr); // Liefert Feldname zu Nr. (nr=1,..,FldCount)
    WORD FieldNo(const char* name); // Liefert Nr. zu Feldname
    BYTE FieldLen(WORD nr);     // Liefert Laenge zu Feldnummer
    BYTE FieldDec(WORD nr);     // Liefert Dezimalstellen zu Feldnummer
    char FieldType(WORD nr);    // Liefert Typ zu Feldnummer
    const char* TypeName(char type);  // liefert Zeiger auf Typname ('C'->"Zeichen")
    bool ListStructure();       // Druckt Dateistruktur aus (LIST STRUCTURE)
};

#ifdef SECURE
inline dbHeader::~dbHeader()
{
	if (handle)
	{
		printf("dbHeader.~dbHeader: FATAL! file was not closed\n");
		exit(1);
	}
}
#endif

inline const char* dbHeader::Dbf(){return filename?(const char*)filename:"";}
inline WORD  dbHeader::FldCount(){return handle?nFldCount:0;}
inline WORD  dbHeader::RecSize() {return handle?header.RecSize:0;}
inline ULONG dbHeader::RecCount()
{
	if (!handle) return 0;
	update_reccount();
	return header.RecCount;
}

inline ULONG dbHeader::getoff(ULONG nr)
{
  if (nr)
    return ulFieldOffset+((--nr)*RecSize());
  else
    return ulFieldOffset;       // Datensatz 0 als 1 behandeln (BOF)
}

// Diese Funktion soll die Eof() Funktion beschleunigen, ausgehend davon,
// dass eine geöffnete dbDatei im Mehrbenutzerbetrieb nur größer wird.
// ACHTUNG: Variablen MÜSSEN UNSIGNED SEIN !!!
inline bool dbHeader::Is_it_EOF(ULONG nr)
{
  if (nr<=header.RecCount)    // Zu pruefende Recordnummer kleiner als die..
    return false;             // ..letzt bekannte Anzahl Records, dann false
  update_reccount();
  return (nr<=header.RecCount)?false:true;
}

inline const char* dbHeader::Field(WORD nr)
{
  if (!handle || (nr<1 || nr>nFldCount)) return NULL;
  return fields[--nr].Name;
}

inline BYTE dbHeader::FieldLen(WORD nr)
{
  if (!handle || (nr<1 || nr>nFldCount)) return 0;
  return fields[--nr].Length;
}

inline BYTE dbHeader::FieldDec(WORD nr)
{
  if (!handle || (nr<1 || nr>nFldCount)) return 0;
  return fields[--nr].Decimals;
}

inline char dbHeader::FieldType(WORD nr)
{
  if (!handle || (nr<1 || nr>nFldCount)) return '\0';
  return fields[--nr].Type;
}

inline ptrdiff_t dbHeader::FieldOfs(WORD nr)
{
  if (!handle || (nr<1 || nr>nFldCount)) return '\0';
    return fieldindex[--nr];
}
#endif
