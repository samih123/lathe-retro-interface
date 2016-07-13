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


extern list<operation> *wiz_opl;
static list<operation> opl;
list<operation>::iterator cur_op;
list<operation>::iterator cur_contour;
list<operation>::iterator cur_tool;

static char Dstr[BUFFSIZE];
static char Zstr[BUFFSIZE];
static char Name[BUFFSIZE];
static char initcommands[BUFFSIZE] = "";

static int operationcreate;
static int operationselect;
static int menuselect;

static cut ccut;
static tool ctool;
static vec2 movepos;
static vec2 face_begin,face_end;

static vec2 start_position;

int maxrpm;
double stockdiameter;
double scale = 1;
static vec2 pos(0,0); 
double retract = 1;


#define  MENU_PHASE_NEW 1
#define  MENU_PHASE_DELETE 2
#define  MENU_PHASE_UP 3
#define  MENU_PHASE_DOWN 4

#define  MENU_NEWCUT    5
#define  MENU_DELETECUT 6

#define  MENU_SAVE 7
#define  MENU_MAIN 8
#define  MENU_SAVE_PROGRAM 9


void getzd()
{
    if( cur_contour != opl.end() ){
         ccut = cur_contour->get_cut();
    }
    
    if( cur_tool != opl.end() )
    {
        ctool = cur_tool->get_tool();
    }
    
    if( cur_op != opl.end() )
    {
        op_type type = cur_op->get_type();
        
        if( type == FACING )
        {
            face_begin = cur_op->getf_begin();
            face_end = cur_op->getf_end();
        }
        else if( type == MOVE )
        {
            movepos = cur_op->get_move();
        }
    }
}

static const char *typestr[] =
{
    "cut_Begin",
    "straight line",
    "Outside arch",
    "Inside arch",
    "Thread",
    "cut_end"
};

void create_operation_menu()
{
    getzd();
    if( cur_op == opl.end() )
    {
        return;
    }
    menuselect = 0;
    Menu.clear();
    op_type type = cur_op->get_type();
    
    Menu.begin( cur_op->get_name() );
    
        Menu.select( &menuselect, MENU_MAIN, "Back" );
        
        if( type == CONTOUR )
        {
            sprintf(Dstr,"D start:%.20g end:", ccut.start.x*2.0f );
            sprintf(Zstr,"Z start:%.20g lenght:", ccut.start.z );

            Menu.edit( &ccut.type, typestr[ ccut.type ] );Menu.hiddenvalue();
            Menu.select( &menuselect, MENU_DELETECUT, "Delete" );
            Menu.select( &menuselect, MENU_NEWCUT, "New" );
            Menu.edit( &ccut.end.x, Dstr ); Menu.shortcut("AX=0" ); Menu.diameter_mode();
            Menu.edit( &ccut.end.z, Zstr ); Menu.shortcut("AX=2" );

            
            if( ccut.type == CUT_ARC_IN || ccut.type == CUT_ARC_OUT )
            {
                Menu.edit( &ccut.r, "Radius " ); Menu.shortcut("AX=5" );
            }
            /*
            if( cur_contour->currentcut->type == THREAD )
            {
                Menu.edit( &cur_contour->currentcut->pitch, "Pitch" );
                Menu.edit( &cur_contour->currentcut->depth, "Depth " );
            }
            */
        }
        else if( type == TOOL )
        {
            Menu.edit( &ctool.tooln, "Tool number " );
            Menu.edit( &ctool.feed, "Feedrate mm/min " );
            Menu.edit( &ctool.speed, "Surface speed m/s " );
            Menu.edit( &ctool.depth, "Depth " );
        }
        
        else if( type == FACING )
        {
            Menu.edit( &face_begin.x, "Start diameter " ); Menu.diameter_mode();
            Menu.edit( &face_end.x, "End diameter " ); Menu.diameter_mode();
            Menu.edit( &face_begin.z, "Start Z " );
            Menu.edit( &face_end.z, "End Z " );           
        }
        
        else if( type == MOVE )
        {
            Menu.edit( &movepos.x, "Diameter " ); Menu.diameter_mode();
            Menu.edit( &movepos.z, "Z        " );
        }       
        
    Menu.end();
}

void create_operation_select_menus()
{ 
    int n = 1;
    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        sprintf(strbuf,"  %d:%s%s", n, i->get_type() > CONTOUR ? "  " : "", i->get_name() );
        Menu.select( &operationselect, n, strbuf );
        if( cur_op == i )
        {
            Menu.setcolor( WARNING );
        }
        n++;
    }
}


void create_new_operation_menu( int type )
{
    Menu.select( &operationcreate, type, operation_name( type ) );
}

void create_main_menu()
{
    printf("create main menu\n");
    Menu.clear();
    menuselect = 0;
    Menu.begin( "" );
    
        Menu.edit( Name, "Program name:" );
        
        Menu.edit( &scale, "Image scale " ); Menu.shortcut("AX=5" );
        Menu.edit( &pos.x, "Image x " ); Menu.shortcut("AX=0" );
        Menu.edit( &pos.z, "Image z " ); Menu.shortcut("AX=2" );
        
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
                sprintf(strbuf,"Delete current operation:%s", cur_op->get_name() );
                Menu.select( &menuselect, MENU_PHASE_DELETE, strbuf );
            }
            
            Menu.begin( "Create new operation" );
                Menu.back("Back");
                create_new_operation_menu( TOOL );
                create_new_operation_menu( CONTOUR );
                create_new_operation_menu( INSIDE_CONTOUR );
                create_new_operation_menu( TURN );
                create_new_operation_menu( UNDERCUT );
                create_new_operation_menu( FINISHING );
                create_new_operation_menu( THREADING );
                create_new_operation_menu( FACING );
                create_new_operation_menu( DRILL );
                create_new_operation_menu( PARTING );
                create_new_operation_menu( MOVE );
            Menu.end();
            
        Menu.end();
        
        create_operation_select_menus();

    Menu.end();
    
    Menu.setmaxlines( 15 );
}

void clear_all_operations()
{
    cur_contour = cur_tool = opl.end();
    bool ready = false;
    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        if( ! ready && cur_op != opl.end() )
        {
            if( i->get_type() == CONTOUR )
            {
                cur_contour = i;
            }

            if( i->get_type() == TOOL )
            {
                cur_tool = i;
            }
            
            if( i == cur_op )
            {
                 ready = true;
            }
        }
         
        i->clear();
    }
}

void create_paths()
{
    
    list<operation>::iterator contour = opl.end();
    list<operation>::iterator tool = opl.end();

    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        
        if( tool != opl.end() )
        {
            if( i->get_type() == FACING )
            {
                i->create_path( *contour, *tool );
            }
        }
        
        if( i->get_type() == CONTOUR )
        {
            contour = i;
            i->create_contour();
        }

        if( i->get_type() == TOOL )
        {
            tool = i;
        }
        
        if( tool != opl.end() && contour != opl.end() )
        {
            if( i->get_type() == TURN )
            {
                i->create_path( *contour, *tool );
            }
        }
        
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
    
    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        i->save_program( fp ); // must call in order! 
        
        if( i->get_type() != CONTOUR &&
            i->get_type() != INSIDE_CONTOUR
        )
        {
            
            bool b = true;
            if( std::next(i) != opl.end() )
            {
                if( std::next(i)->get_type() == MOVE ) 
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
    wiz_opl = &opl;

}


void wizards_save( const char *name )
{
    FILE *fp;

    sprintf(strbuf,"%s/%s.wiz", programPrefix, name );
    printf("save:%s\n",strbuf);
    fp = fopen( strbuf, "w");
    if (fp == NULL) return;
    fprintf(fp, "NAME %s\n", name );
    fprintf(fp, "INIT \"%s\"\n", initcommands );
    fprintf(fp, "STOCKDIAM %.20g\n", stockdiameter );
    fprintf(fp, "MAXRPM %i\n", maxrpm );
    fprintf(fp, "POSX %.20g\n", pos.x );
    fprintf(fp, "POSZ %.20g\n", pos.z );
    fprintf(fp, "SCALE %.20g\n", scale );
    
    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        i->save( fp );
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

    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("%s", line);
        
        sscanf(line, "%s %lf", tag, &val );
        
        findtag( tag, "STOCKDIAM", stockdiameter, val );
        findtag( tag, "MAXRPM", maxrpm, val );
        findtag( line, "INIT", initcommands );
        findtag( tag, "POSX", pos.x, val );
        findtag( tag, "POSZ", pos.z, val );
        findtag( tag, "SCALE", scale, val );

        if( strcmp( tag, "OPERATION" ) == 0 )
        {
            opl.push_back( operation( (op_type)val ) );
            opl.back().load( fp );
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
    

}


void wizards_init()
{
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
    }

    if( status.screenmode == SCREENWIZARDS )
    {
        create_main_menu();
    }
}


void draw_thread(double x1, double y1, double x2, double y2, double pitch, double depth )
{
    double dx =  x1 - x2;
    double dy =  y1 - y2;
    double l = sqrtf( dx*dx + dy*dy );
    if( l == 0.0f ) return;

    int n = l / pitch;

    vec2 v(x1,y1);
    v = v.normal( vec2(x2,y2) ) * depth;

    double nx = v.x;
    double ny = v.z;

    double dl = 1.0f / l;
    dl *= l / (double)n;

    dx *= dl;
    dy *= dl;

    double x = x1;
    double y = y1;
    glBegin(GL_LINE_STRIP);
        setcolor( CONTOUR_LINE );
        glVertex2f( x1, y1 );
        for( int i=0 ; i < n-1 ; i++ )
        {
            x -= dx;
            y -= dy;
            if( i & 1 )
            {
                glVertex2f( x , y );
            }
            else
            {
                glVertex2f( x - nx, y - ny );
            }

        }
        glVertex2f( x2, y2 );
    glEnd();

    x = x1;
    y = y1;
    glBegin(GL_LINE_STRIP);
        setcolor( CONTOUR_LINE );
        glVertex2f( x1, -y1 );
        for( int i=0 ; i < n-1 ; i++ )
        {
            x -= dx;
            y -= dy;
            if( i & 1 )
            {
                glVertex2f( x , -y );
            }
            else
            {
                glVertex2f( x - nx, -(y - ny) );
            }

        }
        glVertex2f( x2, -y2 );
    glEnd();

}



// inSegment(): determine if a point is inside a segment
//    Input:  a point P, and a collinear segment S
//    Return: 1 = P is inside S
//            0 = P is  not inside S
int
inSegment( vec2 P, vec2 S1, vec2 S2)
{
    if (S1.x != S2.x) {    // S is not  vertical
        if (S1.x <= P.x && P.x <= S2.x)
            return 1;
        if (S1.x >= P.x && P.x >= S2.x)
            return 1;
    }
    else {    // S is vertical, so test y  coordinate
        if (S1.z <= P.z && P.z <= S2.z)
            return 1;
        if (S1.z >= P.z && P.z >= S2.z)
            return 1;
    }
    return 0;
}

#define SMALL_NUM 0.000001
//===================================================================
// dot product (3D) which allows vector operations in arguments
#define DOT(u,v)   (u.dot(v))  //)((u).x * (v).x + (u).z * (v).z)
#define PERP(u,v)  (u.perp(v))

int get_line_intersection( vec2 S1P0, vec2 S1P1 , vec2 S2P0, vec2 S2P1, vec2 &I0 )
{
    vec2    u = S1P1 - S1P0;
    vec2   v = S2P1 - S2P0;
    vec2    w = S1P0 - S2P0;
    double     D = PERP(u,v);

    // test if  they are parallel (includes either being a point)
    if (fabs(D) < SMALL_NUM) {           // S1 and S2 are parallel
        if (PERP(u,w) != 0 || PERP(v,w) != 0)  {
            return 0;                    // they are NOT collinear
        }
        // they are collinear or degenerate
        // check if they are degenerate  points
        double du = DOT(u,u);
        double dv = DOT(v,v);
        if (du==0 && dv==0) {            // both segments are points
            if (S1P0 !=  S2P0)         // they are distinct  points
                 return 0;
            I0 = S1P0;                 // they are the same point
            return 1;
        }
        if (du==0) {                     // S1 is a single point
            if  (inSegment(S1P0, S2P0, S2P1 ) == 0)  // but is not in S2
                 return 0;
            I0 = S1P0;
            return 1;
        }
        if (dv==0) {                     // S2 a single point
            if  (inSegment(S2P0, S1P0, S1P1 ) == 0)  // but is not in S1
                 return 0;
            I0 = S2P0;
            return 1;
        }
        // they are collinear segments - get  overlap (or not)
        double t0, t1;                    // endpoints of S1 in eqn for S2
        vec2 w2 = S1P1 - S2P0;
        if (v.x != 0) {
                 t0 = w.x / v.x;
                 t1 = w2.x / v.x;
        }
        else {
                 t0 = w.z / v.z;
                 t1 = w2.z / v.z;
        }
        if (t0 > t1) {                   // must have t0 smaller than t1
                 double t=t0; t0=t1; t1=t;    // swap if not
        }
        if (t0 > 1 || t1 < 0) {
            return 0;      // NO overlap
        }
        t0 = t0<0? 0 : t0;               // clip to min 0
        t1 = t1>1? 1 : t1;               // clip to max 1
        if (t0 == t1) {                  // intersect is a point
            I0 = S2P0 +  v*t0;
            return 1;
        }

        // they overlap in a valid subsegment
        if( S1P0.dist_squared( S2P0 + v*t0 ) <  S1P0.dist_squared( S2P0 + v*t1 ))
        {
            I0 = S2P0 + v*t0;
        }
        else
        {
            I0 = S2P0 + v*t1;
        }
        return 2;
    }

    // the segments are skew and may intersect in a point
    // get the intersect parameter for S1
    double     sI = PERP(v,w) / D;
    if (sI < 0 || sI > 1)                // no intersect with S1
        return 0;

    // get the intersect parameter for S2
    double     tI = PERP(u,w) / D;
    if (tI < 0 || tI > 1)                // no intersect with S2
        return 0;

    I0 = S1P0 + u*sI;                // compute S1 intersect point
    return 1;
}
//===================================================================




void clamp_values()
{
     CLAMP( stockdiameter,0,1000 );
     CLAMP( maxrpm, 1, 5000 );
     CLAMP( ccut.end.x, 0, (stockdiameter/2.0) ); 
     CLAMP( scale,0.1,10 ); 
     CLAMP( ctool.tooln, 0, MAXTOOLS );   
}

void wizards_parse_serialdata()
{

    if( cur_contour == cur_op && cur_contour != opl.end() )
    {

        if( isprefix( "LEFT" ,NULL ) )
        {
            cur_contour->next();
            create_operation_menu();
        }
        else if( isprefix( "RIGHT" ,NULL ) )
        {
            cur_contour->previous();
            create_operation_menu();
        }
    }

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
                      
        if( menuselect == MENU_NEWCUT )
        {
            cur_contour->new_cut(vec2(0,0),CUT_LINE );
            create_operation_menu();
            return;
        }
        
        if( menuselect == MENU_DELETECUT )
        {
            cur_contour->erase();
            create_operation_menu();
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
            //cur_contour = cur_op = cur_tool = opl.end();
            clear_all_operations();
            create_main_menu();
            return;
        }
                
        if( menuselect == MENU_PHASE_DOWN && cur_op !=  opl.end() && cur_op != --opl.end() )
        {
            opl.splice( std::next(cur_op,2), opl, cur_op );
            //cur_contour = cur_op = cur_tool = opl.end();
            clear_all_operations();
            create_main_menu();
            return;
        }
               
        if( Menu.edited( &operationcreate ) )
        {
            opl.push_back( operation( (op_type)operationcreate ) );
            if( operationcreate == CONTOUR || operationcreate == INSIDE_CONTOUR )
            {
                cur_contour = --opl.end();
                cur_contour->new_cut(vec2(0,10),CUT_BEGIN );
            }
            clear_all_operations();
            create_main_menu();
            return;
        }

        else if( Menu.edited( &operationselect ) )
        {
            printf("select %d\n", operationselect );

            int n = 1;
            cur_contour = cur_op = cur_tool = opl.end();
            for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
            {
                if( n++ == operationselect )
                {
                    cur_op = i;
                    break;
                }
            }
            clear_all_operations();
            create_operation_menu();

        }

        // shape
        if( cur_contour != opl.end() )
        {
            
            if( cur_op->get_type() == CONTOUR && (
                Menu.edited( &ccut.end.z ) || 
                Menu.edited( &ccut.end.x ) ||
                Menu.edited( &ccut.type ) || 
                Menu.edited( &ccut.r ) )
            )
            {
                cur_contour->set_cut( ccut );
                if( Menu.edited( &ccut.type ) )
                {
                    create_operation_menu();// refresh type text
                }
            }

        }

        // tool
        if( cur_tool != opl.end() )
        {
            cur_tool->set_tool( ctool );
            ctool = cur_tool->get_tool();
        }
        
        if( cur_op != opl.end() )
        {
            if( cur_op->get_type() == FACING )
            {
                if( 
                    Menu.edited( &face_begin.x ) ||
                    Menu.edited( &face_end.x ) ||
                    Menu.edited( &face_begin.z ) || 
                    Menu.edited( &face_end.z ) 
                )
                {
                    cur_op->setf_begin_end( face_begin, face_end );
                }
            }
            
            else if( cur_op->get_type() == MOVE )
            {
                if( 
                    Menu.edited( &movepos.x ) ||
                    Menu.edited( &movepos.z ) 
                )
                {
                    cur_op->set_move( movepos );
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


void wizards_draw()
{
    draw_statusbar( "WIZARDS" );
    
    clamp_values();
    
    create_paths();
    
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

    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        i->draw( i == cur_op ? NONE:DISABLED );
    }
    
    setcolor( RAPID );
    drawCross( start_position.z, -start_position.x, 5/scale);
    drawCircle( start_position.z, -start_position.x, 5/scale );
    
    glPopMatrix();
    
    Menu.draw(5,50);

//double angle = atan2( fabs( currentcut->start.x - currentcut->end.x) , fabs(currentcut->dim.z) )* 180.0f / M_PI;
  //  sprintf(strbuf,"Angle %g", angle );
    //println( strbuf );

}









