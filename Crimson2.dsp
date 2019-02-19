# Microsoft Developer Studio Project File - Name="Crimson2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Crimson2 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Crimson2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Crimson2.mak" CFG="Crimson2 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Crimson2 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Crimson2 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Crimson2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "Crimson2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386

!ENDIF 

# Begin Target

# Name "Crimson2 - Win32 Release"
# Name "Crimson2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\src\affect.c
# End Source File
# Begin Source File

SOURCE=.\src\alias.c
# End Source File
# Begin Source File

SOURCE=.\src\area.c
# End Source File
# Begin Source File

SOURCE=.\src\base.c
# End Source File
# Begin Source File

SOURCE=.\src\board.c
# End Source File
# Begin Source File

SOURCE=.\src\char.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_area.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_brd.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_cbt.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_code.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_god.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_help.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_inv.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_misc.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_mob.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_mole.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_move.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_obj.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_rst.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_talk.c
# End Source File
# Begin Source File

SOURCE=.\src\cmd_wld.c
# End Source File
# Begin Source File

SOURCE=.\src\code.c
# End Source File
# Begin Source File

SOURCE=.\src\codestuf.c
# End Source File
# Begin Source File

SOURCE=.\src\compile.c
# End Source File
# Begin Source File

SOURCE=.\src\crimson2.c
# End Source File
# Begin Source File

SOURCE=.\src\decomp.c
# End Source File
# Begin Source File

SOURCE=.\src\edit.c
# End Source File
# Begin Source File

SOURCE=.\src\effect.c
# End Source File
# Begin Source File

SOURCE=.\src\exit.c
# End Source File
# Begin Source File

SOURCE=.\src\extra.c
# End Source File
# Begin Source File

SOURCE=.\src\fight.c
# End Source File
# Begin Source File

SOURCE=.\src\file.c
# End Source File
# Begin Source File

SOURCE=.\src\function.c
# End Source File
# Begin Source File

SOURCE=.\src\help.c
# End Source File
# Begin Source File

SOURCE=.\src\history.c
# End Source File
# Begin Source File

SOURCE=.\src\index.c
# End Source File
# Begin Source File

SOURCE=.\src\ini.c
# End Source File
# Begin Source File

SOURCE=.\src\interp.c
# End Source File
# Begin Source File

SOURCE=.\src\log.c
# End Source File
# Begin Source File

SOURCE=.\src\mem.c
# End Source File
# Begin Source File

SOURCE=.\src\mobile.c
# End Source File
# Begin Source File

SOURCE=.\src\mole.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_are.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_mob.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_msc.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_obj.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_rst.c
# End Source File
# Begin Source File

SOURCE=.\src\mole_wld.c
# End Source File
# Begin Source File

SOURCE=.\src\object.c
# End Source File
# Begin Source File

SOURCE=.\src\parse.c
# End Source File
# Begin Source File

SOURCE=.\src\player.c
# End Source File
# Begin Source File

SOURCE=.\src\property.c
# End Source File
# Begin Source File

SOURCE=.\src\queue.c
# End Source File
# Begin Source File

SOURCE=.\src\reset.c
# End Source File
# Begin Source File

SOURCE=.\src\send.c
# End Source File
# Begin Source File

SOURCE=.\src\site.c
# End Source File
# Begin Source File

SOURCE=.\src\skill.c
# End Source File
# Begin Source File

SOURCE=.\src\social.c
# End Source File
# Begin Source File

SOURCE=.\src\socket.c
# End Source File
# Begin Source File

SOURCE=.\src\str.c
# End Source File
# Begin Source File

SOURCE=.\src\thing.c
# End Source File
# Begin Source File

SOURCE=.\src\timing.c
# End Source File
# Begin Source File

SOURCE=.\src\WinsockExt.c
# End Source File
# Begin Source File

SOURCE=.\src\world.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\src\affect.h
# End Source File
# Begin Source File

SOURCE=.\src\alias.h
# End Source File
# Begin Source File

SOURCE=.\src\area.h
# End Source File
# Begin Source File

SOURCE=.\src\base.h
# End Source File
# Begin Source File

SOURCE=.\src\board.h
# End Source File
# Begin Source File

SOURCE=.\src\char.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_area.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_brd.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_cbt.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_code.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_god.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_help.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_inv.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_misc.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_mob.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_mole.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_move.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_obj.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_rst.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_talk.h
# End Source File
# Begin Source File

SOURCE=.\src\cmd_wld.h
# End Source File
# Begin Source File

SOURCE=.\src\code.h
# End Source File
# Begin Source File

SOURCE=.\src\codestuf.h
# End Source File
# Begin Source File

SOURCE=.\src\compile.h
# End Source File
# Begin Source File

SOURCE=.\src\crimson2.h
# End Source File
# Begin Source File

SOURCE=.\src\decomp.h
# End Source File
# Begin Source File

SOURCE=.\src\edit.h
# End Source File
# Begin Source File

SOURCE=.\src\effect.h
# End Source File
# Begin Source File

SOURCE=.\src\exit.h
# End Source File
# Begin Source File

SOURCE=.\src\extra.h
# End Source File
# Begin Source File

SOURCE=.\src\fight.h
# End Source File
# Begin Source File

SOURCE=.\src\file.h
# End Source File
# Begin Source File

SOURCE=.\src\function.h
# End Source File
# Begin Source File

SOURCE=.\src\help.h
# End Source File
# Begin Source File

SOURCE=.\src\history.h
# End Source File
# Begin Source File

SOURCE=.\src\index.h
# End Source File
# Begin Source File

SOURCE=.\src\ini.h
# End Source File
# Begin Source File

SOURCE=.\src\interp.h
# End Source File
# Begin Source File

SOURCE=.\src\log.h
# End Source File
# Begin Source File

SOURCE=.\src\mem.h
# End Source File
# Begin Source File

SOURCE=.\src\mobile.h
# End Source File
# Begin Source File

SOURCE=.\src\mole.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_are.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_mob.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_msc.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_obj.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_rst.h
# End Source File
# Begin Source File

SOURCE=.\src\mole_wld.h
# End Source File
# Begin Source File

SOURCE=.\src\object.h
# End Source File
# Begin Source File

SOURCE=.\src\parse.h
# End Source File
# Begin Source File

SOURCE=.\src\player.h
# End Source File
# Begin Source File

SOURCE=.\src\property.h
# End Source File
# Begin Source File

SOURCE=.\src\queue.h
# End Source File
# Begin Source File

SOURCE=.\src\reset.h
# End Source File
# Begin Source File

SOURCE=.\src\send.h
# End Source File
# Begin Source File

SOURCE=.\src\site.h
# End Source File
# Begin Source File

SOURCE=.\src\skill.h
# End Source File
# Begin Source File

SOURCE=.\src\social.h
# End Source File
# Begin Source File

SOURCE=.\src\socket.h
# End Source File
# Begin Source File

SOURCE=.\src\str.h
# End Source File
# Begin Source File

SOURCE=.\src\thing.h
# End Source File
# Begin Source File

SOURCE=.\src\timing.h
# End Source File
# Begin Source File

SOURCE=.\src\winsockext.h
# End Source File
# Begin Source File

SOURCE=.\src\world.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
