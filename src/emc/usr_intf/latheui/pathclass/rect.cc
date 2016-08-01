#include "../latheintf.h"

extern const double retract;
extern char strbuf[BUFFSIZE];;
extern const double stockdiameter;

void rectangle_path::create( const tool &tl, vec2 begin, vec2 end, int feedd )
{

    vec2 fdir;
    vec2 ddir;
    double flen;
    double dlen;
    
    if( feedd == DIRX )
    {
        fdir = vec2( begin.x > end.x ? -1:1 ,0 );
        ddir = vec2(0, begin.z > end.z ? -1:1 );
        flen = fabs( end.x - begin.x);
        dlen = fabs( end.z - begin.z);
    }
    else
    {
        fdir = vec2(0, begin.z > end.z ? -1:1 );
        ddir = vec2( begin.x > end.x ? -1:1 ,0);
        flen = fabs( end.z - begin.z);
        dlen = fabs( end.x - begin.x);
    }
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    ml.clear();
   
    int count =  dlen / tl.depth; 
    
    vec2 retv = -ddir * retract;
    vec2 dv = ddir * tl.depth;
    vec2 fv = fdir *flen;
    vec2 trv = -ddir * tool_r - fdir * tool_r;
        
    create_line( begin + trv , MOV_RAPID );

    while( count >= 0 )
    {
        vec2 v = begin + ddir*dlen - dv * (double)count + trv;
        
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
