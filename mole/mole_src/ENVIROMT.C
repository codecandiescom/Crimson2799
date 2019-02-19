// enviromt.c
// Two-letter descriptor: en
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

// This module just provides some tools to save/load the
// environment. Winfuck calls these sorts of things "profile" settings.

#include<windows.h>
#include<stdio.h>

/* Globals used exclusively in this file */
char *g_enInitFile;
WINDOWPLACEMENT g_enPlacement;

BOOL enInitEnvironment(char *p_inifile) {
  g_enInitFile=p_inifile;
  return TRUE;
}

void enShutdownEnvironment() {
  return;
}

WINDOWPLACEMENT *enRestoreWindowPlacement(char *p_section,int p_defLeft,
  int p_defTop,int p_defRight,int p_defBottom) {
  if (!p_section)
    return NULL;

  g_enPlacement.length=sizeof(WINDOWPLACEMENT);
  g_enPlacement.flags=(UINT)GetPrivateProfileInt(p_section,"flags",
    0,g_enInitFile);
  g_enPlacement.showCmd=(UINT)GetPrivateProfileInt(p_section,"showCmd",
    SW_SHOWNA,g_enInitFile);
  g_enPlacement.ptMinPosition.x=(int)GetPrivateProfileInt(p_section,"ptMinX",
    100,g_enInitFile);
  g_enPlacement.ptMinPosition.y=(int)GetPrivateProfileInt(p_section,"ptMinY",
    100,g_enInitFile);
  g_enPlacement.ptMaxPosition.y=(int)GetPrivateProfileInt(p_section,"ptMaxY",
    0,g_enInitFile);
  g_enPlacement.ptMaxPosition.y=(int)GetPrivateProfileInt(p_section,"ptMaxY",
    0,g_enInitFile);
  g_enPlacement.rcNormalPosition.left=(int)GetPrivateProfileInt(p_section,"rcNormalLeft",
    p_defLeft,g_enInitFile);
  g_enPlacement.rcNormalPosition.top=(int)GetPrivateProfileInt(p_section,"rcNormalTop",
    p_defTop,g_enInitFile);
  g_enPlacement.rcNormalPosition.right=(int)GetPrivateProfileInt(p_section,"rcNormalRight",
    p_defRight,g_enInitFile);
  g_enPlacement.rcNormalPosition.bottom=(int)GetPrivateProfileInt(p_section,"rcNormalBottom",
    p_defBottom,g_enInitFile);
  return &g_enPlacement;
}

void enSaveWindowPlacement(char *p_section,HWND p_hWnd) {
  char l_buf[30];

  if ((!p_section)||(!p_hWnd))
    return;

  g_enPlacement.length=sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(p_hWnd,&g_enPlacement);

  sprintf(l_buf,"%i",g_enPlacement.length);
  WritePrivateProfileString(p_section,"length",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.flags);
  WritePrivateProfileString(p_section,"flags",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.showCmd);
  WritePrivateProfileString(p_section,"showCmd",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.ptMinPosition.x);
  WritePrivateProfileString(p_section,"ptMinX",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.ptMinPosition.y);
  WritePrivateProfileString(p_section,"ptMinY",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.ptMaxPosition.x);
  WritePrivateProfileString(p_section,"ptMaxX",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.ptMaxPosition.y);
  WritePrivateProfileString(p_section,"ptMaxY",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.rcNormalPosition.left);
  WritePrivateProfileString(p_section,"rcNormalLeft",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.rcNormalPosition.top);
  WritePrivateProfileString(p_section,"rcNormalTop",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.rcNormalPosition.right);
  WritePrivateProfileString(p_section,"rcNormalRight",l_buf,g_enInitFile);
  sprintf(l_buf,"%i",g_enPlacement.rcNormalPosition.bottom);
  WritePrivateProfileString(p_section,"rcNormalBottom",l_buf,g_enInitFile);
  return;
}

void enSaveLong(char *p_Section, char *p_Entry, unsigned long p_Long) {
  char l_buf[20];
  sprintf(l_buf,"%lu",p_Long);
  WritePrivateProfileString(p_Section,p_Entry,l_buf,g_enInitFile);
  return;
}

unsigned long enRestoreLong(char *p_Section, char *p_Entry, unsigned long p_Default) {
  char l_buf[20],l_buf2[20];
  unsigned long l_Long;
  sprintf(l_buf2,"%lu",p_Default);
  GetPrivateProfileString(p_Section,p_Entry,l_buf2,l_buf,20,g_enInitFile);
  sscanf(l_buf,"%lu",&l_Long);
  return l_Long;
}

void enSaveInt(char *p_Section, char *p_Entry, unsigned int p_Int) {
  char l_buf[20];
  sprintf(l_buf,"%u",p_Int);
  WritePrivateProfileString(p_Section,p_Entry,l_buf,g_enInitFile);
  return;
}

unsigned int enRestoreInt(char *p_Section, char *p_Entry, unsigned int p_Default) {
  char l_buf[20],l_buf2[20];
  unsigned int l_Int;
  sprintf(l_buf2,"%u",p_Default);
  GetPrivateProfileString(p_Section,p_Entry,l_buf2,l_buf,20,g_enInitFile);
  sscanf(l_buf,"%u",&l_Int);
  return l_Int;
}

void enSaveString(char *p_Section, char *p_Entry, unsigned char *p_Str) {
  WritePrivateProfileString(p_Section,p_Entry,(char*)p_Str,g_enInitFile);
}

void enRestoreString(char *p_Section, char *p_Entry, char *p_Default,
  unsigned char *p_Str, unsigned long p_StrLen) {
  GetPrivateProfileString(p_Section,p_Entry,p_Default,(char*)p_Str,
    p_StrLen,g_enInitFile);
}

void enSaveYesNo(char *p_Section, char *p_Entry, BOOL p_yesno) {
  if (p_yesno)
    WritePrivateProfileString(p_Section,p_Entry,"Yes",g_enInitFile);
  else
    WritePrivateProfileString(p_Section,p_Entry,"No",g_enInitFile);
}

BOOL enRestoreYesNo(char *p_Section, char *p_Entry, BOOL p_Default) {
  char l_buf[20],*l_default;

  if (p_Default)
    l_default="Yes";
  else
    l_default="No";
  GetPrivateProfileString(p_Section,p_Entry,l_default,l_buf,20,g_enInitFile);
  switch(l_buf[0]) {
    case 'y':
    case 'Y':
    case '1':
      return TRUE;
    case 'n':
    case 'N':
    case '0':
    default:
      return FALSE;
  }
}

void enSaveLogFont(char *p_Section, LOGFONT *p_LogFont) {
#ifdef WIN32
  enSaveLong(p_Section,"LF_lfHeight",(unsigned long)p_LogFont->lfHeight);
  enSaveLong(p_Section,"LF_lfWidth",(unsigned long)p_LogFont->lfWidth);
  enSaveLong(p_Section,"LF_lfEscapement",(unsigned long)p_LogFont->lfEscapement);
  enSaveLong(p_Section,"LF_lfOrientation",(unsigned long)p_LogFont->lfOrientation);
  enSaveLong(p_Section,"LF_lfWeight",(unsigned long)p_LogFont->lfWeight);
#else
  enSaveInt(p_Section,"LF_lfHeight",(unsigned int)p_LogFont->lfHeight);
  enSaveInt(p_Section,"LF_lfWidth",(unsigned int)p_LogFont->lfWidth);
  enSaveInt(p_Section,"LF_lfEscapement",(unsigned int)p_LogFont->lfEscapement);
  enSaveInt(p_Section,"LF_lfOrientation",(unsigned int)p_LogFont->lfOrientation);
  enSaveInt(p_Section,"LF_lfWeight",(unsigned int)p_LogFont->lfWeight);
#endif
  enSaveInt(p_Section,"LF_lfItalic",(unsigned int)p_LogFont->lfItalic);
  enSaveInt(p_Section,"LF_lfUnderline",(unsigned int)p_LogFont->lfUnderline);
  enSaveInt(p_Section,"LF_lfStrikeOut",(unsigned int)p_LogFont->lfStrikeOut);
  enSaveInt(p_Section,"LF_lfCharSet",(unsigned int)p_LogFont->lfCharSet);
  enSaveInt(p_Section,"LF_lfOutPrecision",(unsigned int)p_LogFont->lfOutPrecision);
  enSaveInt(p_Section,"LF_lfClipPrecision",(unsigned int)p_LogFont->lfClipPrecision);
  enSaveInt(p_Section,"LF_lfQuality",(unsigned int)p_LogFont->lfQuality);
  enSaveInt(p_Section,"LF_lfPitchAndFamily",(unsigned int)p_LogFont->lfPitchAndFamily);
  enSaveString(p_Section,"LF_lfFaceName",(unsigned char*)p_LogFont->lfFaceName);
}

void enRestoreLogFont(char *p_Section, LOGFONT *p_LogFont) {
#ifdef WIN32
  p_LogFont->lfHeight=(long)enRestoreLong(p_Section,"LF_lfHeight",0L);
  p_LogFont->lfWidth=(long)enRestoreLong(p_Section,"LF_lfWidth",0L);
  p_LogFont->lfEscapement=(long)enRestoreLong(p_Section,"LF_lfEscapement",0L);
  p_LogFont->lfOrientation=(long)enRestoreLong(p_Section,"LF_lfOrientation",0L);
  p_LogFont->lfWeight=(long)enRestoreLong(p_Section,"LF_lfWeight",0L);
#else
  p_LogFont->lfHeight=(int)enRestoreInt(p_Section,"LF_lfHeight",0);
  p_LogFont->lfWidth=(int)enRestoreInt(p_Section,"LF_lfWidth",0);
  p_LogFont->lfEscapement=(int)enRestoreInt(p_Section,"LF_lfEscapement",0);
  p_LogFont->lfOrientation=(int)enRestoreInt(p_Section,"LF_lfOrientation",0);
  p_LogFont->lfWeight=(int)enRestoreInt(p_Section,"LF_lfWeight",0);
#endif
  p_LogFont->lfItalic=(BYTE)enRestoreInt(p_Section,"LF_lfItalic",0);
  p_LogFont->lfUnderline=(BYTE)enRestoreInt(p_Section,"LF_lfUnderline",0);
  p_LogFont->lfStrikeOut=(BYTE)enRestoreInt(p_Section,"LF_lfStrikeOut",0);
  p_LogFont->lfCharSet=(BYTE)enRestoreInt(p_Section,"LF_lfCharSet",0);
  p_LogFont->lfOutPrecision=(BYTE)enRestoreInt(p_Section,"LF_lfOutPrecision",0);
  p_LogFont->lfClipPrecision=(BYTE)enRestoreInt(p_Section,"LF_lfClipPrecision",0);
  p_LogFont->lfQuality=(BYTE)enRestoreInt(p_Section,"LF_lfQuality",0);
  p_LogFont->lfPitchAndFamily=(BYTE)enRestoreInt(p_Section,"LF_lfPitchAndFamily",0);
  enRestoreString(p_Section,"LF_lfFaceName","",
    (unsigned char*)p_LogFont->lfFaceName,LF_FACESIZE);
}
