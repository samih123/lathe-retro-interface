/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

extern char strbuf[BUFFSIZE];
extern struct machinestatus status;

extern char ttfile[LINELEN];
extern char *ttcomments[CANON_POCKETS_MAX];
extern int screenw, screenh;
int currentline = 0;
int tool = 0;
int Arg = 0;
char estr[BUFFSIZE];
extern char buf[BUFFSIZE];
extern list<string> errors;
int mode;
double offsetX,offsetZ;
static menu Menu;
static int menuselect;
bool show_messages;

list<string> updatelog;

#define MODE_SELECT_TOOL 1
#define MODE_SET_TOOL 2

static int saveToolTable(const char *filename,
             CANON_TOOL_TABLE toolTable[])
{
    int pocket;
    FILE *fp;
    const char *name;
    int start_pocket;

    // check filename
    if (filename[0] == 0) {
    name = tool_table_file;
    } else {
    // point to name provided
    name = filename;
    }

    // open tool table file
    if (NULL == (fp = fopen(name, "w"))) {
    // can't open file
    return -1;
    }

    start_pocket = 1;

    for (pocket = start_pocket; pocket < CANON_POCKETS_MAX; pocket++) {
        if (toolTable[pocket].toolno != -1) {

            fprintf(fp, "T%d P%d", toolTable[pocket].toolno, pocket);

            if (toolTable[pocket].diameter) fprintf(fp, " D%f", toolTable[pocket].diameter);
            if (toolTable[pocket].offset.tran.x) fprintf(fp, " X%+f", toolTable[pocket].offset.tran.x);
            if (toolTable[pocket].offset.tran.y) fprintf(fp, " Y%+f", toolTable[pocket].offset.tran.y);
            if (toolTable[pocket].offset.tran.z) fprintf(fp, " Z%+f", toolTable[pocket].offset.tran.z);
            if (toolTable[pocket].offset.a) fprintf(fp, " A%+f", toolTable[pocket].offset.a);
            if (toolTable[pocket].offset.b) fprintf(fp, " B%+f", toolTable[pocket].offset.b);
            if (toolTable[pocket].offset.c) fprintf(fp, " C%+f", toolTable[pocket].offset.c);
            if (toolTable[pocket].offset.u) fprintf(fp, " U%+f", toolTable[pocket].offset.u);
            if (toolTable[pocket].offset.v) fprintf(fp, " V%+f", toolTable[pocket].offset.v);
            if (toolTable[pocket].offset.w) fprintf(fp, " W%+f", toolTable[pocket].offset.w);
            if (toolTable[pocket].frontangle) fprintf(fp, " I%+f", toolTable[pocket].frontangle);
            if (toolTable[pocket].backangle) fprintf(fp, " J%+f", toolTable[pocket].backangle);
            if (toolTable[pocket].orientation) fprintf(fp, " Q%d", toolTable[pocket].orientation);
            fprintf(fp, " ;%s\n", ttcomments[pocket]);
        }
    }

    fclose(fp);
    return 0;
}

#define MENU_SHUTDOWN -666
#define MENU_UPDATE -2
#define MENU_BACK      -1

static void createmenu()
{
    Menu.clear();
    
    if( mode == MODE_SELECT_TOOL )
    {
        Menu.begin("Settings:");
        
            Menu.begin("Power Off");
                Menu.back("Chancel");
                Menu.select( &menuselect, MENU_SHUTDOWN, "Ok" );
            Menu.end(); 
            
            Menu.begin("Update from github");
                Menu.back("Chancel");
                Menu.select( &menuselect, MENU_UPDATE, "Ok" );
            Menu.end(); 
                        
            Menu.begin("System messages");
                Menu.back("back");
                for(list<string>::iterator i = errors.begin(); i != errors.end(); i++)
                {
                    Menu.show( i->c_str() );
                }
            Menu.end(); 

            for( int i=1; i <= MAXTOOLS; i++ )
            {
                sprintf(strbuf,"Tool %i %s", i,ttcomments[i]);
                Menu.select( &menuselect, i, strbuf );
            }     
                   
        Menu.end();
    }
    else if( mode == MODE_SET_TOOL )
    {
        sprintf(strbuf,"Tool %i", tool);
        Menu.begin( strbuf );
            Menu.select( &menuselect, MENU_BACK, "Back" );
            Menu.edit( ttcomments[tool], "Name:" );
            Menu.edit( &_tools[ tool ].diameter, "Diameter " );
            Menu.edit( &offsetX, "set X offset " );
            Menu.edit( &offsetZ, "set Z offset " );
            Menu.edit( &_tools[ tool ].frontangle, "Frontangle " );
            Menu.edit( &_tools[ tool ].backangle, "Backangle " );
            Menu.edit( &_tools[ tool ].orientation, "Orientation " );
        Menu.end();
    }        
}

void set_init()
{
    loadToolTable( ttfile, _tools, 0, ttcomments, 0);
    currentline = 0;
    tool = currentline;
    mode = MODE_SELECT_TOOL;
    offsetX = 0;
    offsetZ = 0;
    createmenu();
}



void mdicommand( const char *c )
{
    printf("mdi command:%s\n",c);
    sendMdi();
    emcCommandWaitDone();
    sendMdiCmd( c );
    emcCommandWaitDone();    
}

int shutdown()
{
    sendMachineOff();
    emcCommandWaitDone();
    return system("shutdown -P now");
}

void update()
{
    
    const char *commands =
    "cd /home/sami/retro-lathe-interface\n\
    ls\n\
    git pull\n\
    cd src\n\
    make\n";
    FILE *pp;
    
    pp = popen( commands, "r");
    
    if (pp != NULL)
    {
        for(;;)
        {
            char *line;
            char buf[1000];
            line = fgets(buf, sizeof buf, pp);
            if (line == NULL) break;
            
            updatelog.push_back( line );
            if( updatelog.size() > 40 ) updatelog.pop_front();
            
            glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
            glClear( GL_COLOR_BUFFER_BIT );
            
            println( "", 0, 0, 12, GREEN );
            for(std::list<string>::iterator i = updatelog.begin(); i!= updatelog.end(); i++)
            {
                println( i->c_str() );
                printf("%s",i->c_str());
            }
            
            glutSwapBuffers();
        }
        pclose(pp);
        sleep( 5 );
    }

    
}

void set_parse_serialdata()
{
    show_messages = false;
    if( Menu.parse() )
    {
        
        if( menuselect == MENU_SHUTDOWN )
        {
            shutdown();
        }
        
        if( menuselect == MENU_UPDATE )
        {
            update();
        }        
        
        if( mode == MODE_SELECT_TOOL )
        {
        
            if( Menu.edited( &menuselect ) && menuselect > 0 )
            {
                tool = menuselect;
                mode = MODE_SET_TOOL;
                createmenu();
                sprintf(strbuf,"T%i M6 G43", tool );
                mdicommand( strbuf );
            }       
        
        }
        else
        {
            
            if( _tools[ tool ].diameter < 0) _tools[ tool ].diameter = 0;
            if( _tools[ tool ].orientation > 9) _tools[ tool ].orientation = 1; // wrap
            if( _tools[ tool ].orientation < 1) _tools[ tool ].orientation = 9;
        
            else if( Menu.edited( &menuselect ) && menuselect == MENU_BACK )
            {
                mode = MODE_SELECT_TOOL;
                createmenu(); 
                saveToolTable(ttfile,_tools);
                sendLoadToolTable(ttfile);
            } 
            
            if(  Menu.edited( &offsetX ) )
            {
                sprintf(strbuf,"G10 L10 P%i X%f I%f J%f Q%i", 
                    tool,
                    offsetX,
                    _tools[ tool ].frontangle,
                    _tools[ tool ].backangle,
                    _tools[ tool ].orientation
                     );
                mdicommand( strbuf );
                mdicommand( "G43" );    
                sendLoadToolTable(ttfile);
                loadToolTable(ttfile, _tools, 0, ttcomments, 0);
            }  

            else if(  Menu.edited( &offsetZ ) )
            {
                sprintf(strbuf,"G10 L10 P%i Z%f I%f J%f Q%i", 
                    tool,
                    offsetZ,
                    _tools[ tool ].frontangle,
                    _tools[ tool ].backangle,
                    _tools[ tool ].orientation
                     );                
                mdicommand( strbuf );
                mdicommand( "G43" );
                sendLoadToolTable(ttfile);
                loadToolTable(ttfile, _tools, 0, ttcomments, 0);

            } 
            
        }
    }

}

void set_draw()
{
    
    Menu.draw( 5, 50 );
    
    draw_statusbar( "SET" );
    if( ! Menu.current_menu("System messages") )
    {
        draw_dro();
        glPushMatrix();
        glTranslatef(600,150 , 0);
        glScalef(5,5,5);
        draw_tool( tool );
        glPopMatrix();
    }

}




