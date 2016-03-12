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


void manual_init()
{
    sendAbort();
    sendManual();
    emcCommandWaitDone();
}

void manual_parse_serialdata()
{ 
    
    if( isprefix( "JG+" ,NULL ) )
    {
        sendJogIncr(status.axis, status.jogfeedrate, status.incr );
        return;
    }
    
    if( isprefix( "JG-" ,NULL ) )
    {
        sendJogIncr(status.axis, status.jogfeedrate, -status.incr );
        return;
    }
    

}

void manual_draw()
{
    
    draw_statusbar( "MANUAL" );
    
    sprintf(strbuf, "INCR     %4.3f mm\nFEEDRATE %d mm/min",status.incr, status.jogfeedrate );
    print( strbuf ,0,60,30);
    preview_draw();
    draw_dro();

}
