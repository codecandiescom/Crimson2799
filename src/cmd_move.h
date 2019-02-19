extern BYTE MoveExitPrimitive(THING *thing, EXIT *exit);
extern BYTE MoveExitEnterStub(THING *thing, EXIT *exit);
extern BYTE MoveExit(THING *thing, EXIT *exit); /* moves a single person */
extern void MoveExitParse(THING *thing, EXIT *exit); /* moves whole group */
extern CMDPROC(CmdGo);
extern CMDPROC(CmdNorth);
extern CMDPROC(CmdEast);
extern CMDPROC(CmdSouth);
extern CMDPROC(CmdWest);
extern CMDPROC(CmdUp);
extern CMDPROC(CmdDown);
extern CMDPROC(CmdOut);
extern CMDPROC(CmdSneak);
extern CMDPROC(CmdLook);
extern CMDPROC(CmdScan);
extern CMDPROC(CmdExit);
extern CMDPROC(CmdOpen);
extern CMDPROC(CmdClose);
extern CMDPROC(CmdUnlock);
extern CMDPROC(CmdLock);
extern CMDPROC(CmdPicklock);
extern CMDPROC(CmdHack);
extern CMDPROC(CmdStand);
extern CMDPROC(CmdSit);
extern CMDPROC(CmdRest);
extern CMDPROC(CmdSleep);
extern CMDPROC(CmdWake);
extern CMDPROC(CmdTrack);
