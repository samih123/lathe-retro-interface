#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;


void path::create_line( const vec2 &v , const int t, const char *comment )
{
    vec2 start;
    if( ml.empty() )
    {
        //start.x = start.z = 0;
        start = v;
    }
    else
    {
        start = ml.back().end;
    }
    ml.push_back( mov( v.x, v.z , t ) );
    ml.back().start = start;
    if( comment != NULL ) ml.back().comment = comment;
}


void path::create_arc( struct cut &c, const vec2 v1, const vec2 v2, const double r, const bool side)
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

    c.center.x = -cx;
    c.center.z = cy;

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
        create_line( vec2(x + cx, y + cy), c.type );
        double tx = -y;
        double ty = x;
        x += tx * tangetial_factor;
        y += ty * tangetial_factor;
        x *= radial_factor;
        y *= radial_factor;
    }
}

void path::rapid_move( const vec2 v )
{
    double r = stockdiameter/2.0f + retract;
    if( r < ml.back().end.x ) r = ml.back().end.x;
    if( r < v.x ) r = v.x;
    create_line( vec2( r, ml.back().end.z ), RAPID );
    create_line( vec2( r, v.z ), RAPID );
    create_line( v, RAPID );
}


void path::feed_to_left( path &colp, vec2 v, double len , double depth)
{
    list<struct mov>::iterator fi = colp.ml.begin();
    feed_to_left( colp, fi, v, len, depth);
}

void path::feed_to_left( path &colp, list<struct mov>::iterator fi, vec2 v, double len , double depth)
{
    vec2 v2;
    list<struct mov>::iterator ci;
    colp.find_intersection( v, vec2( v.x, v.z - len ), v2, fi, ci, false );
    bool first = (ml.size() == 0);
  //  if( v.dist( v2 ) > retract*2.0 ){
        if( ! first ) rapid_move( vec2( v.x + retract + depth, v.z - retract ) );
        create_line( v, FEED );
        create_line( v2 , FEED );
        create_line( vec2( v2.x + retract, v2.z + retract ) , FEED );
  //  }
}


bool path::find_intersection( const vec2 a, const vec2 b, vec2 &cv,
                                list<struct mov>::iterator fi, list<struct mov>::iterator &ci, bool first )
{

    bool col = false;
    vec2 colv = b;
    double mindist = 1000000000;

    for( ;fi != ml.end(); fi++)
    {
        if( get_line_intersection( a,b , fi->start, fi->end, cv ) )
        {
            
            if( first )
            {
                ci = fi;
                return true;
            }
            
            col = true;
            if( mindist > a.dist_squared( cv ) )
            {
                colv = cv;
                ci = fi;
                mindist = a.dist_squared( cv );
            }
            
        }
    }

    cv = colv;
    return col;
}

#define SMALL_NUM 0.00000001

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



double path::distance( const vec2 p1, const vec2 p2)
{
    double mindist = 1000000;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        double d = Segment_to_Segment( p1, p2, i->start, i->end );
        if( d < mindist ) mindist = d ;
    }
    return mindist;
}


void path::remove_knots()
{
    //printf("find knots...\n");
    vec2 vi(0,0);

    list<struct mov>::iterator i2,ie;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( find_intersection( i->start, i->end, vi, next(i,2), i2, false ) )
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


void path::draw( bool both )
{
  
    for(list<struct mov>::iterator i = ++(ml.begin()); i != ml.end(); i++)
    {


        if( i->type == RAPID )
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


void path::save( FILE *fp )
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
void path::findminmax()
{
    min = 1000000000;
    max = -1000000000;
    vec2 start(0,0);
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->end.findminmax( min, max );
        i->start = start;
        start =  i->end;
    }
}





