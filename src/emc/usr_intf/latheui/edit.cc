/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"
#include <list>
#include <string>
using namespace std;

extern struct machinestatus status;
extern int screenw, screenh;

static int currentline;
std::vector<string> lines;

void edit_init()
{
}

void edit_parse_serialdata()
{
    
    currentline += status.jogged_raw;
    if( currentline < 0 ) currentline = 0;
    if( currentline > (int)lines.size() ) currentline = lines.size();
    
}

void edit_draw()
{
    draw_statusbar( "EDITOR" );
    
    int sreenline = 0;
    int fileline = 0;
    
    for(std::vector<string>::iterator list_iter = lines.begin(); 
    list_iter != lines.end(); list_iter++)
    {
        //std::cout<<*list_iter<<endl;
        if( fileline >= currentline )
        {
            print( list_iter->c_str() ,5 ,41 + sreenline*21 ,20 );
            sreenline++;
            if(sreenline > 25) break;
        }
        fileline++;
    }
}


void edit_load( const char *file )
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    lines.clear();
    currentline = 0;

   fp = fopen( file, "r");
    if (fp == NULL) return;
    
   while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
        lines.push_back(line);
    }
     free(line);
    fclose( fp );
  
   
   
    for(std::vector<string>::iterator list_iter = lines.begin(); 
    list_iter != lines.end(); list_iter++)
    {
        std::cout<<*list_iter<<endl;
    }

}

void edit_save( const char *file )
{
    
}
