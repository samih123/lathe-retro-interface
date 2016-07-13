#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;
extern const double stockdiameter;

void box_path::create( const tool &tl, vec2 start, vec2 fdir, vec2 ddir, double flen, double dlen )
{
    

    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    ml.clear();
   
    int count =  dlen / tl.depth; 
    
    vec2 retv = -ddir * retract;
    vec2 dv = ddir * tl.depth;
    vec2 fv = fdir *flen;
    vec2 trv = -ddir * tool_r - fdir * tool_r;
        
    create_line( start + trv , MOV_RAPID );

    while( count >= 0 )
    {
        vec2 v = start + ddir*dlen - dv * (double)count + trv;
        
        create_line( v, MOV_FEED );
        v += fv;
        create_line( v, MOV_FEED );
        v += retv;
        create_line( v, MOV_FEED );
        v -= fv;
        create_line( v, MOV_RAPID );
        v -= retv;
        
        --count; 
    }
    

    move( tool_cpoint( tl.tooln ) ); 
    findminmax();

}
