// editextr.h
// two-character identifier: ee
// Extra editing tool functions.

// Extras and Properties share the same structure. Therefore, to make
// things a little easier, and reduce duplication a little wee bit,
// I've just mapped the ee* functions to their ep* counterparts!

#define eeEditExtra(p_hWnd,p_ptr,p_sType,p_newType) epEditProperty(p_hWnd,p_ptr,p_sType,p_newType)


