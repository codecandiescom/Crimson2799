struct PsiListType {
  BYTE     *pName;
  WORD     *pSkill;
  WORD      pEffect;
  LWORD     pMaxCost;
  LWORD     pMinCost;
  FLAG      pTarget; /* this is gonna get the axe */
};

#define RESCUE_MOVE_COST      10
#define HIDE_MOVE_COST        3
#define PEEK_MOVE_COST        3
#define STEAL_MOVE_COST       10
#define PICKPOCKET_MOVE_COST  10

extern struct PsiListType psiList[];

extern CMDPROC(CmdHit);
extern CMDPROC(CmdGroup);
extern CMDPROC(CmdFollow);
extern CMDPROC(CmdConcentrate);
extern CMDPROC(CmdPractice);
extern void CbtConsider(THING *thing, THING *target);
extern CMDPROC(CmdConsider);
extern BYTE Flee(THING *thing);
extern CMDPROC(CmdFlee);
extern BYTE CbtRescue(THING *thing, THING *target);
extern CMDPROC(CmdRescue);
extern CMDPROC(CmdHide);
extern CMDPROC(CmdPeek);
extern CMDPROC(CmdSteal);
extern CMDPROC(CmdPickpocket);
