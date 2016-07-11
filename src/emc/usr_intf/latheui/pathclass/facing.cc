#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;
extern const double stockdiameter;

void facing_path::create( const tool &tl, vec2 begin, vec2 end )
{
    

    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    tool_r = 0;
    ml.clear();
    
    

    
   
   
    int count = ( abs( begin.z - end.z ) / tl.depth ); 
    
    double z = end.z + tl.depth*(double)count + tool_r + retract;
    
    create_line( vec2( begin.x, z ) , MOV_RAPID );
    
    while( count >= 0 )
    {
        z = end.z + tl.depth*(double)count + tool_r;
        create_line( vec2( begin.x, z ), MOV_FEED );
        create_line( vec2( end.x, z ), MOV_FEED );
        create_line( vec2( end.x, z + retract ), MOV_FEED );
        create_line( vec2( begin.x, z + retract ), MOV_RAPID );
        --count; 
    }
    

    move( tool_cpoint( tl.tooln ) ); 
    findminmax();

}
