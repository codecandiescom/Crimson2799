/* Call this function to let other windows have a chance to execute */
/* Please note that other windows belonging to the application using this
   function will also get a chance to execute! You can avoid this by passing
   the hWnd parameter of the window calling PeekMessageLoop */

/* If this routine returns FALSE, then the user has just quit your
   application */

int PeekMessageLoop(HWND hWnd) {
  MSG msg;

  while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT)
      return FALSE;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return TRUE;
}

void main() {
  while(1) {
    /* In the midst of your code let other things run */
    if (!PeekMessageLoop(NULL)) break;
  }
}