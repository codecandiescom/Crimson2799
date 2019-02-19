// infobox.c
// Two-letter Module Descriptor: ib
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************

/* This module contains code to make and operate an information box, much
 * like the standard windows created with the MessageBox function.
 * I did this so that I had better control of what the message box looks
 * like.
 */

#include<windows.h>
#include<windowsx.h> /* for message crackers */
#include<stdio.h>
#include<ctl3d.h>
#include"ctl3dl.h"
#include<string.h>
#include"molerc.h"
#include"molem.h"
#include"infobox.h"

/* local typedefs */
typedef struct IBPARAMtag {
  unsigned long style;
	LPCSTR title;
	LPSTR text;
	LPCSTR icon;
	DWORD help;
  HGLOBAL global;
  } IBPARAM;

/* local defines */
#define IB_NUM_BUTTONS 8 // how many buttons do we service here?
#define IB_NUM_INFOBOX_POSITIONS 4 // number of infobox positions on main screen - this affects the positioning of the infobox
#define IB_MAX_BUTTON_WIDTH 80 // button width max
#define IB_MIN_BUTTON_WIDTH 70 // minimum button width
#define IB_MAX_SPACE_WIDTH  20 // space between the buttons max
#define IB_MIN_SPACE_WIDTH  2  // space between the buttons min
#define IB_MIN_TEXT_SPACE  6  // space between the buttons min

/* Globals */
/* Note this module doesn't use any state-dependent globals;
 * it is designed to be re-entrant. */
/* g_ibBoxNum keeps track of which new-box position to use. Yes,
 * there is the possibility of this variable getting mucked up
 * by an interruption at "just the right moment", but hey, it
 * just means that two infoboxes will be a little out of kilter,
 * with a possible complete overlap. This is definately non-fatal! */
int g_ibBoxNum=0;
/* All buttons are recorded (flags & values) here. The order is
 * important: the earlier buttons get the focus and are placed
 * before the later buttons! IE: IDYES and IDNO are both visible-
 * IDYES is to the left of IDNO in the box, and IDYES gets the
 * initial focus, because it's located earlier in these lists!!! */
int g_ibButValue[IB_NUM_BUTTONS]={IDYES ,IDNO ,IDOK ,IDCANCEL ,IDABORT,
  IDRETRY ,IDIGNORE ,IDHELP };
long g_ibButFlag[IB_NUM_BUTTONS]={IB_YES,IB_NO,IB_OK,IB_CANCEL,IB_ABORT,
  IB_RETRY,IB_IGNORE,IB_HELP};

/* ibInfoBox
 * Creates a pop-up window to ask the user a quick question. The
 * p_icon is supposed to be a resource identifier of a bitmap resource
 * to put beside the text.
 */
int ibInfoBox(HWND p_hWnd,LPCSTR p_text,LPCSTR p_title,unsigned long p_style,
  LPCSTR p_icon,DWORD p_help)
  {
  IBPARAM l_param;
  char l_buf[2]=" ";
  int l_rc;

  l_param.global=NULL;
  if (p_text)
    if (p_text[0])
      l_param.global=GlobalAlloc(GHND|GMEM_NOCOMPACT,strlen(p_text)+1);

  if (l_param.global)
    {
    l_param.text=(LPSTR)GlobalLock(l_param.global);
    if (!l_param.text)
      l_param.text=l_buf;
    else
      strcpy(l_param.text,p_text);
    }
  else
    l_param.text=l_buf;

  l_param.style=p_style;
  l_param.title=p_title;
  l_param.icon=p_icon;
  l_param.help=p_help;

  l_rc=DialogBoxParam(g_aahInst,MAKEINTRESOURCE(DIALOG_INFOBOX),p_hWnd,ibInfoBoxProc,(LPARAM)(&l_param));
  if (l_param.global)
    {
	  GlobalUnlock(l_param.global);
	  GlobalFree(l_param.global);
    }
  return l_rc;
  }

BOOL CALLBACK _export ibInfoBoxProc(HWND p_hWnd, UINT p_message,
											WPARAM p_wParam, LPARAM p_lParam)
	{
	switch (p_message)
		{
		case WM_INITDIALOG:
			{
			IBPARAM *l_param;
			int l_i,l_j,l_default;
			HWND l_hWnd;
			RECT l_rect;
			int l_numbuttons,l_centreX,l_buttonX,l_spaceX,l_tempX;
			int l_positionY,l_buttonY;
			int l_longsize;
			POINT l_point,l_textsize;
			HDC l_hDC;
			char *l_p1,*l_p2,l_oldp2;

			/* Subclass this box so we appear 3-D. whoah. */
			Ctl3dSubclassDlgEx(p_hWnd,CTL3D_ALL);

			/* note: icon is currently ignored */
			l_param=(IBPARAM *)p_lParam;
			SetWindowText(p_hWnd,l_param->title);
			SetDlgItemText(p_hWnd,IDC_INFOBOX_TEXT,l_param->text);
			SetDlgItemInt(p_hWnd,IDC_INFOBOX_HELP,LOWORD(l_param->help),FALSE);

			/* First, count the buttons */
			l_numbuttons=0;
			for (l_i=0;l_i<IB_NUM_BUTTONS;l_i++)
				if (l_param->style & g_ibButFlag[l_i])
					l_numbuttons++;

			/* now resize the dialog box according to how big our
			 * text string is. */
			GetClientRect(l_hWnd=GetDlgItem(p_hWnd,IDC_INFOBOX_TEXT),&l_rect); // get original text box size - this is our MAX size
			l_point.x=l_point.y=0; // get our initial Y position
			MapWindowPoints(l_hWnd,p_hWnd,&l_point,1);
			l_positionY=l_point.y; // l_positionY is the Y coord for the top of the text box.
			l_hDC=GetDC(l_hWnd);
			/* note: from this point on, l_point.x and l_point.y represent our text box
			 * x and y dimentions (ie width and height). */
			/* let's find our longest line */
			l_longsize=0;
			l_p1=l_p2=(char *)l_param->text;
			do
				{
				if ((*l_p2=='\r')||(*l_p2==0)) // carridge return
					{
					l_oldp2=*l_p2;
					*l_p2=0;
					GetTextExtentPoint(l_hDC,l_p1,strlen(l_p1),(SIZE *)&l_textsize);
					*l_p2=l_oldp2;
					if (l_textsize.x>l_longsize)
						l_longsize=l_textsize.x;
					l_p1=l_p2+1;
					}
				l_p2++;
				}
			while (*(l_p2-1)!=0);
			/* ok, l_longsize is our longest size. Now, shove this through our MAX WIDTH filter */
			l_point.x=l_longsize;
			/* also use a left-over l_textsize.y value to record our line height */
			l_point.y=l_textsize.y;

			l_i=l_numbuttons*IB_MIN_BUTTON_WIDTH+
				(l_numbuttons-1)*IB_MIN_SPACE_WIDTH; // absolute min window width - to fit buttons in!
			if (l_point.x>l_rect.right) // NOTE: this would be l_rect.right-l_rect.left, but GetClientRect returns l_rect.top=l_rect.left=0! Cool.
				l_point.x=l_rect.right;
			if (l_point.x<l_i) // make sure we have enough room for buttons!!
				l_point.x=l_i;   // note: buttons can override max width!
			/* ok, now comes the height. We want to move the window down so that the
			 * text is centred vertically. However, to do this properly, we need to
			 * know how many lines there are. l_point.y currently gives us the height
			 * of a single line of text. We have to check each line of text, though, for
			 * wrap. */
			l_i=0; // number of rows (lines) of text we have, ACCOUNTING FOR TEXT WRAP
			l_p1=l_p2=(char *)l_param->text;
			do
				{
				if ((*l_p2=='\r')||(*l_p2==0)) // carridge return
					{
					l_oldp2=*l_p2;
					*l_p2=0;
					GetTextExtentPoint(l_hDC,l_p1,strlen(l_p1),(SIZE *)&l_textsize);
					*l_p2=l_oldp2;
					l_i+=1+((l_textsize.x-1)/l_point.x); // add 1 for this line, + 1 for each wrapped line
					l_p1=l_p2+1;
					}
				l_p2++;
				}
			while (*(l_p2-1)!=0);

			/* ok. l_i holds the number of lines.*/
			l_point.y*=l_i; // multiply # lines (l_i) times height per line (l_point.y) and
											// this becomes our text box raw height.
			if (l_point.y>l_rect.bottom) // update our Y height - check make sure not too long
				l_point.y=l_rect.bottom;
			l_positionY+=(l_rect.bottom-l_point.y)/2; // and our Y pos - centred nicely
			l_point.y=l_rect.bottom-l_positionY; // drop bottom as far as it will go, anyways

			/* Ok, we now have our width, our height, and our Y position. */
			/* Adjust our dialog box width according to our new calculations
			 * and move our window in the Y position to centre
			 * the text vertically */
			/* first, move & resize the text box */
			SetWindowPos(l_hWnd,HWND_TOP,IB_MIN_TEXT_SPACE,l_positionY,
				l_point.x,l_point.y,SWP_NOACTIVATE|SWP_NOZORDER);
			/* and secondly, resize the dialog box (x-direction only) */
			GetWindowRect(p_hWnd,&l_rect);
			SetWindowPos(p_hWnd,HWND_TOP,0,0,l_point.x+2*(IB_MIN_TEXT_SPACE+
				GetSystemMetrics(SM_CXDLGFRAME)),l_rect.bottom-l_rect.top,
				SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);
			ReleaseDC(l_hWnd,l_hDC);

			/* next, position this dialog box in the centre of the screen,
			 * with a slight offset between info boxes kept in track by
			 * g_ibBoxNum */
			l_i=g_ibBoxNum; // do this as fast as possible to avoid interruption
			g_ibBoxNum++;
			if (g_ibBoxNum>IB_NUM_INFOBOX_POSITIONS)
				g_ibBoxNum=0;
			GetWindowRect(GetDesktopWindow(),&l_rect); // find middle of screen
			l_point.x=(l_rect.right-l_rect.left)/2;
			l_point.y=(l_rect.bottom-l_rect.top)/3; // l_point is now the scrn middle
				 // NOTE: we divide l_point.y by **3** to achieve the "visual"
				 // centre of the screen - ie where everyone naturally looks!!!
				 // If we plop the window in the real centre, it "appears" too
				 // low to the natural human eye.
			l_j=GetSystemMetrics(SM_CYCAPTION); // get header height
			GetWindowRect(p_hWnd,&l_rect); // get dimentions of this dialog box
			l_point.x-=(l_rect.right-l_rect.left)/2; // get top-left of centred dialog box
			l_point.y-=(l_rect.bottom-l_rect.top)/2;
			l_point.x-=l_j*(IB_NUM_INFOBOX_POSITIONS/2); // find top-left of zero-th dialog box
			l_point.y-=l_j*(IB_NUM_INFOBOX_POSITIONS/2);
			l_point.x+=l_i*l_j; // find top-left of THIS dialog box
			l_point.y+=l_i*l_j;
			/* and now do the great re-positioning deed (without resize) */
			SetWindowPos(p_hWnd,HWND_TOP,l_point.x,l_point.y,0,0,SWP_NOACTIVATE|SWP_NOSIZE);

			/* Check we've got at least one functional button or the user
			 * can't exit!! The special case examined here is for the only
			 * button being IB_HELP with IB_HELPDEF, which would give the
			 * user a single button - the help button - and they still
			 * couldn't exit... but they could get help on how they
			 * can't exit! */
			if ((!l_numbuttons)||((l_numbuttons==1)&&
				(l_param->style&IB_HELP)&&(l_param->style&IB_HELPDEF)))
				{
				l_numbuttons++;
				l_param->style|=g_ibButFlag[0];
				}

			/* next, calculate button sizes  */
			GetClientRect(p_hWnd,&l_rect);
			l_tempX=l_rect.right-l_rect.left;  // get width of our window
			l_centreX=l_tempX/2;  // we position buttons from the centre
														// out so that round-off errors result
														// in a nicely even and centred button
														// array, as opposed to a button array
														// which is positioned a little to one
														// side.
			l_tempX-=2*IB_MIN_SPACE_WIDTH; // leave space at left & right edge
			l_tempX=l_tempX/l_numbuttons; // rough space per button
			l_buttonX=l_tempX-IB_MIN_SPACE_WIDTH; // rough button width
			l_spaceX=IB_MIN_SPACE_WIDTH;          // rough space width (space between buttons)
			if (l_buttonX>IB_MAX_BUTTON_WIDTH)    // finalize button width
				{
				l_buttonX=IB_MAX_BUTTON_WIDTH;
				l_spaceX=l_tempX-l_buttonX;         // re-rough space width
				}
			if (l_spaceX>IB_MAX_SPACE_WIDTH)      // finalize button space
				l_spaceX=IB_MAX_SPACE_WIDTH;

			/* We've got our button width, our space between the button width,
			 * and our window centre mark. That's all we need!
			 * Let's position our buttons (boogey time!)                 */
			/* First, l_tempX is our starting button position. We don't, at
			 * this point, have to worry about round-off problems because
			 * that's all taken care of now that we've finalized our
			 * width calculations. We may be a single pixel off, but that's
			 * close enough for our purposes. */
			if (l_numbuttons>1)
				l_tempX=l_centreX-
					((l_numbuttons*l_buttonX)+((l_numbuttons-1)*l_spaceX))/2;
			else
				l_tempX=l_centreX-l_buttonX/2;
			/* we also need the Y value for our buttons */
			GetWindowRect(l_hWnd=GetDlgItem(p_hWnd,g_ibButValue[0]),&l_rect);
			l_buttonY=l_rect.bottom-l_rect.top; // record height - don't change
			l_point.x=l_rect.left;
			l_point.y=l_rect.top;
			ScreenToClient(p_hWnd,&l_point);
			l_positionY=l_point.y; // record Y position - don't change
			for (l_i=0;l_i<IB_NUM_BUTTONS;l_i++) // go through, positioning buttons
				if (l_param->style & g_ibButFlag[l_i])
					{
					MoveWindow(GetDlgItem(p_hWnd,g_ibButValue[l_i]),l_tempX,
						l_positionY,l_buttonX,l_buttonY,FALSE);
					l_tempX+=l_buttonX+l_spaceX;
					}

			/* now, based on p_style, enable buttons */
			for (l_i=IB_NUM_BUTTONS-1;l_i>=0;l_i--)
				{
				if (l_param->style & g_ibButFlag[l_i])
					{
					EnableWindow(l_hWnd=GetDlgItem(p_hWnd,g_ibButValue[l_i]),TRUE);
					ShowWindow(l_hWnd,SW_SHOWNA);
					l_default=l_i;
					}
				}
			SetFocus(GetDlgItem(p_hWnd,g_ibButValue[l_default]));

			if (l_param->style & IB_HELPDEF)
				SetDlgItemInt(p_hWnd,IDC_INFOBOX_HELP,(UINT)l_param->help,FALSE);
			else
				SetDlgItemInt(p_hWnd,IDC_INFOBOX_HELP,0,FALSE);

			return TRUE;
			}
		case WM_COMMAND:
			{
			switch (GET_WM_COMMAND_ID(p_wParam,p_lParam))
				{
				case IDYES:
				case IDNO:
				case IDOK:
				case IDCANCEL:
				case IDABORT:
				case IDRETRY:
				case IDIGNORE:
					EndDialog(p_hWnd,GET_WM_COMMAND_ID(p_wParam,p_lParam));
					return TRUE;
				case IDHELP:
					{
					unsigned int l_i;
					BOOL l_bool;
					if ((l_i=GetDlgItemInt(p_hWnd,IDC_INFOBOX_HELP,&l_bool,FALSE))!=0)
						WinHelp(g_aahWnd,g_aaHelpFile,HELP_CONTEXT,(DWORD)l_i);
					else
						EndDialog(p_hWnd,GET_WM_COMMAND_ID(p_wParam,p_lParam));
					return TRUE;
					}
				default:
					break;
				}
			}
			break;
		default:
			break;
		}
	return d3DlgMessageCheck(p_hWnd,p_message,p_wParam,p_lParam);
	}


