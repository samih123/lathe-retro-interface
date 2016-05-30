#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern const vec2 startposition;

void rough_path::create( contour_path &c )
{

    tc.create( c, cp.tool_r );
    
    double x;
    double min_z = c.min.z;
    double max_z = c.max.z + cp.tool_r + retract*3.0 ;
    double len = fabs( min_z - max_z );
    ml.clear();
    
    create_line( startposition, RAPID );
    ml.back().toolchange = cp.tool;
    
    x = startposition.x ;

    if( tc.ml.empty() )
    {
        return;
    }
 
    while( x > tc.ml.front().start.x + 0.001 )
    {
        feed_to_left( tc, vec2( x, max_z ), len );
        x -= cp.depth;
    }
    findminmax();
        
}

void rough_path::draw( bool b )
{
    path::draw(b);
    tc.draw(b);
}
