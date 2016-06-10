#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern const vec2 startposition;


void fine_path::create( contour_path &c, double r, bool oside )
{
    
    vec2 n;
    path tp;
    ml.clear();
    outside = oside;
    
    // copy to temp list
    for(list<struct mov>::iterator i = c.ml.begin(); i != c.ml.end(); i++)
    {
        if( i->start.dist( i->end ) > 0.001 )
        {
            tp.create_line( i->end, FEED );
            tp.ml.back().start = i->start;
          //  tp.ml.back().end = i->end;
        }

    }

    // move normal direction
    for(list<struct mov>::iterator i = tp.ml.begin(); i != tp.ml.end(); i++)
    {
        n = i->start.normal( i->end ) * r;
        i->start += n;
        i->end += n;
    }

    // fix intersections
    vec2 iv(0,0);
    list<struct mov>::iterator i2 = ++(tp.ml.begin());
    for( list<struct mov>::iterator i1 = tp.ml.begin(); i2 != tp.ml.end(); i1++,i2++)
    {
        i1->vel.x = i1->vel.z = 0;
        if( get_line_intersection( i1->start, i1->end, i2->start, i2->end , iv ))
        {
            i1->end = iv;
            i2->start = iv;
        }
    }

    // close small gaps;
    i2 = ++(tp.ml.begin());
    for( list<struct mov>::iterator i1 = tp.ml.begin(); i2 != tp.ml.end(); i1++,i2++)
    {
        double l = i1->end.dist( i2->start ) ;
        if( l < 0.01 )
        {
            i1->end = i2->start = (i1->end + i2->start) / 2.0;

        }

    }

    // copy list & fill big gaps

    vec2 start(0,0);
    struct cut cutp;

    i2 = ++(tp.ml.begin());
    for( list<struct mov>::iterator i1 = tp.ml.begin(); i1 != tp.ml.end(); i1++,i2++)
    {

        create_line( i1->end, FEED );
        if(  i1 == tp.ml.begin() )  start = i1->start; // first start

        double l = i1->end.dist( i2->start ) ;
        if( l > 0.01 && i2 != tp.ml.end() )
        {

            if( l > 0.2 )
            {
                double r2 = fabs(r);
                double l = i1->end.dist( i2->start ) + 0.00001f;
                if( r2 < l/2.0f )  r2 = l/2.0f;
                create_arc( cutp, i1->end, i2->start , r2, (r < 0) );
            }
            else
            {
                create_line( i2->start, FEED );
            }
        }
    }

    // calc start/ends
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->start = start;
        start =  i->end;
    }

   // if( ml.size() > 0 ) ml.front().start.x = -0.1;
    remove_knots();
    findminmax();


}
