#define HISTORY_SIZE 1024

struct HistoryType {
  BYTE  hBuf[HISTORY_SIZE];
  LWORD hStart;
  LWORD hEnd;
  LWORD hFirstCommand;
};

extern void  HistoryClear(HISTORY *history);
extern void  HistoryAdd(HISTORY *history, BYTE *cmd);
extern BYTE *HistoryGetLast(HISTORY *history);
extern void  HistoryList(HISTORY *history, SOCK *sock);
extern void  HistorySend(HISTORY *history, THING *thing);
extern LWORD HistoryParse(HISTORY *history, SOCK *sock, BYTE *cmd);
