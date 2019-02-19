# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Crimson2 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Crimson2 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Crimson2 - Win32 Release" && "$(CFG)" !=\
 "Crimson2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Crimson2.mak" CFG="Crimson2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Crimson2 - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "Crimson2 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "Crimson2 - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Crimson2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "./Debug"
# PROP Target_Dir ""
OUTDIR=.\.
INTDIR=.\./Debug

ALL : "$(OUTDIR)\Crimson2.exe"

CLEAN : 
	-@erase "$(INTDIR)\affect.obj"
	-@erase "$(INTDIR)\alias.obj"
	-@erase "$(INTDIR)\area.obj"
	-@erase "$(INTDIR)\base.obj"
	-@erase "$(INTDIR)\board.obj"
	-@erase "$(INTDIR)\char.obj"
	-@erase "$(INTDIR)\cmd_area.obj"
	-@erase "$(INTDIR)\cmd_brd.obj"
	-@erase "$(INTDIR)\cmd_cbt.obj"
	-@erase "$(INTDIR)\cmd_code.obj"
	-@erase "$(INTDIR)\cmd_god.obj"
	-@erase "$(INTDIR)\cmd_help.obj"
	-@erase "$(INTDIR)\cmd_inv.obj"
	-@erase "$(INTDIR)\cmd_misc.obj"
	-@erase "$(INTDIR)\cmd_mob.obj"
	-@erase "$(INTDIR)\cmd_mole.obj"
	-@erase "$(INTDIR)\cmd_move.obj"
	-@erase "$(INTDIR)\cmd_obj.obj"
	-@erase "$(INTDIR)\cmd_rst.obj"
	-@erase "$(INTDIR)\cmd_talk.obj"
	-@erase "$(INTDIR)\cmd_wld.obj"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\codestuf.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\crimson2.obj"
	-@erase "$(INTDIR)\decomp.obj"
	-@erase "$(INTDIR)\edit.obj"
	-@erase "$(INTDIR)\effect.obj"
	-@erase "$(INTDIR)\exit.obj"
	-@erase "$(INTDIR)\extra.obj"
	-@erase "$(INTDIR)\fight.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\function.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\index.obj"
	-@erase "$(INTDIR)\ini.obj"
	-@erase "$(INTDIR)\interp.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mobile.obj"
	-@erase "$(INTDIR)\mole.obj"
	-@erase "$(INTDIR)\mole_are.obj"
	-@erase "$(INTDIR)\mole_mob.obj"
	-@erase "$(INTDIR)\mole_msc.obj"
	-@erase "$(INTDIR)\mole_obj.obj"
	-@erase "$(INTDIR)\mole_rst.obj"
	-@erase "$(INTDIR)\mole_wld.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\player.obj"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\reset.obj"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\site.obj"
	-@erase "$(INTDIR)\skill.obj"
	-@erase "$(INTDIR)\social.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\str.obj"
	-@erase "$(INTDIR)\thing.obj"
	-@erase "$(INTDIR)\timing.obj"
	-@erase "$(INTDIR)\WinsockExt.obj"
	-@erase "$(INTDIR)\world.obj"
	-@erase "$(OUTDIR)\Crimson2.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/Crimson2.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\./Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Crimson2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/Crimson2.pdb" /machine:I386 /out:"$(OUTDIR)/Crimson2.exe" 
LINK32_OBJS= \
	"$(INTDIR)\affect.obj" \
	"$(INTDIR)\alias.obj" \
	"$(INTDIR)\area.obj" \
	"$(INTDIR)\base.obj" \
	"$(INTDIR)\board.obj" \
	"$(INTDIR)\char.obj" \
	"$(INTDIR)\cmd_area.obj" \
	"$(INTDIR)\cmd_brd.obj" \
	"$(INTDIR)\cmd_cbt.obj" \
	"$(INTDIR)\cmd_code.obj" \
	"$(INTDIR)\cmd_god.obj" \
	"$(INTDIR)\cmd_help.obj" \
	"$(INTDIR)\cmd_inv.obj" \
	"$(INTDIR)\cmd_misc.obj" \
	"$(INTDIR)\cmd_mob.obj" \
	"$(INTDIR)\cmd_mole.obj" \
	"$(INTDIR)\cmd_move.obj" \
	"$(INTDIR)\cmd_obj.obj" \
	"$(INTDIR)\cmd_rst.obj" \
	"$(INTDIR)\cmd_talk.obj" \
	"$(INTDIR)\cmd_wld.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\codestuf.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\crimson2.obj" \
	"$(INTDIR)\decomp.obj" \
	"$(INTDIR)\edit.obj" \
	"$(INTDIR)\effect.obj" \
	"$(INTDIR)\exit.obj" \
	"$(INTDIR)\extra.obj" \
	"$(INTDIR)\fight.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\function.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\index.obj" \
	"$(INTDIR)\ini.obj" \
	"$(INTDIR)\interp.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mobile.obj" \
	"$(INTDIR)\mole.obj" \
	"$(INTDIR)\mole_are.obj" \
	"$(INTDIR)\mole_mob.obj" \
	"$(INTDIR)\mole_msc.obj" \
	"$(INTDIR)\mole_obj.obj" \
	"$(INTDIR)\mole_rst.obj" \
	"$(INTDIR)\mole_wld.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\player.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\reset.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\site.obj" \
	"$(INTDIR)\skill.obj" \
	"$(INTDIR)\social.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\str.obj" \
	"$(INTDIR)\thing.obj" \
	"$(INTDIR)\timing.obj" \
	"$(INTDIR)\WinsockExt.obj" \
	"$(INTDIR)\world.obj"

"$(OUTDIR)\Crimson2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Crimson2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "./Debug"
# PROP Target_Dir ""
OUTDIR=.\.
INTDIR=.\./Debug

ALL : "$(OUTDIR)\Crimson2.exe"

CLEAN : 
	-@erase "$(INTDIR)\affect.obj"
	-@erase "$(INTDIR)\alias.obj"
	-@erase "$(INTDIR)\area.obj"
	-@erase "$(INTDIR)\base.obj"
	-@erase "$(INTDIR)\board.obj"
	-@erase "$(INTDIR)\char.obj"
	-@erase "$(INTDIR)\cmd_area.obj"
	-@erase "$(INTDIR)\cmd_brd.obj"
	-@erase "$(INTDIR)\cmd_cbt.obj"
	-@erase "$(INTDIR)\cmd_code.obj"
	-@erase "$(INTDIR)\cmd_god.obj"
	-@erase "$(INTDIR)\cmd_help.obj"
	-@erase "$(INTDIR)\cmd_inv.obj"
	-@erase "$(INTDIR)\cmd_misc.obj"
	-@erase "$(INTDIR)\cmd_mob.obj"
	-@erase "$(INTDIR)\cmd_mole.obj"
	-@erase "$(INTDIR)\cmd_move.obj"
	-@erase "$(INTDIR)\cmd_obj.obj"
	-@erase "$(INTDIR)\cmd_rst.obj"
	-@erase "$(INTDIR)\cmd_talk.obj"
	-@erase "$(INTDIR)\cmd_wld.obj"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\codestuf.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\crimson2.obj"
	-@erase "$(INTDIR)\decomp.obj"
	-@erase "$(INTDIR)\edit.obj"
	-@erase "$(INTDIR)\effect.obj"
	-@erase "$(INTDIR)\exit.obj"
	-@erase "$(INTDIR)\extra.obj"
	-@erase "$(INTDIR)\fight.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\function.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\index.obj"
	-@erase "$(INTDIR)\ini.obj"
	-@erase "$(INTDIR)\interp.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mobile.obj"
	-@erase "$(INTDIR)\mole.obj"
	-@erase "$(INTDIR)\mole_are.obj"
	-@erase "$(INTDIR)\mole_mob.obj"
	-@erase "$(INTDIR)\mole_msc.obj"
	-@erase "$(INTDIR)\mole_obj.obj"
	-@erase "$(INTDIR)\mole_rst.obj"
	-@erase "$(INTDIR)\mole_wld.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\player.obj"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\reset.obj"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\site.obj"
	-@erase "$(INTDIR)\skill.obj"
	-@erase "$(INTDIR)\social.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\str.obj"
	-@erase "$(INTDIR)\thing.obj"
	-@erase "$(INTDIR)\timing.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\WinsockExt.obj"
	-@erase "$(INTDIR)\world.obj"
	-@erase "$(OUTDIR)\Crimson2.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/Crimson2.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\./Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Crimson2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386
LINK32_FLAGS=wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug\
 /machine:I386 /out:"$(OUTDIR)/Crimson2.exe" 
LINK32_OBJS= \
	"$(INTDIR)\affect.obj" \
	"$(INTDIR)\alias.obj" \
	"$(INTDIR)\area.obj" \
	"$(INTDIR)\base.obj" \
	"$(INTDIR)\board.obj" \
	"$(INTDIR)\char.obj" \
	"$(INTDIR)\cmd_area.obj" \
	"$(INTDIR)\cmd_brd.obj" \
	"$(INTDIR)\cmd_cbt.obj" \
	"$(INTDIR)\cmd_code.obj" \
	"$(INTDIR)\cmd_god.obj" \
	"$(INTDIR)\cmd_help.obj" \
	"$(INTDIR)\cmd_inv.obj" \
	"$(INTDIR)\cmd_misc.obj" \
	"$(INTDIR)\cmd_mob.obj" \
	"$(INTDIR)\cmd_mole.obj" \
	"$(INTDIR)\cmd_move.obj" \
	"$(INTDIR)\cmd_obj.obj" \
	"$(INTDIR)\cmd_rst.obj" \
	"$(INTDIR)\cmd_talk.obj" \
	"$(INTDIR)\cmd_wld.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\codestuf.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\crimson2.obj" \
	"$(INTDIR)\decomp.obj" \
	"$(INTDIR)\edit.obj" \
	"$(INTDIR)\effect.obj" \
	"$(INTDIR)\exit.obj" \
	"$(INTDIR)\extra.obj" \
	"$(INTDIR)\fight.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\function.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\index.obj" \
	"$(INTDIR)\ini.obj" \
	"$(INTDIR)\interp.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mobile.obj" \
	"$(INTDIR)\mole.obj" \
	"$(INTDIR)\mole_are.obj" \
	"$(INTDIR)\mole_mob.obj" \
	"$(INTDIR)\mole_msc.obj" \
	"$(INTDIR)\mole_obj.obj" \
	"$(INTDIR)\mole_rst.obj" \
	"$(INTDIR)\mole_wld.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\player.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\reset.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\site.obj" \
	"$(INTDIR)\skill.obj" \
	"$(INTDIR)\social.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\str.obj" \
	"$(INTDIR)\thing.obj" \
	"$(INTDIR)\timing.obj" \
	"$(INTDIR)\WinsockExt.obj" \
	"$(INTDIR)\world.obj"

"$(OUTDIR)\Crimson2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "Crimson2 - Win32 Release"
# Name "Crimson2 - Win32 Debug"

!IF  "$(CFG)" == "Crimson2 - Win32 Release"

!ELSEIF  "$(CFG)" == "Crimson2 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\src\world.c
DEP_CPP_WORLD=\
	".\src\area.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\property.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\world.obj" : $(SOURCE) $(DEP_CPP_WORLD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\alias.c
DEP_CPP_ALIAS=\
	".\src\alias.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\alias.obj" : $(SOURCE) $(DEP_CPP_ALIAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\area.c
DEP_CPP_AREA_=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\area.obj" : $(SOURCE) $(DEP_CPP_AREA_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\base.c
DEP_CPP_BASE_=\
	".\src\base.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\mem.h"\
	".\src\queue.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\base.obj" : $(SOURCE) $(DEP_CPP_BASE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\board.c
DEP_CPP_BOARD=\
	".\src\base.h"\
	".\src\board.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\send.h"\
	".\src\social.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\board.obj" : $(SOURCE) $(DEP_CPP_BOARD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\char.c
DEP_CPP_CHAR_=\
	".\src\affect.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\char.obj" : $(SOURCE) $(DEP_CPP_CHAR_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_area.c
DEP_CPP_CMD_A=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_area.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_area.obj" : $(SOURCE) $(DEP_CPP_CMD_A) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_brd.c
DEP_CPP_CMD_B=\
	".\src\base.h"\
	".\src\board.h"\
	".\src\char.h"\
	".\src\cmd_brd.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_brd.obj" : $(SOURCE) $(DEP_CPP_CMD_B) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_cbt.c
DEP_CPP_CMD_C=\
	".\src\affect.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_cbt.h"\
	".\src\cmd_move.h"\
	".\src\crimson2.h"\
	".\src\effect.h"\
	".\src\exit.h"\
	".\src\fight.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_cbt.obj" : $(SOURCE) $(DEP_CPP_CMD_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_code.c
DEP_CPP_CMD_CO=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_code.h"\
	".\src\cmd_god.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\decomp.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\function.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_code.obj" : $(SOURCE) $(DEP_CPP_CMD_CO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_god.c
DEP_CPP_CMD_G=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_god.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_move.h"\
	".\src\cmd_talk.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_god.obj" : $(SOURCE) $(DEP_CPP_CMD_G) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_help.c
DEP_CPP_CMD_H=\
	".\src\base.h"\
	".\src\cmd_help.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\parse.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_help.obj" : $(SOURCE) $(DEP_CPP_CMD_H) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_inv.c
DEP_CPP_CMD_I=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\board.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_inv.obj" : $(SOURCE) $(DEP_CPP_CMD_I) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_misc.c
DEP_CPP_CMD_M=\
	".\src\affect.h"\
	".\src\alias.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_god.h"\
	".\src\cmd_misc.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_misc.obj" : $(SOURCE) $(DEP_CPP_CMD_M) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_mob.c
DEP_CPP_CMD_MO=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_mob.h"\
	".\src\cmd_wld.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_mob.obj" : $(SOURCE) $(DEP_CPP_CMD_MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_mole.c
DEP_CPP_CMD_MOL=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_mole.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_are.h"\
	".\src\mole_mob.h"\
	".\src\mole_msc.h"\
	".\src\mole_obj.h"\
	".\src\mole_rst.h"\
	".\src\mole_wld.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_mole.obj" : $(SOURCE) $(DEP_CPP_CMD_MOL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_move.c
DEP_CPP_CMD_MOV=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_move.obj" : $(SOURCE) $(DEP_CPP_CMD_MOV) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_obj.c
DEP_CPP_CMD_O=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_obj.h"\
	".\src\cmd_wld.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_obj.obj" : $(SOURCE) $(DEP_CPP_CMD_O) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_rst.c
DEP_CPP_CMD_R=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_area.h"\
	".\src\cmd_rst.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_rst.obj" : $(SOURCE) $(DEP_CPP_CMD_R) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_talk.c
DEP_CPP_CMD_T=\
	".\src\affect.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_talk.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\cmd_talk.obj" : $(SOURCE) $(DEP_CPP_CMD_T) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\cmd_wld.c
DEP_CPP_CMD_W=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_move.h"\
	".\src\cmd_wld.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\cmd_wld.obj" : $(SOURCE) $(DEP_CPP_CMD_W) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\code.c
DEP_CPP_CODE_=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\decomp.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\function.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\timing.h"\
	".\src\world.h"\
	

"$(INTDIR)\code.obj" : $(SOURCE) $(DEP_CPP_CODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\codestuf.c
DEP_CPP_CODES=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\decomp.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\function.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\codestuf.obj" : $(SOURCE) $(DEP_CPP_CODES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\compile.c
DEP_CPP_COMPI=\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\extra.h"\
	".\src\function.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\send.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\crimson2.c
DEP_CPP_CRIMS=\
	".\src\crimson2.h"\
	".\src\macro.h"\
	".\src\str.h"\
	

"$(INTDIR)\crimson2.obj" : $(SOURCE) $(DEP_CPP_CRIMS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\decomp.c
DEP_CPP_DECOM=\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\decomp.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\function.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\decomp.obj" : $(SOURCE) $(DEP_CPP_DECOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\edit.c
DEP_CPP_EDIT_=\
	".\src\base.h"\
	".\src\board.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\edit.obj" : $(SOURCE) $(DEP_CPP_EDIT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\effect.c
DEP_CPP_EFFEC=\
	".\src\affect.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_move.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\fight.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\effect.obj" : $(SOURCE) $(DEP_CPP_EFFEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\exit.c
DEP_CPP_EXIT_=\
	".\src\crimson2.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\exit.obj" : $(SOURCE) $(DEP_CPP_EXIT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\extra.c
DEP_CPP_EXTRA=\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\extra.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\extra.obj" : $(SOURCE) $(DEP_CPP_EXTRA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\fight.c
DEP_CPP_FIGHT=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_cbt.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\exit.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\fight.obj" : $(SOURCE) $(DEP_CPP_FIGHT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\file.c
DEP_CPP_FILE_=\
	".\src\crimson2.h"\
	".\src\file.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\str.h"\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\file.obj" : $(SOURCE) $(DEP_CPP_FILE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\function.c
DEP_CPP_FUNCT=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\decomp.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\function.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\function.obj" : $(SOURCE) $(DEP_CPP_FUNCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\help.c
DEP_CPP_HELP_=\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\send.h"\
	".\src\social.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\help.obj" : $(SOURCE) $(DEP_CPP_HELP_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\history.c
DEP_CPP_HISTO=\
	".\src\alias.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	

"$(INTDIR)\history.obj" : $(SOURCE) $(DEP_CPP_HISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\index.c
DEP_CPP_INDEX=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\index.obj" : $(SOURCE) $(DEP_CPP_INDEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\ini.c
DEP_CPP_INI_C=\
	".\src\crimson2.h"\
	".\src\log.h"\
	".\src\str.h"\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\ini.obj" : $(SOURCE) $(DEP_CPP_INI_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\interp.c
DEP_CPP_INTER=\
	".\src\base.h"\
	".\src\code.h"\
	".\src\codestuf.h"\
	".\src\compile.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\function.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\interp.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\parse.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\interp.obj" : $(SOURCE) $(DEP_CPP_INTER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\log.c
DEP_CPP_LOG_C=\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\log.obj" : $(SOURCE) $(DEP_CPP_LOG_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mem.c
DEP_CPP_MEM_C=\
	".\src\crimson2.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\str.h"\
	

"$(INTDIR)\mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mobile.c
DEP_CPP_MOBIL=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_cbt.h"\
	".\src\cmd_move.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mobile.obj" : $(SOURCE) $(DEP_CPP_MOBIL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole.c
DEP_CPP_MOLE_=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole.obj" : $(SOURCE) $(DEP_CPP_MOLE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_are.c
DEP_CPP_MOLE_A=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_are.obj" : $(SOURCE) $(DEP_CPP_MOLE_A) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_mob.c
DEP_CPP_MOLE_M=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_mob.obj" : $(SOURCE) $(DEP_CPP_MOLE_M) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_msc.c
DEP_CPP_MOLE_MS=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_msc.obj" : $(SOURCE) $(DEP_CPP_MOLE_MS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_obj.c
DEP_CPP_MOLE_O=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_obj.obj" : $(SOURCE) $(DEP_CPP_MOLE_O) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_wld.c
DEP_CPP_MOLE_W=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_wld.obj" : $(SOURCE) $(DEP_CPP_MOLE_W) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\object.c
DEP_CPP_OBJEC=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\board.h"\
	".\src\cmd_inv.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\effect.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\property.h"\
	".\src\send.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\parse.c
DEP_CPP_PARSE=\
	".\src\alias.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_area.h"\
	".\src\cmd_brd.h"\
	".\src\cmd_cbt.h"\
	".\src\cmd_code.h"\
	".\src\cmd_god.h"\
	".\src\cmd_help.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_misc.h"\
	".\src\cmd_mob.h"\
	".\src\cmd_mole.h"\
	".\src\cmd_move.h"\
	".\src\cmd_obj.h"\
	".\src\cmd_rst.h"\
	".\src\cmd_talk.h"\
	".\src\cmd_wld.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\moledefs.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\social.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\parse.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\player.c
DEP_CPP_PLAYE=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\cmd_misc.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\player.obj" : $(SOURCE) $(DEP_CPP_PLAYE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\property.c
DEP_CPP_PROPE=\
	".\src\alias.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\extra.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\property.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\property.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\queue.c
DEP_CPP_QUEUE=\
	".\src\crimson2.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\queue.h"\
	

"$(INTDIR)\queue.obj" : $(SOURCE) $(DEP_CPP_QUEUE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\reset.c
DEP_CPP_RESET=\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\property.h"\
	".\src\reset.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\reset.obj" : $(SOURCE) $(DEP_CPP_RESET) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\send.c
DEP_CPP_SEND_=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\macro.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\send.obj" : $(SOURCE) $(DEP_CPP_SEND_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\site.c
DEP_CPP_SITE_=\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\winsockext.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\site.obj" : $(SOURCE) $(DEP_CPP_SITE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\skill.c
DEP_CPP_SKILL=\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\crimson2.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\skill.obj" : $(SOURCE) $(DEP_CPP_SKILL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\social.c
DEP_CPP_SOCIA=\
	".\src\base.h"\
	".\src\char.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\extra.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\parse.h"\
	".\src\send.h"\
	".\src\social.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\social.obj" : $(SOURCE) $(DEP_CPP_SOCIA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\socket.c
DEP_CPP_SOCKE=\
	".\src\alias.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\board.h"\
	".\src\char.h"\
	".\src\cmd_move.h"\
	".\src\cmd_talk.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\effect.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\help.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\social.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\timing.h"\
	".\src\winsockext.h"\
	".\src\world.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\socket.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\str.c
DEP_CPP_STR_C=\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\str.h"\
	

"$(INTDIR)\str.obj" : $(SOURCE) $(DEP_CPP_STR_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\thing.c
DEP_CPP_THING=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_cbt.h"\
	".\src\cmd_inv.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mobile.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\thing.obj" : $(SOURCE) $(DEP_CPP_THING) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\timing.c
DEP_CPP_TIMIN=\
	".\src\crimson2.h"\
	".\src\timing.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\timing.obj" : $(SOURCE) $(DEP_CPP_TIMIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\WinsockExt.c
DEP_CPP_WINSO=\
	".\src\crimson2.h"\
	".\src\winsockext.h"\
	

"$(INTDIR)\WinsockExt.obj" : $(SOURCE) $(DEP_CPP_WINSO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\affect.c
DEP_CPP_AFFEC=\
	".\src\affect.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\cmd_inv.h"\
	".\src\crimson2.h"\
	".\src\effect.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\object.h"\
	".\src\player.h"\
	".\src\queue.h"\
	".\src\skill.h"\
	".\src\str.h"\
	".\src\thing.h"\
	

"$(INTDIR)\affect.obj" : $(SOURCE) $(DEP_CPP_AFFEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src\mole_rst.c
DEP_CPP_MOLE_R=\
	".\src\affect.h"\
	".\src\area.h"\
	".\src\base.h"\
	".\src\char.h"\
	".\src\code.h"\
	".\src\crimson2.h"\
	".\src\edit.h"\
	".\src\exit.h"\
	".\src\extra.h"\
	".\src\fight.h"\
	".\src\file.h"\
	".\src\history.h"\
	".\src\index.h"\
	".\src\ini.h"\
	".\src\log.h"\
	".\src\macro.h"\
	".\src\mem.h"\
	".\src\mobile.h"\
	".\src\mole.h"\
	".\src\mole_msc.h"\
	".\src\moledefs.h"\
	".\src\object.h"\
	".\src\parse.h"\
	".\src\player.h"\
	".\src\property.h"\
	".\src\queue.h"\
	".\src\reset.h"\
	".\src\send.h"\
	".\src\site.h"\
	".\src\skill.h"\
	".\src\socket.h"\
	".\src\str.h"\
	".\src\thing.h"\
	".\src\world.h"\
	

"$(INTDIR)\mole_rst.obj" : $(SOURCE) $(DEP_CPP_MOLE_R) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
