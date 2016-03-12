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

void home_init()
{
}

void home_parse_serialdata()
{
    int n;
    if( isprefix( "AX=" ,&n ) )
    {
        status.axis = n;
        if( ! emcStatus->motion.axis[n].homing )
        {
            sendHome( n );
        }
        return;
    }
}

void home_draw()
{
    draw_statusbar( "HOMING" );
    draw_dro();
}
