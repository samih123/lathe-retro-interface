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



static list<operation> opl;
list<operation>::iterator cur_op;
list<operation>::iterator cur_contour;
list<operation>::iterator cur_tool;

static bool draw_toolpath = false;
static bool dynamic_toolpath = true;


void save( const char *name )
{
    FILE *fp;

    sprintf(strbuf,"%s/%s.wiz", programPrefix, name );
    printf("save:%s\n",strbuf);
    fp = fopen( strbuf, "w");
    if (fp == NULL) return;
    fprintf(fp, "NAME %s\n", name );
    for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
    {
        i->save( fp );
    }

    fclose( fp );
}


static char Dstr[BUFFSIZE];
static char Zstr[BUFFSIZE];
static char Name[BUFFSIZE];
static int phasecreate;
static int phaseselect;
static int menuselect;

static cut ccut;
static tool ctool;
static double diameter;

vec2 startposition;

int maxrpm;
double stockdiameter;
static double scale = 2;
double retract = 1;


#define  MENU_NEWPHASE 1
#define  MENU_NEWCUT    2
#define  MENU_DELETECUT 3
#define  MENU_SAVE 4
#define  MENU_MAIN 5


void getzd()
{
    if( cur_contour != opl.end() ){
         ccut = cur_contour->get_cut();
         diameter = ccut.end.x *2.0;
    }
    
    if( cur_tool != opl.end() )
    {
        ctool = cur_tool->get_tool();
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

void create_phase_menu()
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
        if( type == CONTOUR )
        {
            sprintf(Dstr,"D start:%.20g end:", ccut.start.x*2.0f );
            sprintf(Zstr,"Z start:%.20g lenght:", ccut.start.z );

            Menu.edit( &ccut.type, typestr[ ccut.type ] );Menu.hiddenvalue();
            Menu.select( &menuselect, MENU_DELETECUT, "Delete" );
            Menu.select( &menuselect, MENU_NEWCUT, "New" );
            Menu.edit( &diameter, Dstr ); Menu.shortcut("AX=0" );
            Menu.edit( &ccut.end.z, Zstr ); Menu.shortcut("AX=2" );

            /*
            if( cur_contour->currentcut->type == ARC_IN || cur_contour->currentcut->type == ARC_OUT )
            {
                Menu.edit( &cur_contour->currentcut->r, "Radius" ); Menu.shortcut("AX=5" );
            }
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
            Menu.edit( &ctool.count, "Count " );
        }

        Menu.select( &menuselect, MENU_MAIN, "Back" );
    Menu.end();
}

void create_phase_select_menus()
{
    int n = 1;
    for( auto i: opl )
    {
        sprintf(strbuf,"phase %d:%s%s", n, i.get_type() > CONTOUR ? "  " : "", i.get_name() );
        Menu.select( &phaseselect, n, strbuf );
        n++;
    }
}


void create_new_phase_menu( int type )
{
    Menu.select( &phasecreate, type, phase_name( type ) );
}

void create_main_menu()
{
    printf("create main menu\n");
    Menu.clear();
    menuselect = 0;
    Menu.begin( "machining phases:" );
        Menu.edit( Name, "Program name:" );
        Menu.select( &menuselect, MENU_SAVE, "Save" );
        Menu.edit( &stockdiameter, "Stock diameter " );
        Menu.edit( &maxrpm, "Max spindle rpm " );
        Menu.begin( "Create new phase:" );
            Menu.back("Back");
            create_new_phase_menu( TOOL );
            create_new_phase_menu( CONTOUR );
            create_new_phase_menu( INSIDE_CONTOUR );
            create_new_phase_menu( TURN );
            create_new_phase_menu( UNDERCUT );
            create_new_phase_menu( FINISHING );
            create_new_phase_menu( THREADING );
            create_new_phase_menu( FACING );
            create_new_phase_menu( DRILL );
            create_new_phase_menu( PARTING );
        Menu.end();
        
        create_phase_select_menus();

    Menu.end();
    Menu.setmaxlines( 15 );
}



 void save_program(const char *name )
{

    FILE *fp;
    /*
    printf("save_program:%s\n",name);
    fp = fopen( name, "w");

    if (fp == NULL) return;

    fprintf(fp, "G18 G8 G21 G94 G40\n");
    fprintf(fp, "G64 P0.01\n");
    fprintf(fp, "( Rough )\n");

    roughpath.save( fp );

    fprintf(fp, "( undercut )\n");
  //  save_path( undercutpath,fp, tp[UNDERCUT] );


    fprintf(fp, "( Finish )\n");
    for( int i = tp[FINISH].count-1 ; i >= 0; i-- )
    {
    //  save_path( finepath[i], fp, tp[FINISH] );
    }

    fprintf(fp, "M5 (stop spindle)\n");

    fprintf(fp, "M2\n");
    fclose( fp );
*/
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
    cur_contour = cur_op = cur_tool= opl.end();

    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("%s", line);
        
        sscanf(line, "%s %lf", tag, &val );
        
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
        cur_contour = cur_op = cur_tool= opl.end();
    }
    create_main_menu();
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





static path temp1;
static path temp2;

void create_toolpath()
{
    /*
    startposition = contour.max;
    startposition.x = max( stockdiameter/2.0 , startposition.x );
    startposition += tp[ROUGH].depth + tp[ROUGH].tool_r;
    startposition.z += 5;

    roughpath.set_tool( tp[ROUGH].tool );
    roughpath.set_cut_param( 50,50,2 );
    roughpath.create( contour );

    undercutpath.set_tool( tp[UNDERCUT].tool );
    undercutpath.set_cut_param( 50,50,2 );
  //  undercutpath.create( contour );

    finepath[0].set_tool( tp[FINISH].tool );
    finepath[0].set_cut_param( 50,50,2 );
   // finepath[0].create( contour );
*/
}


void wizards_parse_serialdata()
{

    if( cur_contour == cur_op && cur_contour != opl.end() )
    {

        if( isprefix( "LEFT" ,NULL ) )
        {
            cur_contour->next();
            create_phase_menu();
        }
        else if( isprefix( "RIGHT" ,NULL ) )
        {
            cur_contour->previous();
            create_phase_menu();
        }
    }

    if( Menu.parse() )
    {

        if( menuselect == MENU_MAIN )
        {
            create_main_menu();
            return;
        }
        
        if( menuselect == MENU_SAVE )
        {
            
            save( Name );
            return;
        }
                
        if( menuselect == MENU_NEWCUT )
        {
            cur_contour->new_cut(vec2(10,0),CUT_LINE );
            create_phase_menu();
            return;
        }

        if( Menu.edited( &phasecreate ) )
        {
            opl.push_back( operation( (op_type)phasecreate ) );
            if( phasecreate == CONTOUR || phasecreate == INSIDE_CONTOUR )
            {
                cur_contour = --opl.end();
                cur_contour->new_cut(vec2(0,10),CUT_BEGIN );
            }
            create_main_menu();
            return;
        }

        else if( Menu.edited( &phaseselect ) )
        {
            printf("select %d\n", phaseselect );

            int n = 1;
            cur_contour = cur_op = cur_tool = opl.end();

            for(list<operation>::iterator i = opl.begin(); i != opl.end(); i++)
            {

                if( i->get_type() == CONTOUR )
                {
                    cur_contour = i;
                }

                if( i->get_type() == TOOL )
                {
                    cur_tool = i;
                }

                if( n++ == phaseselect )
                {
                    cur_op = i;
                    break;
                }

            }

            create_phase_menu();

        }

        // shape
        if( cur_contour != opl.end() )
        {

            if( Menu.edited( &ccut.end.z ) || 
                Menu.edited( &diameter ) ||
                Menu.edited( &ccut.type ) 
            )
            {
                ccut.end.x = diameter / 2.0;
                
                cur_contour->set_cut( ccut );
                if( Menu.edited( &ccut.type ) )
                {
                    create_phase_menu();// refresh type text
                }
            }

        }

        // tool
        if( cur_tool != opl.end() )
        {
            cur_tool->set_tool( ctool );
            ctool = cur_tool->get_tool();
        }

    }

    if( dynamic_toolpath) create_toolpath();

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

    if( cur_contour != opl.end() )
    {

        if( cur_contour->get_type() == CONTOUR )
        {
            cur_contour->draw( 0,100,100,100 );
        }

         if( cur_tool != opl.end() && cur_op != opl.end() )
         {
            if( cur_op->get_type() == TURN )
            {
                cur_op->create_path( *cur_contour, *cur_tool ,cur_contour->get_side() );
                cur_op->draw( 0,100,100,100 );
            }
         }

    }

    Menu.draw(5,50);

//double angle = atan2( fabs( currentcut->start.x - currentcut->end.x) , fabs(currentcut->dim.z) )* 180.0f / M_PI;
  //  sprintf(strbuf,"Angle %g", angle );
    //println( strbuf );

}









