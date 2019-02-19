/* more or less standard windows-type ini file, sans groups:
setting=value
  setting=value ; your comment here
; commented line
# commented line
setting one=blah ; invalid will be interpreted as setting=one

As with windows an INI file > 64K will cause a crash....
white space is mostly ignored
# and ; are both reserved for comments
dont use lines > 256 chars in length, as weird stuff occurs...
*/

/* result should be >= 256 char buffer */
extern void  INIRead(BYTE *fileName, BYTE *setting, BYTE *defResult, BYTE *result);
extern LWORD INILWordRead(BYTE *fileName, BYTE *setting, LWORD defResult);
extern void  INIWrite(BYTE *fileName, BYTE *setting, BYTE *result);
