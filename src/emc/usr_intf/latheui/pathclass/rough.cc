#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;

void rough_path::create( contour_path &c, const tool &tl, Side s )
{
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    
    side = s;
    tc.create( c, tool_r, side, MOV_CONTOUR );
    
    double x;
    double min_z = c.min.z;
    double max_z = c.max.z + tool_r + retract ;
    double len = fabs( min_z - max_z );
    ml.clear();
    
    if( tc.ml.empty() )
    {
        return;
    }
    
    if( side == OUTSIDE )
    {
        x = c.max.x - tl.depth;
        while( x > tc.ml.front().start.x + 0.001 )
        {
            feed_to_left( tc, vec2( x, max_z ), len );
            x -= tl.depth;
        }
    }
    
    else if( side == INSIDE )
    {
        x = c.min.x + tl.depth;
        while( x < tc.ml.front().start.x - 0.001 )
        {
            feed_to_left( tc, vec2( x, max_z ), len );
            x += tl.depth;
        }
    }
        
    findminmax();
        
}

void rough_path::draw( bool b )
{
    path::draw(b);
    tc.draw(b);
}
