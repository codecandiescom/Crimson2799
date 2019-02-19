/* MOLE Protocol header file */

#ifndef _MOLEDEFS_H_
#define _MOLEDEFS_H_

/* Version Information */
#define MOLE_MAJOR_VERSION 3
#define MOLE_MINOR_VERSION 0

/* MOLE protocol constants */
/* If you change any of these, you may have to change them all */
#define MOLE_HDR        "##MOLE"
#define MOLE_HDR_SIZE    6         /* # bytes of MOLE_CMD */
#define MOLE_DELIM      "##"
#define MOLE_DELIM_CHAR '#'
#define MOLE_PKT_MAX     30        /* max # data bytes in a single msg */
                                   /* This leave our total packet size */
                                   /* maxing out at around 124 - under */
                                   /* 128 with a bit of room to play.  */
                                   /* Remember - each byte=2 chars.    */
#define MOLE_PKT_START  0x03       /* sent from server to client -     */
                                   /* start of MOLE packet!            */
#define MOLE_PKT_STOP   0x02       /* sent from server to client -     */
                                   /* end of MOLE packet!              */

/* MOLE Packet ID constants */
#define MOLE_PKID_NULL        0x00L /* invalid packet number */
#define MOLE_PKID_UNSOLICITED 0x01L /* unsolicited info from server to client */
#define MOLE_PKID_START       0x02L /* start of available packet numbers */

/* MOLE command constants */
#define MOLE_CMD_IDRQ 0x49445251L /* IDRQ - ID request  */
#define MOLE_CMD_IDEN 0x4944454EL /* IDEN - ID response */
#define MOLE_CMD_SYSR 0x53595352L /* SYSR - System Info Request (weapon types, world types, etc. - the lists ) */
#define MOLE_CMD_SYSD 0x53595344L /* SYSD - System Info Detail (weapon types, world types, etc. - the lists ) */
#define MOLE_CMD_MORE 0x4D4F5245L /* MORE - part of a multi-part msg */
#define MOLE_CMD_PLRQ 0x504C5251L /* PLRQ - on-line player request */
#define MOLE_CMD_PLST 0x504C5354L /* PLST - on-line player list */
#define MOLE_CMD_ALRQ 0x414C5251L /* ALRQ - area list request */
#define MOLE_CMD_ALST 0x414C5354L /* ALST - area list */
#define MOLE_CMD_ADRQ 0x41445251L /* ADRQ - request details on specified area */
#define MOLE_CMD_ADTL 0x4144544CL /* ADTL - provide details on specified area */
#define MOLE_CMD_ACPY 0x41435059L /* ACPY - area details copy to another area */
#define MOLE_CMD_RLRQ 0x524C5251L /* RLRQ - Reset list request */
#define MOLE_CMD_RLST 0x524C5354L /* RLST - Reset list */
#define MOLE_CMD_WLRQ 0x574C5251L /* WLRQ - World list (for an area) request */
#define MOLE_CMD_WLST 0x574C5354L /* WLST - World list (for an area) */
#define MOLE_CMD_WDRQ 0x57445251L /* WDRQ - World detail request */
#define MOLE_CMD_WDTL 0x5744544CL /* WDTL - World detail */
#define MOLE_CMD_WCPY 0x57435059L /* WCPY - World details copy to another world */
#define MOLE_CMD_MLRQ 0x4D4C5251L /* MLRQ - Mobile list (for an area) request */
#define MOLE_CMD_MLST 0x4D4C5354L /* MLST - Mobile list (for an area) */
#define MOLE_CMD_MDRQ 0x4D445251L /* MDRQ - Mobile detail request */
#define MOLE_CMD_MDTL 0x4D44544CL /* MDTL - Mobile detail */
#define MOLE_CMD_MCPY 0x4D435059L /* MCPY - Mobile details copy to another mobile */
#define MOLE_CMD_OLRQ 0x4F4C5251L /* OLRQ - Object list (for an area) request */
#define MOLE_CMD_OLST 0x4F4C5354L /* OLST - Object list (for an area) */
#define MOLE_CMD_ODRQ 0x4F445251L /* ODRQ - Object detail request */
#define MOLE_CMD_ODTL 0x4F44544CL /* ODTL - Object detail */
#define MOLE_CMD_OCPY 0x4F435059L /* OCPY - Object details copy to another object */
#define MOLE_CMD_NACK 0x4E41434BL /* NACK - specified command was not processed - error specified */
#define MOLE_CMD_ACKP 0x41434B50L /* ACKP - specified command was successfully processed */

/* MOLE NACK codes */
#define MOLE_NACK_NOTIMPLEMENTED 0x00000001L /* command not implemented */
#define MOLE_NACK_RESEND         0x00000002L /* garbled packet - please resend */
#define MOLE_NACK_AUTHORIZATION  0x00000003L /* you don't have the authorization (level) for the command */
#define MOLE_NACK_NODATA         0x00000004L /* area/wld/obj/mob doesn't exist */
#define MOLE_NACK_PROTECTION     0x00000005L /* access/protection violation - they don't have permission to do this to this area! */
#define MOLE_NACK_POSITION       0x00000006L /* They're not in position to do that command! (ie: sitting/sleeping/etc) */

/* List types */
#define MOLE_LISTTYPE_INVALID   0
#define MOLE_LISTTYPE_TYPE      1
#define MOLE_LISTTYPE_FLAG      2

/* LIST order - MUST CORRESPOND to IDRQ/IDEN messages */
#define MOLE_LIST_MTYPE         0
#define MOLE_LIST_SEX           1
#define MOLE_LIST_POS           2  
#define MOLE_LIST_MACT          3
#define MOLE_LIST_AFFECT        4
#define MOLE_LIST_WTYPE         5
#define MOLE_LIST_WFLAG         6
#define MOLE_LIST_OACT          7
#define MOLE_LIST_OSFLAG        8
#define MOLE_LIST_OWFLAG        9
#define MOLE_LIST_OAMMO        10
#define MOLE_LIST_OCFLAG       11
#define MOLE_LIST_OLIQUID      12
#define MOLE_LIST_APPLY        13
#define MOLE_LIST_RFLAG        14
#define MOLE_LIST_WEAR         15
#define MOLE_LIST_DIR          16
#define MOLE_LIST_EFLAG        17
#define MOLE_LIST_OTYPE        18
#define MOLE_LIST_RESIST       19
#define MOLE_LIST_WEAPON       20
#define MOLE_LIST_NUMLISTS     21 /* # items (1 + last list) */
#define MOLE_LIST_INVALID   32000 /* invalid list number */

#endif /* _MOLEDEFS_H_ */
