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

static menu Menu;
static double diameter = 0,z = 0;
static int oldaxis; 
static bool jogcont;

static void createmenu()
{
    Menu.clear();
    if( status.axis == AXISX )
    {
        Menu.begin("X axis");
            Menu.edit( &diameter, "Set Diameter:" );
        Menu.end();
    }
    if( status.axis == AXISZ )
    {
        Menu.begin("Z axis");
            Menu.edit( &z, "Set Z:" );
        Menu.end();
    }    
}


void manual_init()
{   
    jogcont = false;
    sendAbort();
    sendManual();
    emcCommandWaitDone();
    createmenu();
    oldaxis = status.axis;
}

void manual_parse_serialdata()
{ 
    
    if( jogcont )
    {
        jogcont = false;
        sendJogStop( AXISX );
        sendJogStop( AXISZ );
        printf("stop\n");
        return;
    }
    else
    {
       if( isprefix( "LEFT" ,NULL ) )
       {
           sendJogCont( AXISZ, -2000 );
           printf("left\n");
           jogcont = true;
           return;
       } 
       if( isprefix( "RIGHT" ,NULL ) )
       {
           sendJogCont( AXISZ, 2000 );
           jogcont = true;
           return;
       }         
       if( isprefix( "UP" ,NULL ) )
       {
           sendJogCont( AXISX, -2000 );
           printf("left\n");
           jogcont = true;
           return;
       } 
       if( isprefix( "DOWN" ,NULL ) )
       {
           sendJogCont( AXISX, 2000 );
           jogcont = true;
           return;
       }            
    }
     
    
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
    
    if( oldaxis != status.axis )
    {
        oldaxis = status.axis;
        createmenu();
    }
    
    if( Menu.parse() && isprefix( "RET" ,NULL ) )
    {
        if( Menu.edited( &diameter ) )
        {
            sprintf(strbuf,"G10 L20 P1 X%3.2f", diameter/2.0f );
            mdicommand( strbuf );
            sendManual();
            emcCommandWaitDone();           
        }
        else if( Menu.edited( &z ) )
        {
            sprintf(strbuf,"G10 L20 P1 Z%3.2f", z );
            mdicommand( strbuf );
            sendManual();
            emcCommandWaitDone();
        }        
    }

}

void manual_draw()
{
    
    draw_statusbar( "MANUAL" );
    
    sprintf(strbuf, "INCR     %4.3f mm\nFEEDRATE %d mm/min",status.incr, status.jogfeedrate );
    print( strbuf ,0,60,25);
    preview_draw();
    draw_dro();
    Menu.draw( 5, 120 );

}
