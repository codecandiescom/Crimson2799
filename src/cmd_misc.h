struct ColorPrefListType {
  BYTE *cName;
  BYTE cSymbol;
  BYTE cColumn;
};

extern COLORPREFLIST colorPrefList[];

extern CMDPROC(CmdTitle);
extern CMDPROC(CmdWho);
extern CMDPROC(CmdWhere);
extern CMDPROC(CmdSave);
extern void SetFlagCheck(THING *thing);
extern CMDPROC(CmdSet);
extern CMDPROC(CmdScore);
extern CMDPROC(CmdAlias);
extern CMDPROC(CmdUnAlias);
extern CMDPROC(CmdTime);
extern CMDPROC(CmdEnterMsg);
extern CMDPROC(CmdExitMsg);
extern CMDPROC(CmdPrompt);
extern CMDPROC(CmdFinger);
extern CMDPROC(CmdLevels);
extern CMDPROC(CmdSkillMax);
extern CMDPROC(CmdAffects);
extern CMDPROC(CmdStop);
extern CMDPROC(CmdSetColor);
extern CMDPROC(CmdQuit);


