struct CmdMOLEListType {
  ULWORD       mCmd;
  CMDMOLEPROC(*mProc);
  BYTE         mPos;
  BYTE         mLevel;
  BYTE         mLog;
  FLAG         mFlag;
};

extern const CMDMOLELIST cmdMOLEList[];

extern CMDPROC(CmdMOLE);
extern BYTE CmdMOLEPositionCheck(LWORD i,THING *thing);
extern BYTE CmdMOLEFlagCheck(LWORD i,SOCK *sock,LWORD virtual);
extern BYTE *CmdMOLELog(LWORD i,LWORD virtual);
