// pack.c
// two-char identifier: pa

void paPackCleanup(HGLOBAL p_Memory);
HGLOBAL paPackWorld(DSSTRUCT *p_thing,long *p_dataLen,char **p_data);
HGLOBAL paPackMobile(DSSTRUCT *p_thing,long *p_dataLen,char **p_data);
HGLOBAL paPackObject(DSSTRUCT *p_thing,long *p_dataLen,char **p_data);
HGLOBAL paPackAreaDetail(DSSTRUCT *p_thing,long *p_dataLen,char **p_data);
HGLOBAL paPackReset(DSSTRUCT *p_thing,long *p_dataLen,char **p_data);
int paEncodeString(char *p_dst,char *p_src);

