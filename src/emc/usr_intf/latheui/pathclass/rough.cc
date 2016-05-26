#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern const vec2 startposition;

void rough_path::create( contour_path &c )
{

    fine_path tp;
    tp.create( c, cp.tool_r );
    
    double x;
    double min_z = c.min.z;
    double max_z = c.max.z + cp.tool_r + retract*3.0 ;
    double len = fabs( min_z - max_z );
    ml.clear();
    
    create_line( startposition, RAPID );
    ml.back().toolchange = cp.tool;
    
    x = startposition.x ;

    while( x > 0 )
    {
        feed_to_left( tp, vec2( x, max_z ), len );
        x -= cp.depth;
    }
    findminmax();
    
}
