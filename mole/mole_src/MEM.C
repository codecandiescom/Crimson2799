// mem.c
// two-letter descriptor: me        
// ****************************************************************************
// Copyright (C) B. Cameron Lesiuk, 1999. All rights reserved.
// Permission to use/copy this code is granted for non-commercial use only.
// B. Cameron Lesiuk
// Victoria, BC, Canada
// wi961@freenet.victoria.bc.ca
// ****************************************************************************
//
// This module provides some common memory functions

void *meMemCpy(void *p_dest,void *p_src, unsigned long p_size) {
  unsigned long l_i;
  unsigned char *l_src,*l_dst;

  if ((!p_dest)||(!p_src))
    return ((void*)0L);

  l_src=(unsigned char *)p_src;
  l_dst=(unsigned char *)p_dest;

  for (l_i=0L;l_i<p_size;l_i++) {
    *l_dst=*l_src;
    l_dst++;
    l_src++;
  }
  return p_dest;
}

void *meMemClr(void *p_dest, unsigned long p_size) {
  unsigned long l_i;
  unsigned char *l_dst;

  if (!p_dest)
    return ((void*)0L);

  l_dst=(unsigned char *)p_dest;

  for (l_i=0L;l_i<p_size;l_i++) {
    *l_dst=(unsigned char)0;
    l_dst++;
  }
  return p_dest;
}
