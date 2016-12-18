#include "../latheintf.h"


extern char strbuf[BUFFSIZE];

void undercut_path::create( contour_path &c, double depth, double tool_r, double retract, Side s )
{
    side = s;
    tc.create( c, tool_r, side, MOV_CONTOUR );
    tc.temporary = true;
    bool up = false;
    double x = 0;
    ml.clear();
    if( ! tc.ml.empty() )
    {
        for(list<struct mov>::iterator i = tc.ml.begin(); i != tc.ml.end(); i++)
        {
    
            if( i->end.x > i->start.x ) // uphill
            {
                while( x < i->end.x ) x += depth;
                up = true;
            }
            else  // downhill
            {
                while( x > i->start.x ) x -= depth;
                if( i->end.x < x )
                {
                    double dx = i->end.x - i->start.x;
                    double dz = i->end.z - i->start.z;
        
                    while( x > i->end.x )
                    {
                        double z = i->start.z + dz * (x - i->start.x) / dx;
                        if( z > tc.min.z )
                        {
                             if( up )
                             {
                                 rapid_move( vec2( x, z ) );
                                 up = false; 
                             }
                             feed( tc, next(i,1), vec2( x, z ), fabs( c.min.z -z ), vec2( 0,-1 ), vec2( 1,1 ) );
                        }
                        x -= depth;
                    }
                    
                }
            }
        }
    }
    findminmax();
}

void undercut_path::draw( color c )
{
    path::draw( c );
    //tc.draw( c );
}
