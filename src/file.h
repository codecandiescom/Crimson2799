/* file.h */
/* due to the inclusion of time_t as part of FILELIST 
   you will have to #include <time.h> prior to this
   header files inclusion */


typedef struct FileListType {
  BYTE   *fileName;
  STR    *fileStr;
  time_t  fileDate;
} FILELIST;

/* files in the filelist */
#define FILE_GREETING 0
#define FILE_CIAO     1
#define FILE_NEWS     2
#define FILE_MOTD     3
#define FILE_QUIT     4
#define FILE_CREDIT   5
#define FILE_DEATH    6
#define FILE_BANNED   7
#define FILE_BANNEW   8
#define FILE_OFFSITE  9

extern BYTE fileVerbose;
extern BYTE fileError;
extern FILELIST fileList[];

extern void  FileInit(void);
extern void  FileRead(FILELIST *fileList);
extern STR  *FileStrRead(FILE *file);
extern LWORD FileFlagRead(FILE *file, BYTE *fList[]);
extern LWORD FileTypeRead(FILE *file, ULWORD tList, LWORD tSize);
extern BYTE  FileByteRead(FILE *file);
#define      FILETYPEREAD(file,tList) FileTypeRead(file,(ULWORD)tList,sizeof(*tList))
#define      FILESTRREAD(file, str) do { STRFREE(str); str = FileStrRead(file); } while(0)

extern BYTE FileStrWrite(FILE *file, STR *str);
extern BYTE FileBinaryWrite(FILE *file, STR *str);
extern void FileTypeWrite(FILE *file, LWORD type, ULWORD tList, LWORD tSize, BYTE next);
extern void FileFlagWrite(FILE *file, FLAG flag, BYTE *fList[], BYTE next);
extern void FileByteWrite(FILE *file, BYTE value, BYTE separator);
#define     FILETYPEWRITE(file,type,tList,byte) FileTypeWrite(file,type,(ULWORD)tList,sizeof(*tList),byte)
