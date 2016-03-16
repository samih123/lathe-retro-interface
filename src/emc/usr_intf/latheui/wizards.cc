/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"
#include "menu.h"

#include <list>
using namespace std;
extern char programPrefix[LINELEN];

#define LINE 0
#define ARC_OUT 1
#define ARC_IN 2
#define THREAD 3
#define GROOVE 4
#define RAPID 5
#define FEED 6

#define MENUDELETE 1
#define MENUNEW 2
#define MENUSAVE 3
#define MENUTOOLPATH 4

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
static double retract = 1;
static int finish_count = 2;
//vec2 startpoint;
//double max_r;

struct cutparam
{
    double depth;
    double feed;
    double tool_r;
    int tool;
};

static cutparam rough;
static cutparam undercut;
static cutparam finish;

struct cut
{
    cut( double ax,double az )
    {
        dim.x = ax;
        dim.z = az;
        type = LINE;
        r = 1.0f;
        pitch = 1.0f;
        depth = 0.65f * pitch;
    }
    int type;
    vec2 dim;
    double r;
    double pitch;
    double depth;
    vec2 end,start,center;
};

static list<struct cut> cuts;
static list<struct cut>::iterator currentcut;






struct mov
{
    mov( double ax,double az, int ct )
    {
        end.x = ax;
        end.z = az;
        feed = 0;
        type = ct;
    }
    mov( const vec2 &v, int ct )
    {
        end = v;
        feed = 0;
        type = ct;
    }
    ~mov()
    {
        comment.erase();
    }
    
    vec2 end,start;
    vec2 vel;
    double pitch;
    double depth;
    double feed;
    int type;
    string comment;
};

#define MAXFINEPASS 10


static vec2 contour_max;
static vec2 contour_min;



static std::list<struct mov> contour;
static std::list<struct mov> finepath[MAXFINEPASS];
static std::list<struct mov> roughpath;
static std::list<struct mov> undercutpath;
static std::list<struct mov> threadpath;
static std::list<struct mov> toolpath[2];
static bool draw_toolpath = false;

void create_line( std::list<struct mov> &ml, const vec2 &v , int t, const char *comment = NULL )
{
    vec2 start;
    if( ml.empty() )
    {
        start.x = start.z = 0;
    }
    else
    {
        start = ml.back().end;
    }
    ml.push_back( mov( v.x, v.z , t ) );
    ml.back().start = start;
    if( comment != NULL ) ml.back().comment = comment;
}

void create_line( struct cut *c, std::list<struct mov> &ml, const vec2 &v )
{
    create_line( ml, v, c->type );
}



void create_arc( struct cut *c, std::list<struct mov> &ml, const vec2 v1, const vec2 v2, double r, bool side)
{
    
    double x1 = v1.x;
    double y1 = v1.z;
    double x2 = v2.x;
    double y2 = v2.z;
    double q = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    double x3 = (x1+x2)/2.0f;
    double y3 = (y1+y2)/2.0f;

    double cx;
    double cy;
    
    double u = r*r - q*q/4.0;
        
    if( side )
    {
        cx = x3 + sqrt(u)*(y1-y2)/q;
        cy = y3 + sqrt(u)*(x2-x1)/q;
    }
    else
    {
        cx = x3 - sqrt(u)*(y1-y2)/q;
        cy = y3 - sqrt(u)*(x2-x1)/q;
    }
    
    c->center.x = -cx;
    c->center.z = cy;

    double start_angle = atan2(y1 - cy, x1 - cx);
    double dot = (x1-cx)*(x2-cx) + (y1-cy)*(y2-cy);
    double det = (x1-cx)*(y2-cy) - (y1-cy)*(x2-cx);
    double angle = atan2(det, dot);

    int num_segments = fabs( angle )*5.0f*r;
    if( num_segments < 3 ) num_segments = 3;
    
    double theta = angle / double(num_segments - 1);
    double tangetial_factor = tanf(theta);
    double radial_factor = cosf(theta);

    double x = r * cosf(start_angle);
    double y = r * sinf(start_angle);
    for(int ii = 0; ii < num_segments; ii++)
    {
        create_line( c, ml, vec2(x + cx, y + cy) );
        double tx = -y;
        double ty = x;
        x += tx * tangetial_factor;
        y += ty * tangetial_factor;
        x *= radial_factor;
        y *= radial_factor;
    }
}


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



void remove_short_lines(std::list<struct mov> &ml, double min )
{
    vec2 start(0,0);
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( start.dist_squared( i->end ) < min*min )
        {
           // printf("removing short line %f\n", start.dist( i->dim ) );
            i = contour.erase( i );
        }
        start = i->end;
    }
     
}

bool create_contour()
{
    contour.clear();

    contour_max = vec2( -1000000,-1000000 );
    contour_min = vec2( 1000000,1000000 );
    
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
            create_arc( &*i, contour, i->start, i->end, i->r, i->type == ARC_IN );
            //contour.back().end = i->end;
        }
        else if(i->type == LINE )
        {
            create_line( &*i, contour, i->end );
        }
        else if(i->type == THREAD )
        {
            create_line( &*i, contour, i->end );
        }
        else printf("error:unknow type %i\n",i->type); 
    }
    
    //remove_short_lines( contour ,0.01 );
    
    vec2(0,0).findminmax( contour_min, contour_max );
    start.x = start.z = 0;
    for(list<struct mov>::iterator i = contour.begin(); i != contour.end(); i++)
    {
        i->end.findminmax( contour_min, contour_max );
        i->start = start;
        start =  i->end;
    }
    
    
    return true;
}


void save( const char *name )
{
    FILE *fp;

    sprintf(strbuf,"%s/%s.wiz", programPrefix, name );
    printf("save:%s\n",strbuf);
    fp = fopen( strbuf, "w");

    if (fp == NULL) return;

     fprintf(fp, "%d %d %d\n",
            rough.tool,
            undercut.tool,
            finish.tool
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


void save_path( list<struct mov> ml,FILE *fp ,cutparam &cp) 
{
    
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        strbuf[0] = 0;
        if( ! i->comment.empty() )
        {
            sprintf( strbuf, "( %s )", i->comment.c_str() );
        }
        fprintf(fp, "G%s X%.4f Z%.4f%s\n",
            i->type == FEED ? "1":"0" ,
            fabs( i->end.x ) < 0.001 ? 0:i->end.x,
            fabs( i->end.z ) < 0.001 ? 0:i->end.z,
            strbuf 
        );
         
    }  
}

 void save_program(const char *name )
{
    FILE *fp;
    printf("save_program:%s\n",name);
    fp = fopen( name, "w");

    if (fp == NULL) return;
    
    fprintf(fp, "G18 G8 G21 G94 G40\n");    
    fprintf(fp, "G64 P0.01\n");
    fprintf(fp, "T1 M6\n");
    fprintf(fp, "( Rough )\n");
    fprintf(fp, "F400\n");
    
    save_path( roughpath, fp, rough );

    fprintf(fp, "( undercut )\n");
    fprintf(fp, "F400\n");
    save_path( undercutpath,fp, undercut );

    
    fprintf(fp, "( Finish )\n");
    fprintf(fp, "F400\n");
    for( int i = finish_count-1 ; i >= 0; i-- )
    {
      save_path( finepath[i],fp, finish );
    }   

    fprintf(fp, "M2");
    fclose( fp );
    
    
}

static char Dstr[BUFFSIZE];
static char Zstr[BUFFSIZE];
static char Name[BUFFSIZE];
static int menuselect;
static double diameter;
static double stockdiameter;
static double X;
static double Y;

void createmenu()
{
    
    diameter = currentcut->dim.x *2.0f;
    printf("max  %g,%g\n",contour_max.x,contour_max.z);
    printf("min  %g,%g\n",contour_min.x,contour_min.z);
    
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
        Menu.select( &menuselect, MENUTOOLPATH, "Create toolpath" ); 
        Menu.select( &menuselect, MENUSAVE, "Save program" );
        Menu.edit( Name, "Program name:" );  
        Menu.edit( &scale, "Scale " );  
        Menu.edit( &X, "X " );
        Menu.edit( &Y, "Y " );
        Menu.edit( &rough.tool, "Rough tool " );
        Menu.edit( &finish.tool, "Finish tool " );
        Menu.edit( &undercut.tool, "Undercut tool " );
    Menu.end(); 
    Menu.setmaxlines( 10 );   
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

    if((read = getline( &line, &len, fp)) != -1)
    {
         sscanf(line, "%d %d %d\n",
            &rough.tool,
            &undercut.tool,
            &finish.tool
         );
    }
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
    create_contour();
    createmenu();
}


void wizards_init()
{
    
   rough.depth = 2.0;
   undercut.depth = 2.0;
   finish.depth  = 0.2;
   
   scale = 2;
   retract = 1;
   finish_count = 2;
   
    //wizards_load( "/home/sami/linuxcnc/nc_files/sotilas.wiz" );
    if( cuts.size() == 0 )
    {
         add_cut(10,0);
         
         rough.tool = 1;
         finish.tool  = 1;
         undercut.tool = 1;
    }
    scale = 1;
    create_contour();
    createmenu();
}


void draw_contour( std::list<struct mov> &ml ,bool both )
{
    
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        
        //setcolor( MAGENTA );drawCross( i->vel.z,-(i->vel.x) ,2);////////////////////
        
        if( i->type == THREAD && both )
        {
            setcolor( GREY );
        }
        else if( i->type == RAPID )
        {
            setcolor( RED );
        }
        else if( i->type == FEED )
        {
            setcolor( YELLOW );
        }
        else
        {
            setcolor( GREEN );
        }
        
        glBegin(GL_LINES);

            glVertex2f( i->start.z, -i->start.x );
            glVertex2f( i->end.z, -i->end.x );

            if( i != ml.begin() && 0 ) // draw normals
            {
                glVertex2f( i->end.z, -i->end.x );
                vec2 v( i->start.z, -i->start.x );
                v = v.normal( vec2(i->end.z, -i->end.x) );
                glVertex2f( i->end.z + v.x,  -i->end.x + v.z );
            }

            if( both )
            {
                glVertex2f( i->start.z, i->start.x );
                glVertex2f( i->end.z, i->end.x );
            }

        glEnd();
        
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



double dist_from_segment( const vec2 v, const vec2 w, const vec2 p )
{
    const double distSq = v.dist_squared( w );
    if ( distSq == 0.0 )
    {
        return v.dist( p );
    }
    const double t = ( p - v ).dot( w - v ) / distSq;
    if ( t < 0.0 )
    {
        return v.dist( p );
    }
    else if ( t > 1.0 )
    {
        return w.dist( p );
    }
    const vec2 projection = v + ( ( w - v ) * t );
    return p.dist( projection );
}


double distance( std::list<struct mov> &ml, const vec2 &p )
{
    double mindist = 1000000;
    vec2 start(0,0);
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( i->type != RAPID )
        {
            double d = dist_from_segment( i->start, i->end , p );
            if( d < mindist ) mindist = d ;
        }

    }
    return mindist;
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

#define SMALL_NUM 0.00000001
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
 
  
double Segment_to_Segment( vec2 a1, vec2 a2, vec2 b1, vec2 b2)
{
    vec2   u = a2 - a1;//S1P1 - S1P0;
    vec2   v = b2 - b1;//S2P1 - S2P0;
    vec2   w = a1 - b1;//S1P0 - S2P0;

    double    a = u.dot(u);         // always >= 0
    double    b = u.dot(v);
    double    c = v.dot(v);         // always >= 0
    double    d = u.dot(w);
    double    e = v.dot(w);
    double    D = a*c - b*b;        // always >= 0
    double    sc, sN, sD = D;       // sc = sN / sD, default sD = D >= 0
    double    tc, tN, tD = D;       // tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < SMALL_NUM) { // the lines are almost parallel
        sN = 0.0;         // force using point P0 on segment S1
        sD = 1.0;         // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else {                 // get the closest points on the infinite lines
        sN = (b*e - c*d);
        tN = (a*e - b*d);
        if (sN < 0.0) {        // sc < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if (sN > sD) {  // sc > 1  => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) {            // tc < 0 => the t=0 edge is visible
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0)
            sN = 0.0;
        else if (-d > a)
            sN = sD;
        else {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD) {      // tc > 1  => the t=1 edge is visible
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0)
            sN = 0;
        else if ((-d + b) > a)
            sN = sD;
        else {
            sN = (-d +  b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (fabs(sN) < SMALL_NUM ? 0.0 : sN / sD);
    tc = (fabs(tN) < SMALL_NUM ? 0.0 : tN / tD);

    // get the difference of the two closest points
    vec2   dP = w + (u *sc) - ( v*tc );  // =  S1(sc) - S2(tc)

    return dP.length();   // return the closest distance
}
//===================================================================void

void find_max( std::list<struct mov> &ml , vec2 &m )
{
    m.z = m.x = -1000000;
    
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( m.z < i->end.z ) m.z = i->end.z;
        if( m.z < i->start.z ) m.z = i->start.z;
        if( m.x < i->end.x ) m.x = i->end.x;
        if( m.x < i->start.x ) m.x = i->start.x;        
    }
} 

double distance( std::list<struct mov> &ml, const vec2 p1, const vec2 p2)
{
    double mindist = 1000000;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        double d = Segment_to_Segment( p1, p2, i->start, i->end );
        if( d < mindist ) mindist = d ;
    }
    return mindist;
}


bool test_collision( std::list<struct mov> &ml, const vec2 p1, const vec2 p2, double d)
{
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( d > Segment_to_Segment( p1, p2, i->start, i->end ) ) return true;
    }
    return false;;
}

void rapid_move( std::list<struct mov> &ml, std::list<struct mov> &contour, const vec2 &v , cutparam cp)
{
    
    if( distance( contour, v, ml.back().end ) > cp.tool_r - 0.001 )
    {
        create_line( ml, v, RAPID, "rapid" );
    }
    else
    {
        double r = max( stockdiameter/2.0f, ml.back().end.x ) + retract;
        if( r < ml.back().end.x ) r = ml.back().end.x;
        if( r < v.x ) r = v.x;
        create_line( ml, vec2( r, ml.back().end.z ), RAPID ,"rapid start");
        create_line( ml, vec2( r, v.z ), RAPID );
        create_line( ml, v, RAPID  ,"rapid end");
    }
    
}



bool find_intersection( std::list<struct mov> &cl, list<struct mov>::iterator ci, const vec2 a, const vec2 b, vec2 &vi, list<struct mov>::iterator &ii, bool first = true )
{
    
    bool col = false;
    //list<struct mov>::iterator i = i2;
    for(; ci != cl.end(); ci++)
    {
        if( get_line_intersection( a,b , ci->start, ci->end, vi ) ) 
        {
           ii = ci; 
           col = true;
           if( first ) return true;
        }
    } 
    if( ! col ) vi = b;
    return col;
} 


void remove_knots( std::list<struct mov> &ml )
{
    //printf("find knots...\n");
    vec2 vi(0,0);
    
    list<struct mov>::iterator i2,ie;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {   
        if( find_intersection( ml, next(i,2), i->start, i->end, vi, i2, false ) )
        {
            //printf("intersec %f %f \n",vi.x,vi.z);
            ie = i;
            ie++;
            if( ie != ml.end())
            {
                ml.erase( ie, i2 );
                i->end = vi;
                i2->start = vi;
            }
            i->vel = vi;
        }
        
    }
}

double create_finepath( std::list<struct mov> &ml, std::list<struct mov> &ml2, double r )
{
    vec2 n;
    
    std::list<struct mov> tm;
    ml.clear();
    tm.clear();
    
    // copy to temp list
    for(list<struct mov>::iterator i = ml2.begin(); i != ml2.end(); i++)
    {
        if( i->start.dist( i->end ) > 0.001 )
        {
            create_line( tm, i->end, FEED );
            tm.back().start = i->start;
            tm.back().end = i->end;
        }
        
    }
    
    // move normal direction
    for(list<struct mov>::iterator i = tm.begin(); i != tm.end(); i++)
    {
        n = i->start.normal( i->end ) * r;
        i->start += n;
        i->end += n;
    }
        
    // fix intersections
    vec2 iv(0,0);
    list<struct mov>::iterator i2 = ++(tm.begin());
    for( list<struct mov>::iterator i1 = tm.begin(); i2 != tm.end(); i1++,i2++)
    {
        i1->vel.x = i1->vel.z = 0;
        if( get_line_intersection( i1->start, i1->end, i2->start, i2->end , iv ))
        {
            i1->end = iv;
            i2->start = iv; 
        }
    }

    // close small gaps;
    i2 = ++(tm.begin());
    for( list<struct mov>::iterator i1 = tm.begin(); i2 != tm.end(); i1++,i2++)
    {
        double l = i1->end.dist( i2->start ) ;
        if( l < 0.01 )
        {
            i1->end = i2->start = (i1->end + i2->start) / 2.0;
            
        }
             
    }   

    // copy list & fill big gaps
    
    vec2 start(0,0);
    cut c(0,0);
    c.type = FEED;
    
    i2 = ++(tm.begin());
    for( list<struct mov>::iterator i1 = tm.begin(); i1 != tm.end(); i1++,i2++)
    {   
        
        create_line( ml, i1->end, FEED );
        if(  i1 == tm.begin() )  start = i1->start; // first start
        
        double l = i1->end.dist( i2->start ) ;
        if( l > 0.01 && i2 != tm.end() )
        {
            
            if( l > 0.2 )
            {
                double r2 = fabs(r);
                double l = i1->end.dist( i2->start ) + 0.00001f;
                if( r2 < l/2.0f )  r2 = l/2.0f;
                create_arc( &c, ml, i1->end, i2->start , r2, (r < 0)  );
            }
            else
            {
                create_line( ml, i2->start, FEED );
            }
        }
    }

    // calc start/ends
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->start = start;
        start =  i->end;
    }
    
    if( ml.size() > 0 ) ml.front().start.x = -0.1;
    
    remove_knots( ml );
    
    tm.clear();
    return 0;
    
}

void feed_to_left(
    list<struct mov> &ml, 
    list<struct mov> &cl,
    list<struct mov>::iterator ci,
    vec2 v, double len , cutparam &cp)
{
    vec2 v2;
    list<struct mov>::iterator it;
    bool first = (ml.size() == 0);
    find_intersection( cl, ci, v, vec2( v.x, v.z - len ), v2, it, true );

    if( v.dist( v2 ) > retract*2.0 ){
        if( ! first ) rapid_move( ml, cl, vec2( v.x + retract + cp.depth, v.z - retract ) , cp );
        create_line( ml, v, FEED );
        create_line( ml, v2 , FEED );
        create_line( ml, vec2( v2.x + retract, v2.z + retract ) , FEED );
        if( first )
        {
             ml.front().start = v;
             ml.front().comment += "first";
        }
    }
    
}



void make_rough_path( std::list<struct mov> &ml, std::list<struct mov> &cl )
{
        
    vec2 max;
    find_max( cl, max );
    
    double x;
    double min_z = contour_min.z;
    double max_z = max.z + rough.tool_r + retract*3.0 ;
    double len = fabs( min_z - max_z );
    ml.clear();

    x = stockdiameter/2.0 + rough.depth + rough.tool_r ;
    
    vec2 start( x, max_z );
    
    while( x > 1 )
    {
        feed_to_left( ml, cl, cl.begin(), vec2( x, max_z ), len, rough );
        x -= rough.depth;
    }
    
}


void make_undercut_path( std::list<struct mov> &ml, std::list<struct mov> &cl )
{

    double x = 0;
    ml.clear();
    
    for(list<struct mov>::iterator i = cl.begin(); i != cl.end(); i++)
    {
        
        if( i->end.x > i->start.x ) // uphill
        {
            while( x < i->end.x ) x += undercut.depth;
        }
        else  // downhill
        {
            while( x > i->start.x ) x -= undercut.depth;
            if( i->end.x < x )
            {
                double dx = i->end.x - i->start.x;
                double dz = i->end.z - i->start.z;
               // for x from x1 to x2 {
                while( x > i->end.x )
                {
                    double z = i->start.z + dz * (x - i->start.x) / dx;
                    if( z > contour_min.z )
                    {
                         feed_to_left( ml, cl, next(i,1), vec2( x, z ), fabs( contour_min.z -z ), undercut );
                    }
                    x -= undercut.depth;
                }
            }
        }
    }   
    
}


void rapid_to_next_path( list<struct mov> &p1, list<struct mov> &p2, list<struct mov> &cl, cutparam &cp )
{
    rapid_move( p1, cl, vec2( p2.front().start.x+ retract, p2.front().start.z + retract ) , cp );
    create_line( p1, p2.front().start, FEED, "rapid to next");
}


static std::list<struct mov> temp1;
static std::list<struct mov> temp2;

void create_toolpath()
{

 
   rough.tool_r = _tools[ rough.tool ].diameter/2.0f;
   undercut.tool_r = _tools[ undercut.tool ].diameter/2.0f;
   finish.tool_r  = _tools[ finish.tool ].diameter/2.0f;
   
    create_contour();
     //   make_rough_path( roughpath, contour );
        
    create_finepath( finepath[0],contour, finish.tool_r );
    for( int i=1; i < finish_count; i++ )
    {
        create_finepath( finepath[i],finepath[i-1], finish.tool_r + finish.depth );
    }
    
    create_finepath( temp1, finepath[ finish_count-1 ] ,rough.tool_r );
    create_finepath( temp2, finepath[ finish_count-1 ] ,undercut.tool_r ); 
    
    make_rough_path( roughpath, temp1 );
    make_undercut_path( undercutpath, temp2 );
    
    rapid_to_next_path( roughpath, undercutpath, temp1, undercut );
    rapid_to_next_path( undercutpath, finepath[finish_count-1], temp2, finish );
   
    for( int i=1; i < finish_count; i++ )
    {
        rapid_to_next_path( finepath[i], finepath[i-1], finepath[i], finish );
    }
    
    draw_toolpath = true;
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
        
        CLAMP( rough.tool, 1, MAXTOOLS );
        CLAMP( finish.tool, 1, MAXTOOLS );
        CLAMP( undercut.tool, 1, MAXTOOLS );
        
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

    create_contour();
    if( stockdiameter < contour_max.x *2.0f ) stockdiameter = contour_max.x *2.0f;
    //create_toolpath();

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

    draw_contour( contour, true );

    if( draw_toolpath )
    {
        for( int i=0; i < finish_count; i++ )
        {
            draw_contour( finepath[i], false );
        }
        draw_contour( temp1, false );
        draw_contour( temp2, false );
        draw_contour( roughpath, false );
        draw_contour( undercutpath, false );  
    }
    
    glPopMatrix();
    
    //glDisable(GL_LINE_STIPPLE);

    Menu.draw(0,25);
    
    double angle = atan2( fabs( currentcut->start.x - currentcut->end.x) , fabs(currentcut->dim.z) )* 180.0f / M_PI;
    sprintf(strbuf,"Angle %g", angle );
    println( strbuf );   
    
    show_tool( 500, 70 , rough.tool, "Rough" );
    show_tool( 600, 70 , finish.tool, "Finish" );
    show_tool( 700, 70 , undercut.tool, "Undercut" );
}









