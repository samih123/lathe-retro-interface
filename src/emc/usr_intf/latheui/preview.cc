/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/


#include "latheintf.h"
#include <list>
using namespace std;

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "rs274ngc_return.hh"
#include "inifile.hh"		// INIFILE
#include "canon.hh"		// _parameter_file_name
#include "config.h"		// LINELEN
#include "tool_parse.h"
#include <stdio.h>    /* gets, etc. */
#include <stdlib.h>   /* exit       */
#include <string.h>   /* strcpy     */
#include <getopt.h>
#include <stdarg.h>
#include <string>


InterpBase *pinterp;
#define interp_new (*pinterp)


#define active_settings  interp_new.active_settings
#define active_g_codes   interp_new.active_g_codes
#define active_m_codes   interp_new.active_m_codes
#define error_text	 interp_new.error_text
#define interp_execute	 interp_new.execute
#define file_name	 interp_new.file_name
#define interp_init	 interp_new.init
#define stack_name	 interp_new.stack_name
#define line_text	 interp_new.line_text
#define line_length	 interp_new.line_length
#define sequence_number  interp_new.sequence_number
#define interp_close     interp_new.close
#define interp_exit      interp_new.exit
#define interp_open      interp_new.open
#define interp_read	 interp_new.read
#define interp_load_tool_table interp_new.load_tool_table
#define interp_set_loglevel interp_new.set_loglevel
#define interp_task_init interp_new.task_init


extern char ttfile[LINELEN];
static float xmax,ymax,xmin,ymin;

struct line
{
    line( float ax, float ay, float ax2, float ay2 )
    {
        x = ax;
        y = ay;
        x2 = ax2;
        y2 = ay2;  
        if( xmax < x ) xmax = x;
        if( ymax < y ) ymax = y; 
        if( xmin > x ) xmin = x;
        if( ymin > y ) ymin = y;                  
    } 
    
    float x,y,x2,y2;
    int type;
    
}; 

static std::list<line> feedlines;
static std::list<line> rapidlines;

void preview_addfeed( float x, float y, float x2, float y2)
{
    feedlines.push_back( line(x,y,x2,y2) );
}

void preview_addrapid( float x, float y, float x2, float y2)
{
    rapidlines.push_back( line(x,y,x2,y2) );
}


void preview_draw()
{
    float scale,scale2;
    
    float sizex = 500;
    float sizey = 300;
    
    float x = emcStatus->motion.traj.actualPosition.tran.z;
    float y = -emcStatus->motion.traj.actualPosition.tran.x;
    x -= emcStatus->task.g5x_offset.tran.z;
    y += emcStatus->task.g5x_offset.tran.x;
    x -= emcStatus->task.g92_offset.tran.z;
    y += emcStatus->task.g92_offset.tran.x;
    x -= emcStatus->task.toolOffset.tran.z;
    y += emcStatus->task.toolOffset.tran.x;
    
    float xmax2 = xmax;
    float xmin2 = xmin;
    float ymax2 = ymax;
    float ymin2 = ymin;
    float s=1.0f; 
    if( xmax2 < x+s ) xmax2 = x+s;
    if( ymax2 < y+s ) ymax2 = y+s;
    if( xmin2 > x-s ) xmin2 = x-s;
    if( ymin2 > y-s ) ymin2 = y-s;   
    //printf( "min %f,%f max %f,%f\n",xmin,ymin,xmax,ymax);
    scale = sizex / (xmax2 - xmin2);
    scale2 = sizey / (ymax2 - ymin2);
    
    if( scale >  scale2 ) scale = scale2;
    
    glPushMatrix();
    
    glTranslatef(5+ -xmin2*scale,300 , 0);
    glScalef(scale, scale,scale);
        
    
    setcolor( OUTLINE );
    glBegin(GL_LINE_LOOP);
        glVertex2f( xmin, ymin ); 
        glVertex2f( xmin, ymax ); 
        glVertex2f( xmax, ymax );
        glVertex2f( xmax, ymin );
    glEnd();    
        
    setcolor( RAPID );
    glBegin(GL_LINES);
    for(std::list<line>::iterator i = rapidlines.begin(); i != rapidlines.end(); i++)
    {
        glVertex2f( i->x, i->y );
        glVertex2f( i->x2, i->y2 );
    }
    glEnd();
    
    setcolor( FEED );
    glBegin(GL_LINES);
    for(std::list<line>::iterator i = feedlines.begin(); i != feedlines.end(); i++)
    {
        glVertex2f( i->x, i->y );
        glVertex2f( i->x2, i->y2 );
    }
    glEnd();
    
    setcolor( CROSS );
    glBegin(GL_LINES);
        glVertex2f( 0,15 ); 
        glVertex2f( 0,-15 ); 
        glVertex2f( 15,0 );
        glVertex2f( -15,0 );
    glEnd();
    
    glTranslatef( x, y , 0);
    draw_tool( emcStatus->io.tool.toolInSpindle );

    glPopMatrix();
 
}





void report_error( int error_code )
{
    char interp_error_text_buf[LINELEN];

    error_text(error_code, interp_error_text_buf, 5); /* for coverage of code */
    error_text(error_code, interp_error_text_buf, LINELEN);
    printf("%s\n",
            ((interp_error_text_buf[0] == 0) ? "Unknown error, bad error code" : interp_error_text_buf));
    line_text(interp_error_text_buf, LINELEN);
    printf("%s\n", interp_error_text_buf);

}



int interpret_from_file( int do_next, int block_delete)       
{
    int status=0;

    SET_BLOCK_DELETE(block_delete);

    for(; ;)
    {
        status = interp_read();
        
        if ((status == INTERP_EXECUTE_FINISH) && (block_delete == ON))
            continue;
            
        else if (status == INTERP_ENDFILE)
            break;
            
        if ((status != INTERP_OK) &&  (status != INTERP_EXECUTE_FINISH))
        {
            report_error(status);
            status = 1;
            break;
        }
        
        status = interp_execute();
        if ((status != INTERP_OK) &&
                (status != INTERP_EXIT) &&
                (status != INTERP_EXECUTE_FINISH))
        {
            report_error(status);
            status = 1;
            break;
        }
        else if (status == INTERP_EXIT)
            return 0;
            
    }
    return ((status == 1) ? 1 : 0);
}





extern int _task ; // control preview behaviour when remapping

int preview( char *file )
{
    
    /*
    int res = INTERP_OK;
    int active_g_codes[ACTIVE_G_CODES];
    int active_m_codes[ACTIVE_M_CODES];
    double active_settings[ACTIVE_SETTINGS];
    InterpBase interp;
    interp.init();
    interp.open("a.ngc");
    interp.read("G80 G17 G40 G21 G90 G94 G54 G49 G99 G64 G0 G97 G91.1 G8 M5 M9 M48 M53 M0 F0 S0");
    while ((res = interp.read()) == INTERP_OK || res == INTERP_EXECUTE_FINISH) {
        res = interp.execute();
        if (res != INTERP_OK) break;
    }
    interp.active_g_codes(active_g_codes);
    interp.active_m_codes(active_m_codes);
    interp.active_settings(active_settings);
    
    return;
    */
    
    int status;
    int do_next; /* 0=continue, 1=mdi, 2=stop */
    int block_delete;
    char buffer[80];
  //  int tool_flag;
    int gees[ACTIVE_G_CODES];
    int ems[ACTIVE_M_CODES];
    double sets[ACTIVE_SETTINGS];
  //  int print_stack;
  //  int go_flag;
    std::string interp;

    do_next = 2;  /* 2=stop */
    block_delete = OFF;
  //  print_stack = OFF;
//    tool_flag = 0;
    _outfile = stdout; 
//    go_flag = 0;

    if(!pinterp) pinterp = new Interp;

    if ((status = interp_init()) != INTERP_OK)
    {
        report_error(status);
        return status;
    }

    feedlines.clear();
    rapidlines.clear();
    xmax = ymax = -10000000.0f;
    xmin = ymin = 10000000.0f;
      
    loadToolTable( ttfile, _tools, 0, 0, 0);
    
    if( file == NULL ) return 0;
    
    status = interp_open( file );
    
    if (status != INTERP_OK) /* do not need to close since not open */
    {
        report_error(status);
        return status;
    }
    
    status = interpret_from_file(do_next, block_delete );
    
    file_name(buffer, 5);  /* called to exercise the function */
    file_name(buffer, 79); /* called to exercise the function */
    interp_close();

    line_length();         /* called to exercise the function */
    sequence_number();     /* called to exercise the function */
    active_g_codes(gees);  /* called to exercise the function */
    active_m_codes(ems);   /* called to exercise the function */
    active_settings(sets); /* called to exercise the function */
    interp_exit(); /* saves parameters */
    
    return status;
}




