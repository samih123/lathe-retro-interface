/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

extern char strbuf[BUFFSIZE];
extern struct machinestatus status;
extern int screenw, screenh;

void mdi_init()
{
}

void mdi_parse_serialdata()
{
}

void mdi_draw()
{
    draw_statusbar( "MDI" );
    draw_dro();
}
