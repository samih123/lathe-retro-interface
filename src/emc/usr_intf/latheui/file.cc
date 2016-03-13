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
extern char programPrefix[LINELEN];

char currentdir[PATH_MAX] = {0};

static menu Menu;
static int menuselect;

struct dirent **eps;
int nfiles;
    
static int
one (const struct dirent *unused)
{
  return 1;
}

void load( char* n )
{
    
    if( strstr( n, ".wiz" ) != NULL )
    {
        wizards_load( n );
        return;
    }
    
    if( strstr( n, ".ngc" ) != NULL )
    {    
        edit_load( n );
        auto_load( n );
    }
    
}

void save( char* s )
{
    
}

static void createmenu( const char* s )
{
    Menu.clear();

    Menu.begin( s );
    
    for (int cnt = 1; cnt < nfiles; ++cnt)
    {
        if( strcmp( "..", eps[cnt]->d_name ) == 0 )
        {
            Menu.select( &menuselect, cnt , "Back" );
        }
        else if( eps[cnt]->d_name[0] != '.' )
        {
            Menu.select( &menuselect, cnt , eps[cnt]->d_name );
            if( eps[cnt]->d_type == DT_DIR )
            {
                Menu.color( BLUE );
            }
        }
    }
            
    Menu.end();
    
}

void open_dir( const char* s )
{
    printf("open dir:%s\n",s);
    strcpy( currentdir, s );
    nfiles = scandir ( currentdir, &eps, one, alphasort);
    if (nfiles >= 0)
    {
        createmenu( s );
    }
    else printf( "Couldn't open the directory %s\n",currentdir);
}



void file_init()
{
    if( currentdir[0] == 0 ) open_dir( programPrefix );
    sendAbort();
    emcCommandWaitDone();
}


void file_parse_serialdata()
{
    menuselect = -1;
    if( Menu.parse() )
    {
        if( menuselect > 0 )
        {
            char path[PATH_MAX];
            char rpath[PATH_MAX];
            snprintf( path, PATH_MAX, "%s/%s", currentdir, eps[ menuselect ]->d_name );
            if( realpath( path, rpath ) == NULL )
            {
                printf("realpath() fails\n");
                return;
            }
            if( eps[ menuselect ]->d_type == DT_DIR )
            {
                open_dir( rpath );
            }
            else
            {
                load( rpath );
            }
        }
    }

}


    
void file_draw()
{
 
    draw_statusbar( "FILE" );
    Menu.draw( 5, 50 );
}
