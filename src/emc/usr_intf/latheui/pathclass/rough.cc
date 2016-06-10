#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;

void rough_path::create( contour_path &c, const tool &tl, bool oside )
{
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    
    outside = oside;
    tc.create( c, tool_r, outside );
    
    double x;
    double min_z = c.min.z;
    double max_z = c.max.z + tool_r + retract*3.0 ;
    double len = fabs( min_z - max_z );
    ml.clear();
    
    x = c.max.x;

    if( tc.ml.empty() )
    {
        return;
    }
 
    while( x > tc.ml.front().start.x + 0.001 )
    {
        feed_to_left( tc, vec2( x, max_z ), len ,tl.depth );
        x -= tl.depth;
    }
    findminmax();
        
}

void rough_path::draw( bool b )
{
    path::draw(b);
    tc.draw(b);
}
