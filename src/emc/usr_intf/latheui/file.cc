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
int line;
int selected;

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

void open_dir( const char* s )
{
    printf("open dir:%s\n",s);
    
    line = 1;
    selected = -1;

    strcpy( currentdir, s );
    nfiles = scandir ( currentdir, &eps, one, alphasort);
    if (nfiles >= 0)
    {
        
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
    if( isprefix( "JG+" ,NULL ) )
    {
        line++;
    }

    if( isprefix( "JG-" ,NULL ) )
    {
        line--;
    }
    if( isprefix( "RET" ,NULL ) && selected >= 0 )
    {
        
        char path[PATH_MAX];
        char rpath[PATH_MAX];
        snprintf( path, PATH_MAX, "%s/%s", currentdir, eps[selected]->d_name );
        if( realpath( path, rpath ) == NULL )
        {
            printf("realpath() fails\n");
            return;
        }
        if( eps[selected]->d_type == DT_DIR )
        {
            open_dir( rpath );
        }
        else
        {
            load( rpath );
        }
    }

}


void draw_directory()
{
    if( line < 0 ) line = 0;
    if( line > nfiles-1 ) line = nfiles-1;
    selected = -1;
    
    for (int cnt = 0; cnt < nfiles; ++cnt)
    {
        int y = cnt-line;
        
        if( y == 0 ) selected = cnt;
        
        if( y >= 0 && y < 25 )
        {
            if( eps[cnt]->d_type == DT_DIR ) print( eps[cnt]->d_name ,40, 60+20*y, 20, BLUE );
                                        else print( eps[cnt]->d_name ,40, 60+20*y, 20, GREEN );
        }
    }
    print( "->" ,10, 60, 20);
    print( currentdir ,5, 40, 15);

}

void file_draw()
{
 
    draw_statusbar( "FILE" );
    draw_directory();

}
