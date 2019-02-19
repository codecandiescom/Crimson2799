// help.h
// Two letter descriptor: hp

/* HELP.H
 * This file contains the "glue" between the program and the help file.
 * ALL references to the Help file are contained in this header file.
 * This header file has no .C component.
 * This file provides the ability to centralize the help file effort
 * and is extremely convenient during help file modifications.
 */

/* Help file reference naming conventions:
 * Suppose you have a dialog box DIALOG_NEWPASSWORD located in
 * the file DIALOG.C. The help reference for the help button
 * goes like this:
 *        HP_DL_DIALOG_NEWPASSWORD_HELP
 * The breakdown is like so:
 *        <help.h descriptor> <dialog.c descriptor> <help source item name> <a descriptor of your choice>
 * Got it? In the above example note:
 *        DL is the two letter descriptor of DIALOG.C
 *        the "DIALOG_NEWPASSWORD" corresponds to the dialog box name.
 * For the bazillions of ibInfoBox pop-up dialog boxes, just use "IB" for the
 * help source item name. Example:
 *        HP_DL_IB_NEWPASS_ERROR
 * Good-O.
 */

/* note: the infobox module uses 16-bit identifiers for its
 * help system. Thus, although the help system can take 32 bits,
 * try to keep the define's here to under 32767. Now, the system
 * SHOULD be able to take numbers up to 65535 with no problem,
 * but I have not tested the system with the signed numbers
 * from 32768-65535, so I advise against using those. Besides,
 * do you REALLY need that many help screens!? Whoah! */

/* Dialog Box Help References
** The following are help references directly from unique dialog boxes (as opposed
** to ibInfoBox dialog boxes). These help references should have meaningfull names.
*/
#define HP_DL_DIALOG_ABOUT_HELP        10000      // DIALOG_ABOUT       (about box)-About box help button
#define HP_HO_DIALOG_OPENHOST_HELP     10010      // DIALOG_OPENHOST    (connect to host)
#define HP_HO_DIALOG_NEWHOST_HELP      10020      // DIALOG_NEWHOST     (Create new host)
#define HP_HO_DIALOG_LOGIN_HELP        10030      // DIALOG_LOGIN       (prompt for name/pw)
#define HP_EM_DIALOG_EDITMOB_HELP      10040      // DIALOG_EDITMOB     (MOB edit window)
#define HP_EP_DIALOG_EDITPROP_HELP     10050      // DIALOG_EDITPROP    (Property Edit Window)
#define HP_EF_DIALOG_EDITFLAG_HELP     10060      // DIALOG_EDITFLAG    (Flag Edit Window)
#define HP_EW_DIALOG_EDITWLD_HELP      10070      // DIALOG_EDITWLD     (World Edit Window)
#define HP_EO_DIALOG_EDITOBJ_HELP      10080      // DIALOG_EDITOBJ     (Object Edit Window)
#define HP_EA_DIALOG_EDITADT_HELP      10090      // DIALOG_EDITADT     (Area Detail Edit Window)
#define HP_ER_DIALOG_EDITRST_HELP      10100      // DIALOG_EDITRST     (Reset Edit Window)
#define HP_AT_DIALOG_ATOPT_HELP        10110      // DIALOG_ATOPT       (AntiTerm Options)
#define HP_AT_DIALOG_ATFNKEY_HELP      10120      // DIALOG_ATFNKEY     (AnsiTerm Macro Def'n)
#define HP_COMMDLG_FONT_HELP           10130      // CommDlg Font Help  (Common Dialog Box Font Picker)
#define HP_IB_AREALIST_RELOAD_WARN     10140      // edit.c             (edFetchItem) - You are about to re-load the Area List. This will cause any un-saved\nediting changes to be lost. Do you wish to continue?
#define HP_IB_WLDLIST_RELOAD_WARN      10150      // edit.c             (edFetchItem) - You are about to re-load a World List.\nUn-saved World editing changes will be lost (for this area only).\n Do you wish to continue?
#define HP_IB_MOBLIST_RELOAD_WARN      10160      // edit.c             (edFetchItem) - You are about to re-load a Mobile List.\nUn-saved Mobile editing changes will be lost (for this area only).\n Do you wish to continue?
#define HP_IB_OBJLIST_RELOAD_WARN      10170      // edit.c             (edFetchItem) - You are about to re-load an Object List.\nUn-saved Object editing changes will be lost (for this area only).\n Do you wish to continue?
#define HP_IB_MOLEEDITERROR_HELP       10180      // edit.c             (edEditError) - Error code (NACK) returned from server in response to a MOLE request
#define HP_IB_LOW_SYS_RES_WARN         10190      // edit.c             (edEditItem) - You are running short on system resources.\nTry closing some windows, eh?
#define HP_IB_DISCONNECT_WARN          10200      // host.c             (hoDisconnectHost) - You have an active connection with %s.\nAre you sure you wish to disconnect?
#define HP_IB_BAD_MOLE_VERSION         10210      // host.c             (hoDialogConnectProc) - MOLE cannot talk to this server (incompatible versions).\nContact your SysAdmin regarding gettting the proper version.
#define HP_IB_SOCKET_ERROR_NO_SOCK     10220      // host.c             (hoDialogConnectProc) - Cannot get a free socket!
#define HP_IB_SOCKET_ERROR_HOSTNAME    10230      // host.c             (hoDialogConnectProc) - Couldn't resolve host name '%s'.
#define HP_IB_SOCKET_ERROR_REFUSED     10240      // host.c             (hoDialogConnectProc) - Connection refused by host:\rServer isn't listening.
#define HP_IB_SOCKET_ERROR_MISC        10250      // host.c             (hoDialogConnectProc) - Error connecting to host.
#define HP_IB_SOCKET_ERROR_ASYNC       10260      // host.c             (hoDialogConnectProc) - Error setting socket to Async mode. Connection aborted.
#define HP_IB_SOCKET_ERROR_KILLED      10270      // host.c             (hoSocketHandler) - Connection has been unexpectedly terminated.
#define HP_IB_SOCKET_ERROR_CLOSED      10280      // host.c             (hoSocketHandler) - Connection closed by host.
#define HP_IB_SOCKET_ERROR_UNEXPCL     10290      // host.c             (hoSocketHandler) - Connection unexpectedly closed.
#define HP_IB_SOCKET_ERROR_NETERR      10300      // moleprot.c         (mpMoleSendData) - Connection closed due to a network error.


