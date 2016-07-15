/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"
#include "motion_types.h"
#include "motion.h"
extern char strbuf[BUFFSIZE];
extern struct machinestatus status;
extern int screenw, screenh;

static menu Menu;
static double diameter = 0,z = 0;
static int oldaxis; 
static bool jogcont;
static bool jogcont_stopped;
static vec2 pos;

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


void reset_position()
{
    pos.x = emcStatus->motion.traj.position.tran.x;
    pos.z = emcStatus->motion.traj.position.tran.z;  
}

void manual_init()
{   
    jogcont = false;
    jogcont_stopped = false;
    sendAbort();
    sendManual();
    emcCommandWaitDone();
    createmenu();
    oldaxis = status.axis;
    reset_position();
}

void manual_parse_serialdata()
{ 
    // rapid jogging with buttons
    if( jogcont_stopped && emcStatus->motion.traj.current_vel == 0 )
    {
        reset_position();
        jogcont_stopped = false;
    }
    
    if( jogcont && ( isprefix( "RLKB" ,NULL ) || isprefix( "RLUP" ,NULL )) )
    {
        jogcont = false;
        jogcont_stopped = true;
        sendJogStop( AXISX );
        sendJogStop( AXISZ );
        return;
    }
    else
    {
       if( isprefix( "LEFT" ,NULL ) )
       {
           sendJogCont( AXISZ, -2000 );
           jogcont = true;
           jogcont_stopped = false;
           return;
       } 
       if( isprefix( "RIGHT" ,NULL ) )
       {
           sendJogCont( AXISZ, 2000 );
           jogcont = true;
           jogcont_stopped = false;
           return;
       }         
       if( isprefix( "UP" ,NULL ) )
       {
           sendJogCont( AXISX, -2000 );
           jogcont = true;
           jogcont_stopped = false;
           return;
       } 
       if( isprefix( "DOWN" ,NULL ) )
       {
           sendJogCont( AXISX, 2000 );
           jogcont = true;
           jogcont_stopped = false;
           return;
       }            
    }
     
    // jogging wheel
    if( status.jogged != 0 )
    {
        
        if( status.axis == AXISX )
        {
            pos.x += status.jogged;
            sendJogAbs(status.axis, status.jogfeedrate, pos.x );
        }
        else if( status.axis == AXISZ )
        {
            pos.z += status.jogged;
            sendJogAbs(status.axis, status.jogfeedrate, pos.z );
        }
         
        return;
    }
    
    // new menu if axis is changed.
    if( oldaxis != status.axis )
    {
        oldaxis = status.axis;
        createmenu();
    }
    
    
    // set work zero.
    if( Menu.parse() && isprefix( "RET" ,NULL ) )
    {
        if( Menu.edited( &diameter ) )
        {
            sprintf(strbuf,"G10 L20 P1 X%3.3f", diameter/2.0f );
            mdicommand( strbuf );
            sendManual();
            emcCommandWaitDone(); 
        }
        else if( Menu.edited( &z ) )
        {
            sprintf(strbuf,"G10 L20 P1 Z%3.3f", z );
            mdicommand( strbuf );
            sendManual();
            emcCommandWaitDone();
        }
        reset_position();        
    }

}

void manual_draw()
{
    
    if( jogcont_stopped && emcStatus->motion.traj.current_vel == 0 )
    {
        reset_position();
        jogcont_stopped = false;
    }
    
    draw_statusbar( "MANUAL" );
    
    sprintf(strbuf, "INCR     %4.3f mm\nFEEDRATE %d mm/min",status.incr, status.jogfeedrate );
    print( strbuf ,0,60,25);
    preview_draw();
    draw_dro( &pos );
    Menu.draw( 5, 120 );

}
