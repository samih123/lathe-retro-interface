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


#define MENUDELETE 1
#define MENUNEW 2
#define MENUSAVE 3
#define MENUTOOLPATH 4

#define ROUGH 0
#define UNDERCUT 1
#define FINISH 2
#define CONTOUR 3

const char *typestr[] =
{
    "straight line",
    "Outside arch",
    "Inside arch",
    "Thread",
    "Groove"
    "Rapid move",
    "Feed move",
};

extern char strbuf[BUFFSIZE];
extern struct machinestatus status;
extern int screenw, screenh;
extern char estr[BUFFSIZE];
static menu Menu;

static double scale = 2;
double retract = 1;
double stockdiameter;



static cutparam tp[3];

static list<struct cut> cuts;
static list<struct cut>::iterator currentcut;


#define MAXFINEPASS 20


//static vec2 contour_max;
//static vec2 contour_min;
vec2 startposition; 

static bool draw_toolpath = false;
static bool dynamic_toolpath = true;

static contour_path contour;
static rough_path roughpath;
static undercut_path undercutpath;
static fine_path finepath[MAXFINEPASS];



void add_cut(double x,double z)
{
    vec2 start(0,0);
    if( ! cuts.empty() )
    {
       start = cuts.back().end;
    }
    cuts.push_back( cut( cuts.back().dim.x + x, z) );
    currentcut = --cuts.end();
    currentcut->type = LINE;
    currentcut->start = start;
}




bool create_contour( contour_path &p )
{
    p.ml.clear();

    vec2 start(0,0);

    for(list<struct cut>::iterator i = cuts.begin(); i != cuts.end(); i++)
    {

       i->start = start;

       i->end.z = start.z + i->dim.z;
       i->end.x = i->dim.x;

       start =  i->end;

        if( i->type == ARC_IN || i->type == ARC_OUT )
        {
            double l = i->start.dist( i->end ) + 0.00001f;
            if( i->r < l/2.0f )  i->r = l/2.0f;
            p.create_arc( *i, i->start, i->end, i->r, i->type == ARC_IN );
        }
        else 
        {
            p.create_line( i->end, i->type );
        }

    }

    //p.remove_knots();
    p.findminmax();

    return true;
}


void save( const char *name )
{
    FILE *fp;

    sprintf(strbuf,"%s/%s.wiz", programPrefix, name );
    printf("save:%s\n",strbuf);
    fp = fopen( strbuf, "w");

    if (fp == NULL) return;

     fprintf(fp, "%d %d %d %f %f\n%d %d %d %f %f\n%d %d %d %f %f\n",
            tp[ROUGH   ].tool, tp[ROUGH   ].count, tp[ROUGH   ].speed, tp[ROUGH   ].feed, tp[ROUGH   ].depth,
            tp[UNDERCUT].tool, tp[UNDERCUT].count, tp[UNDERCUT].speed, tp[UNDERCUT].feed, tp[UNDERCUT].depth,
            tp[FINISH  ].tool, tp[FINISH  ].count, tp[FINISH  ].speed, tp[FINISH  ].feed, tp[FINISH  ].depth
         );
    for(list<struct cut>::iterator i = cuts.begin(); i != cuts.end(); i++)
    {
         fprintf(fp, "%d %f %f %f %f %f\n",
            i->type,
            i->dim.x,
            i->dim.z,
            i->r,
            i->pitch,
            i->depth
         );
    }
    fclose( fp );

}


static char Dstr[BUFFSIZE];
static char Zstr[BUFFSIZE];
static char Name[BUFFSIZE];
static int menuselect;
static double diameter;

static double X;
static double Y;
int maxrpm;

void toolmenu( int t, const char *n )
{
    Menu.begin( n );
        Menu.back("back");
        Menu.edit( &tp[t].tool, "Tool number " );
        Menu.edit( &tp[t].feed, "Feedrate mm/min" );
        Menu.edit( &tp[t].speed, "Surface speed m/s " );
        Menu.edit( &tp[t].depth, "Depth " );
        if( t== FINISH )
        {
            Menu.edit( &tp[t].count, "Count " );
        }
    Menu.end();
}

void createmenu()
{

    diameter = currentcut->dim.x *2.0f;
    //printf("max  %g,%g\n",contour_max.x,contour_max.z);
    //printf("min  %g,%g\n",contour_min.x,contour_min.z);

    sprintf(Dstr,"D start:%4.3g end:", currentcut->start.x*2.0f );
    sprintf(Zstr,"Z start:%4.3g lenght:", currentcut->start.z );

    Menu.clear();
    Menu.begin( "" );
        Menu.edit( &currentcut->type, typestr[ currentcut->type ] ); Menu.hiddenvalue();
        Menu.select( &menuselect, MENUDELETE, "Delete" );
        Menu.select( &menuselect, MENUNEW, "New" );
        Menu.edit( &diameter, Dstr ); Menu.shortcut("AX=0" );
        Menu.edit( &currentcut->dim.z, Zstr ); Menu.shortcut("AX=2" );
        if( currentcut->type == ARC_IN || currentcut->type == ARC_OUT )
        {
        Menu.edit( &currentcut->r, "Radius" ); Menu.shortcut("AX=5" );
        }
        if(currentcut->type == THREAD )
        {
        Menu.edit( &currentcut->pitch, "Pitch" );
        Menu.edit( &currentcut->depth, "Depth " );
        }
        Menu.edit( &stockdiameter, "Stock diameter " );
        Menu.edit( &maxrpm, "Max spindle rpm " );
        Menu.select( &menuselect, MENUTOOLPATH, "Create toolpath" );
        Menu.select( &menuselect, MENUSAVE, "Save program" );
        Menu.edit( Name, "Program name:" );
        Menu.edit( &scale, "Scale " );
        Menu.edit( &X, "X " );
        Menu.edit( &Y, "Y " );

        toolmenu( ROUGH , "Rough tool settings");
        toolmenu( UNDERCUT , "Undercut tool settings");
        toolmenu( FINISH , "Finish tool settings");
        Menu.edit( &dynamic_toolpath, "dynamic toolpath calculation " );
    Menu.end();
    Menu.setmaxlines( 10 );
}



 void save_program(const char *name )
{
    
    FILE *fp;
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
     
}


void wizards_load( const char *name )
{

    cuts.clear();

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

   printf("load:%s\n",name);
   fp = fopen( name, "r");

   if (fp == NULL) return;

    // read tool parameters
    for( int i=0 ; i<3 ; i++ )
    {
        if((read = getline( &line, &len, fp)) != -1)
        {
            sscanf(line, "%d %d %d %lf %lf",
                &tp[i].tool, &tp[i].count, &tp[i].speed, &tp[i].feed, &tp[i].depth
            );
        }
    }
    
    // read contour
    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("%s", line);
        cuts.push_back( cut(0,0) );
        list<struct cut>::iterator i= --cuts.end();
        sscanf(line, "%d %lf %lf %lf %lf %lf",
            &i->type,
            &i->dim.x,
            &i->dim.z,
            &i->r,
            &i->pitch,
            &i->depth
        );

        free(line);
        line = NULL;
    }
    currentcut = --cuts.end();
    fclose( fp );

    // strip name
    const char *n = name + strlen(name);
    while( *n != '/' && n > name ) n--;
    strcpy( Name, n+1 );
    n = strstr(Name, ".wiz" );
    if( n ) *(char *)n = 0;
    scale = 3;
    create_contour( contour );
    createmenu();
}


void wizards_init()
{

    scale = 2;
    retract = 1;
    maxrpm = status.maxrpm;

    //wizards_load( "/home/sami/linuxcnc/nc_files/sotilas.wiz" );
    if( cuts.size() == 0 )
    {
         add_cut(0,0);
         tp[ROUGH].tool = tp[FINISH].tool = tp[UNDERCUT].tool = 1;
         tp[ROUGH].feed = tp[FINISH].feed = tp[UNDERCUT].feed = 50;
         tp[ROUGH].speed = tp[FINISH].speed = tp[UNDERCUT].speed = 20;
    }
    scale = 1;
    create_contour( contour );
    createmenu();
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
        setcolor( GREEN );
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
        setcolor( GREEN );
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
      
}



void wizards_parse_serialdata()
{

    list<struct cut>::iterator oldcurrentcut = currentcut;
    int oldtype = currentcut->type;

    if( isprefix( "LEFT" ,NULL ) && currentcut != --cuts.end() )
    {
        currentcut++;
    }
    else if( isprefix( "RIGHT" ,NULL ) && currentcut != cuts.begin() )
    {
        currentcut--;
    }

    if( Menu.parse() )
    {

        CLAMP( tp[ROUGH].tool, 1, MAXTOOLS );
        CLAMP( tp[FINISH].tool, 1, MAXTOOLS );
        CLAMP( tp[FINISH].count, 1, MAXFINEPASS-1 );
        CLAMP( tp[UNDERCUT].tool, 1, MAXTOOLS );
        
        CLAMP( tp[ROUGH].feed, 0, 2000 );
        CLAMP( tp[UNDERCUT].feed, 0, 2000 );
        CLAMP( tp[FINISH].feed, 0, 2000 );
        
        CLAMP( tp[ROUGH].depth, 0.01, 5 );
        CLAMP( tp[UNDERCUT].depth, 0.01, 5 );
        CLAMP( tp[FINISH].depth, 0.01, 5 );   
        
        if( diameter < 0 ) diameter = 0;
        
        if( menuselect == MENUNEW && currentcut->dim.length() > 0 )
        {
             add_cut( 0,0 );
        }

        if( menuselect == MENUSAVE )
        {
             save( Name );
        }

        if( menuselect == MENUTOOLPATH )
        {
             create_toolpath();
             char path[LINELEN];
             sprintf( path, "%s/%s.ngc", programPrefix, Name );
             save_program( path );
             edit_load( path );
             auto_load( path );
        }

        if( Menu.edited( &diameter ) ||
            Menu.edited( &currentcut->dim.z ) ||
            Menu.edited( &currentcut->pitch ) ||
            Menu.edited( &currentcut->r ) ||
            Menu.edited( &currentcut->depth ) ||
            Menu.edited( &currentcut->type ) )
        {

            draw_toolpath = false;

            currentcut->dim.x = diameter/2.0;

            double dx =  currentcut->start.x - currentcut->dim.x;
            double dz =  currentcut->dim.z;
            double l =  sqrt( dx*dx + dz*dz ) + 0.00001f;
            if( currentcut->r < l/2.0f )  currentcut->r = l/2.0f;

            CLAMP( currentcut->type, 0, 3 );

            if( Menu.edited( &currentcut->pitch ) ) currentcut->depth = currentcut->pitch * 0.61343;
            if( currentcut->pitch < 0.1f ) currentcut->pitch = 0.1f;
            if( currentcut->depth < 0.1f ) currentcut->depth = 0.1f;

        }

        if( scale < 0.1f ) scale = 0.1f;

        if( menuselect == MENUDELETE && cuts.size() > 1 )
        {
             currentcut =  cuts.erase( currentcut );
             if( currentcut == cuts.end() ) currentcut--;
        }

    }

    if( currentcut->type != oldtype || currentcut != oldcurrentcut )
    {
        createmenu();
    }

    create_contour( contour );
    if( stockdiameter < contour.max.x *2.0f ) stockdiameter = contour.max.x *2.0f;
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

    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;


    //glEnable(GL_LINE_STIPPLE);
    //glLineStipple(1,0x5555);

    glPushMatrix();
    glTranslatef( 750 ,300 , 0);
    glScalef(scale*3.0f, scale*3.0f,scale*3.0f);
    glTranslatef( X ,Y , 0);
    glBegin(GL_LINES);
        setcolor( GREY );
        glVertex2f( 10,0 );
        glVertex2f( -600,0 );
    glEnd();

    for(list<struct cut>::iterator i = cuts.begin(); i != cuts.end(); i++)
    {

        x1 = i->start.z;
        y1 = -i->start.x;
        x2 = i->end.z;
        y2 = -i->end.x;

       // if( i != cuts.begin() )
        {
            glBegin(GL_LINES);
                setcolor( GREY );
                glVertex2f( x2, -y2 );
                glVertex2f( x2, y2  );
                glVertex2f( x1, -y1 );
                glVertex2f( x1, y1  );
            glEnd();

            if(i->type == THREAD )
            {
                setcolor( GREEN );
                draw_thread( x1,  y1,  x2,  y2, i->pitch, i->depth );
            }

            if( i->type == ARC_IN || i->type == ARC_OUT )
            {
                setcolor( GREEN );
                drawCross( i->center.z, i->center.x, 3.0f / scale );
                drawCross( i->center.z, -i->center.x, 3.0f / scale );
            }

            if( i == currentcut )
            {
                setcolor( RED );
                drawCircle( x2, y2, 3.0f / scale );
            }
        }

    }

    contour.draw( true );
    roughpath.draw( false );
    undercutpath.draw( false );
    finepath[0].draw( false );
    
/*
    if( draw_toolpath )
    {
        for( int i=0; i < tp[FINISH].count; i++ )
        {
            draw_contour( finepath[i], false );
        }
        draw_contour( temp1, false );
        draw_contour( temp2, false );
        draw_contour( roughpath, false );
        draw_contour( undercutpath, false );
    }
*/
    glPopMatrix();

    //glDisable(GL_LINE_STIPPLE);

    Menu.draw(5,50);

    double angle = atan2( fabs( currentcut->start.x - currentcut->end.x) , fabs(currentcut->dim.z) )* 180.0f / M_PI;
    sprintf(strbuf,"Angle %g", angle );
    println( strbuf );

    show_tool( 500, 70 , tp[ROUGH].tool, "Rough" );
    show_tool( 600, 70 , tp[FINISH].tool, "Finish" );
    show_tool( 700, 70 , tp[UNDERCUT].tool, "Undercut" );
}









