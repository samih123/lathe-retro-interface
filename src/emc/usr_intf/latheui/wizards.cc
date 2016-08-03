/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"
//#include "menu.h"

#include <list>
using namespace std;

extern char programPrefix[LINELEN];
extern char strbuf[BUFFSIZE];
extern struct machinestatus status;
extern int screenw, screenh;
extern char estr[BUFFSIZE];
static menu Menu;

static list<new_operation *> opl;
static list<ftag> tagl;

list<new_operation *>::iterator cur_op;
list<new_operation *>::iterator cur_contour;
list<new_operation *>::iterator cur_tool;

static char Name[BUFFSIZE];

static char initcommands[BUFFSIZE] = "";

static int operationcreate;
static int operationselect;
static int menuselect;
static bool main_menu;

static vec2 start_position;
int maxrpm;

double stockdiameter;
double scale = 1;
static vec2 pos(0,0); 
double retract = 1;
bool draw_wiz;

#define  MENU_PHASE_NEW 1
#define  MENU_PHASE_DELETE 2
#define  MENU_PHASE_UP 3
#define  MENU_PHASE_DOWN 4

#define  MENU_SAVE 7
#define  MENU_MAIN 8
#define  MENU_SAVE_PROGRAM 9


void create_operation_select_menus()
{ 
    int n = 1;
    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        sprintf(strbuf,"  %d:%s%s", n, (*i)->type() > CONTOUR ? "  " : "", (*i)->name() );
        Menu.select( &operationselect, n, strbuf );
        if( cur_op == i )
        {
            Menu.setcolor( WARNING );
        }
        n++;
    }
}


void create_new_operation_menu( int type, const char *name )
{
    Menu.select( &operationcreate, type, name );
}

void create_main_menu()
{
    printf("create main menu\n");
    Menu.clear();
    menuselect = 0;
    Menu.begin( "" );
    
        Menu.edit( Name, "Program name:" );
        
        Menu.coordinate(  &pos.x, &pos.z, &scale, "Display pos " );
        
        Menu.edit( &start_position.x, "start diameter   " ); Menu.diameter_mode();
        Menu.edit( &start_position.z, "start position Z " );
        
        Menu.edit( initcommands, "Init commands:" );
        Menu.select( &menuselect, MENU_SAVE, "Save" );
        Menu.select( &menuselect, MENU_SAVE_PROGRAM, "Save program" );
        Menu.edit( &stockdiameter, "Stock diameter " );
        Menu.edit( &maxrpm, "Max spindle rpm " );
        
        Menu.begin( "Create and delete operations" );
            Menu.back("Back");
            
            if( cur_op != opl.end() )
            {
                Menu.select( &menuselect, MENU_PHASE_UP, "Move current operation up" );
                Menu.select( &menuselect, MENU_PHASE_DOWN, "Move current operation down" );
                sprintf(strbuf,"Delete current operation:%s", (*cur_op)->name() );
                Menu.select( &menuselect, MENU_PHASE_DELETE, strbuf );
            }
            
            Menu.begin( "Create new operation" );
                Menu.back("Back");
                
                create_new_operation_menu( TOOL, "Tool" );
                create_new_operation_menu( RECTANGLE, "Rectangle" );
                create_new_operation_menu( THREADING, "Threading" );
                create_new_operation_menu( RAPIDMOVE, "Rapid move" );
                
                /*
                create_new_operation_menu( CONTOUR "");
                create_new_operation_menu( INSIDE_CONTOUR );
                create_new_operation_menu( TURN );
                create_new_operation_menu( UNDERCUT );
                create_new_operation_menu( FINISHING );
                create_new_operation_menu( THREADING );
                create_new_operation_menu( RECTANGLE );
                create_new_operation_menu( DRILL );
                create_new_operation_menu( PARTING );
                create_new_operation_menu( RAPIDMOVE );
                */
            Menu.end();
            
        Menu.end();
        
        create_operation_select_menus();

    Menu.end();
    Menu.setmaxlines( 15 );
    
    main_menu = true;
}


void clear_all_operations()
{
    cur_contour = cur_tool = opl.end();

    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        if( (*i)->type() == CONTOUR )
        {
            cur_contour = i;
        }

        if( (*i)->type() == TOOL )
        {
            cur_tool = i;
        }
        
        if( cur_tool != opl.end() )
        {
            (*i)->set_tool( (op_tool*)*cur_tool );
        }
        else
        {
            (*i)->set_tool( NULL );
        }
        
        if( cur_contour != opl.end() )
        {
            (*i)->set_contour( (op_contour*)*cur_contour );
        }
        else
        {
            (*i)->set_contour( NULL );
        }       
        
        (*i)->update();
    }
}



void save_program(const char *name )
{

    FILE *fp;
    char ngc_file[BUFFSIZE];
    
    sprintf( ngc_file, "%s/%s.ngc", programPrefix, name );
    printf("save_program:%s\n",ngc_file);
    fp = fopen( ngc_file, "w");

    if (fp == NULL) return;

    fprintf(fp, "%s\n", initcommands );
    
    
    fprintf(fp, "G0 X%.10g Z%.10g\n", start_position.x, start_position.z );
    
    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        (*i)->save_program( fp ); // must call in order! 
        
        if( (*i)->type() != CONTOUR &&
            (*i)->type() != INSIDE_CONTOUR
        )
        {
            
            bool b = true;
            if( std::next(i) != opl.end() )
            {
                if( (*std::next(i))->type() == RAPIDMOVE ) 
                {
                    b = false;
                }
            }
            
            if(b)
            {
                fprintf(fp, "G0 X%.10g Z%.10g (start position)\n", start_position.x, start_position.z );
            }
        }
        
    }
        
    fprintf(fp, "M5 (stop spindle)\n");
    fprintf(fp, "M2\n");
    
    fclose( fp );

    edit_load( ngc_file );
    auto_load( ngc_file );
    draw_wiz = true;
}


void wizards_save( const char *name )
{
    FILE *fp;

    
    sprintf(strbuf,"%s/%s.wiz", programPrefix, name );
    printf("save:%s\n",strbuf);
    fp = fopen( strbuf, "w");
    if (fp == NULL) return;
    
    savetagl( fp, tagl );
    
    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        (*i)->save( fp );
    }

    fclose( fp );
}


void wizards_load( const char *name )
{

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    char tag[BUFFSIZE+1];
    double val=0;
    
    printf("load:%s\n",name);
    fp = fopen( name, "r");

   if (fp == NULL) return;
    
    opl.clear();
    wizards_init();

    loadtagl( fp, tagl );


    while ((read = getline( &line, &len, fp)) != -1)
    {
        printf("%s", line);
        val = 0;
        sscanf(line, "%s %lf", tag, &val );
        if( strcmp( tag, "OPERATION" ) == 0 )
        {
            if( val == TOOL )
            {
                opl.push_back( new op_tool() );
                opl.back()->load( fp );
            }
            else if( val == RECTANGLE )
            {
                opl.push_back( new op_rectangle() );
                opl.back()->load( fp );
            }
            else if( val == THREADING )
            {
                opl.push_back( new op_threading() );
                opl.back()->load( fp );
            }
        }

        free(line);
        line = NULL;
    }

    fclose( fp );


    // strip name
    const char *n = name + strlen(name);
    while( *n != '/' && n > name ) n--;
    strcpy( Name, n+1 );
    n = strstr(Name, ".wiz" );
    if( n ) *(char *)n = 0;
  
    clear_all_operations();

}


void wizards_init()
{
    tagl.clear();
    tagl.push_front( ftag( "STOCKDIAM", &stockdiameter ));
    tagl.push_front( ftag( "MAXRPM", &maxrpm ));
    tagl.push_front( ftag( "INIT", initcommands ));
    tagl.push_front( ftag( "POSX", &pos.x ));
    tagl.push_front( ftag( "POSZ", &pos.z ));
    tagl.push_front( ftag( "SCALE", &scale ));
    tagl.push_front( ftag( "NAME", Name ));
    tagl.push_front( ftag( "START_X", &start_position.x ));
    tagl.push_front( ftag( "START_Z", &start_position.z ));
    
        
    if( opl.size() == 0 )
    {
        scale = 2;
        retract = 1;
        maxrpm = status.maxrpm;
        stockdiameter = 20;
        start_position.x = stockdiameter/2.0;
        start_position.z = 20;
        cur_contour = cur_op = cur_tool= opl.end();
        strcpy( initcommands, "G18 G8 G21 G95 G40 G64 P0.01 Q0.01" );
        draw_wiz = false;
    }

    if( status.screenmode == SCREENWIZARDS )
    {
        create_main_menu();
    }
}


void clamp_values()
{
     CLAMP( stockdiameter,1,1000 );
     CLAMP( maxrpm, 1, 5000 );
     CLAMP( scale,0.1,10 ); 
}

void wizards_parse_serialdata()
{

    if( main_menu )
    {
        menuselect = 0;
        if( Menu.parse() )
        {
            clamp_values();
    
            if( menuselect == MENU_MAIN )
            {
                create_main_menu();
                return;
            }
            
            if( menuselect == MENU_SAVE_PROGRAM )
            {
                save_program( Name );
                return;
            }
            
            if( menuselect == MENU_SAVE )
            {
                wizards_save( Name );
                return;
            } 
                          
            if( menuselect == MENU_PHASE_DELETE && cur_op !=  opl.end() )
            {
                opl.erase( cur_op );
                cur_contour = cur_op = cur_tool = opl.end();
                clear_all_operations();
                create_main_menu();
                return;
            }
    
    
            if( menuselect == MENU_PHASE_UP && cur_op !=  opl.end() && cur_op != opl.begin() )
            {
                opl.splice( std::prev(cur_op), opl, cur_op );
                clear_all_operations();
                create_main_menu();
                return;
            }
                    
            if( menuselect == MENU_PHASE_DOWN && cur_op !=  opl.end() && cur_op != --opl.end() )
            {
                opl.splice( std::next(cur_op,2), opl, cur_op );
                clear_all_operations();
                create_main_menu();
                return;
            }
                   
            if( Menu.edited( &operationcreate ) )
            {
                
                if( operationcreate == TOOL )
                {
                    opl.push_back( new op_tool() );
                }
                else if( operationcreate == RECTANGLE )
                {
                    opl.push_back( new op_rectangle() );
                }
                else if( operationcreate == THREADING )
                {
                    opl.push_back( new op_threading() );
                }
                                
                cur_op =  --opl.end();
                clear_all_operations();
                create_main_menu();
                return;
                
            }
    
            else if( Menu.edited( &operationselect ) )
            {
                int n = 1;
                cur_contour = cur_op = cur_tool = opl.end();
                for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
                {
                    if( n++ == operationselect )
                    {
                        cur_op = i;
                        create_main_menu();
                        break;
                    }
                }
                
                if( cur_op != opl.end() )
                {
                    //clear_all_operations();
                    main_menu = false;
                    (*cur_op)->createmenu();
                }
               
            }
    
        }
        
            
        
    }
    else
    {
        int a = (*cur_op)->parsemenu();
        if( a == OP_EXIT )
        {
            create_main_menu();
            main_menu = true;
            return;
        }
        
        if( a == OP_EDITED )
        {
            
            (*cur_op)->update();
            op_type t = (*cur_op)->type();
            
            if( t == TOOL || t == CONTOUR )
            {
                
                list<new_operation *>::iterator i = cur_op;
                for( i++; i != opl.end(); i++)
                {

                    if( (*i)->type() == t )
                    {
                        break;
                    }
                    
                    if( t == TOOL )
                    {
                        (*i)->set_tool( (op_tool*)*cur_op );
                        (*i)->update();
                    }
                    
                    else if( t == CONTOUR )
                    {
                        (*i)->set_contour( (op_contour*)*cur_op );
                        (*i)->update();
                    }                  
                }
            }
            
        }
    }

}

void show_tool( int x,int y, int t, const char* name )
{
    draw_tool( t );
    glPushMatrix();
    glTranslatef( 0,0 , 0);

    glTranslatef( x ,screenh-y , 0);
    glScalef(3.0f, 3.0f, 3.0f);

    draw_tool( t );
    glPopMatrix();
    println( name ,x-25,y+50,16);
}

void wizards_draw_outlines()
{
    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        (*i)->draw( DISABLED, false );
    }
}

void wizards_draw()
{
    draw_statusbar( "WIZARDS" );
    clamp_values();
    
    glLineStipple(1,0x27ff);
    glPushMatrix();
    glTranslatef( 750 ,300 , 0);
    glTranslatef( pos.z * scale ,pos.x * scale , 0);
    glScalef(scale*3.0f, scale*3.0f,scale*3.0f);
    
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
        setcolor( CENTERLINE );
        glVertex2f( 10,0 );
        glVertex2f( -600,0 );
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    for(list<new_operation *>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        (*i)->draw( i == cur_op ? NONE:DISABLED );
    }
    
    setcolor( RAPID );
    drawCross( start_position.z, -start_position.x, 5/scale);
    drawCircle( start_position.z, -start_position.x, 5/scale );
    
    glPopMatrix();
    
    if( main_menu )
    {
        Menu.draw(5,50);
    }
    else
    {
        (*cur_op)->drawmenu(5,50);
    }

//double angle = atan2( fabs( currentcut->start.x - currentcut->end.x) , fabs(currentcut->dim.z) )* 180.0f / M_PI;
  //  sprintf(strbuf,"Angle %g", angle );
    //println( strbuf );

}









