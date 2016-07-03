#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;
extern const double stockdiameter;

void rough_path::create( contour_path &c, const tool &tl, Side s )
{
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    
    side = s;
    tc.create( c, tool_r, side, MOV_CONTOUR );
    tc.temporary = true;
    
    double x;
    double min_z = tc.min.z;
    double max_z = tc.max.z + tool_r + retract ;
    double len = fabs( min_z - max_z );
    ml.clear();
    
    if( tc.ml.empty() )
    {
        return;
    }
    
    if( side == OUTSIDE )
    {
        x = std::max( tc.max.x, stockdiameter/2.0 ) - tl.depth;
        create_line( vec2( x, max_z ), MOV_RAPID );
        
        while( x > tc.ml.front().start.x + 0.001 )
        {
            feed( tc, vec2( x, max_z ), len, vec2( 0,-1 ), vec2( 1,1 ) );
            x -= tl.depth;
        }
        
    }
    
    else if( side == INSIDE )
    {
        x = tc.min.x + tl.depth;
        create_line( vec2( x, max_z ), MOV_RAPID );
        
        while( x < tc.ml.front().start.x - 0.001 )
        {
            feed( tc, vec2( x, max_z ), len, vec2( 0,-1 ), vec2( -1,1 ) );
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
