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
extern std::vector<string> lines;

list<operation> *wiz_opl;

void auto_init()
{
    sendAuto();
    emcCommandWaitDone();
    updateStatus();
    emcCommandWaitDone();
    preview( emcStatus->task.file );   
}

void auto_load( char *n )
{
    printf( "auto load %s\n", n );
    sendAuto();
    emcCommandWaitDone();
    sendProgramOpen( n );
    emcCommandWaitDone();
    updateStatus();
    emcCommandWaitDone();
    preview( emcStatus->task.file );
    wiz_opl = NULL;
}



void auto_parse_serialdata()
{
    
    if( status.feedoverride !=  status.feedoverride_new )
    {
        status.feedoverride =  status.feedoverride_new;
        sendFeedOverride(((double) status.feedoverride ) / 100.0);
    }
    
    if( isprefix( "START" ,NULL ) )
    {
        if( emcStatus->task.interpState ==  EMC_TASK_INTERP_IDLE )
        {
            sendProgramRun(0);
        }
        else if( emcStatus->task.interpState ==  EMC_TASK_INTERP_PAUSED )
        {
            sendProgramResume();
        }       
        return;
    }
    
    if( isprefix( "HOLD" ,NULL ) )
    {
        sendProgramPause();
        return;
    }
    
}

void auto_draw()
{
    draw_statusbar( "AUTO" );
    println(  emcStatus->task.file ,5,40 ,15);
   
    if( ! lines.empty() )
    {
        for( int i = emcStatus->task.motionLine-1; i < emcStatus->task.motionLine-1 + 10; i++)
        {
            if( i < 0 || i >=  (int)lines.size() )
            {
                break;
            }
            println(  lines[ i ].c_str() );
        }
    }

    draw_dro();
    
    preview_draw();
    
    
}
