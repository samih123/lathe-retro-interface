#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;



void path::sanitize()
{
    if( ml.size() > 1 )
    {
        vec2 v = ml.begin()->start;
        for( list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
        {
            i->start = v;
            v = i->end;
        } 
    }
}


void path::erase()
{
    if( ml.size() > 1 && currentmov != ml.begin() )
    {
        currentmov =  ml.erase( currentmov );
        if( currentmov == ml.end() ) currentmov--;
        sanitize();
    }
}

void path::previous()
{
    if(  currentmov != ml.begin() )
    {
        currentmov--;
    }
}

void path::next()
{
    if( currentmov != --ml.end() )
    {
        currentmov++;
    }
}


void path::movecurpos( vec2 v )
{
    currentmov->end += v;
    if( currentmov != ml.end() )
    {
        (++currentmov)->start += v;
        --currentmov;
    }
    findminmax();
}


void path::setcur_type( move_type t )
{
   if( currentmov != ml.end() && currentmov != ml.begin() )
   {
       if( t < MOV_LINE ) t = MOV_LINE;
       if( t > MOV_ARC_IN ) t = MOV_ARC_IN;
       currentmov->type = t;
   }
}

void path::setcurradius( double r )
{
   if( currentmov != ml.end() )
   {
       if( r > 0 && (currentmov->type == MOV_ARC_IN || currentmov->type == MOV_ARC_OUT) )
       {
           currentmov->r = r;
       }
   }
}

double path::cur_radius()
{
    if( currentmov == ml.end() )
    {
         return 0;
    }
    return currentmov->r; 
}

vec2 path::cur_end()
{
    if( currentmov == ml.end() )
    {
         return 0;
    }
    return currentmov->end; 
}

vec2 path::cur_start()
{
    if( currentmov == ml.end() )
    {
         return 0;
    }
    return currentmov->start; 
}

move_type path::cur_type()
{
    if( currentmov == ml.end() )
    {
         return MOV_NONE;
    }
    return currentmov->type; 
}

vec2 path::end()
{
    if( ml.empty() )
    {
         return vec2(0,0);
    }
    
    return ml.back().end;
}
vec2 path::start()
{
    if( ml.empty() )
    {
         return vec2(0,0);
    }
    
    return ml.front().start;
}


vec2 path::current()
{
    if( ml.empty() || currentmov == ml.end() )
    {
         return vec2(0,0);
    }
    
    return currentmov->end; 
}


void path::create_line( const vec2 &v , const move_type t, const char *comment )
{
    vec2 start;
    if( ml.empty() )
    {
        start = v;
    }
    else
    {
        start = ml.back().end;
        if( t != MOV_LINE && start == v ) 
        {
            return; // zero movement
        }
    }
    ml.push_back( mov( vec2( v.x, v.z ) , t ) );
    ml.back().start = start;
    currentmov = --ml.end();
    if( comment != NULL ) ml.back().comment = comment;
    //printf("createline %f,%f \n",v.x, v.z);
}


void path::create_arc( struct mov &c, const vec2 v1, const vec2 v2, const double r, const bool side, move_type mtype )
{

    double x1 = v1.x;
    double y1 = v1.z;
    double x2 = v2.x;
    double y2 = v2.z;
    double q = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    double x3 = (x1+x2)/2.0f;
    double y3 = (y1+y2)/2.0f;

    double cx;
    double cy;

    double u = r*r - q*q/4.0;

    if( side )
    {
        cx = x3 + sqrt(u)*(y1-y2)/q;
        cy = y3 + sqrt(u)*(x2-x1)/q;
    }
    else
    {
        cx = x3 - sqrt(u)*(y1-y2)/q;
        cy = y3 - sqrt(u)*(x2-x1)/q;
    }

    c.center.x = -cx;
    c.center.z = cy;

    double start_angle = atan2(y1 - cy, x1 - cx);
    double dot = (x1-cx)*(x2-cx) + (y1-cy)*(y2-cy);
    double det = (x1-cx)*(y2-cy) - (y1-cy)*(x2-cx);
    double angle = atan2(det, dot);

    int num_segments = fabs( angle )*5.0f*r;
    if( num_segments < 3 ) num_segments = 3;

    double theta = angle / double(num_segments - 1);
    double tangetial_factor = tanf(theta);
    double radial_factor = cosf(theta);

    double x = r * cosf(start_angle);
    double y = r * sinf(start_angle);
    for(int ii = 0; ii < num_segments; ii++)
    {
        create_line( vec2(x + cx, y + cy), mtype );
        double tx = -y;
        double ty = x;
        x += tx * tangetial_factor;
        y += ty * tangetial_factor;
        x *= radial_factor;
        y *= radial_factor;
    }
}

void path::rapid_move( const vec2 v )
{
    double r = stockdiameter/2.0f + retract;
   // if( side == OUTSIDE && r < ml.back().end.x ) r = ml.back().end.x;
    if( r < v.x ) r = v.x;
    if( side == INSIDE )
    {
		r = min.x - retract;
	}
    create_line( vec2( r, ml.back().end.z ), MOV_RAPID );
    create_line( vec2( r, v.z ), MOV_RAPID );
    create_line( v , MOV_RAPID );
}


void path::rapid_move_and_feed_close( path &nextp )
{
	
	vec2 s = end();
	vec2 d = nextp.start();
	
	if( side == INSIDE )
	{
		d.x -= retract;
		d.z += retract;
	}
	else
	{
		d.x += retract;
		d.z += retract;
	}
	
	rapid_move( d );
	create_line( nextp.start(), MOV_FEED );
	
}



void path::feed( path &colp, vec2 v, double len, const vec2 dir, const vec2 ret)
{
    list<struct mov>::iterator fi = colp.ml.begin();
    feed( colp, fi, v, len, dir, ret );
}

void path::feed( path &colp, list<struct mov>::iterator fi, vec2 v, double len, const vec2 dir, const vec2 ret )
{
    vec2 v2;
    
    list<struct mov>::iterator ci;
    colp.find_intersection( v, v + dir * len , v2, fi, ci, false );
    
    create_line( v, MOV_FEED );
    create_line( v2 , MOV_FEED );
    create_line( v2 + ret , MOV_FEED );
    create_line( v + ret, MOV_RAPID );
    create_line( v, MOV_FEED );
}


bool path::find_intersection( const vec2 a, const vec2 b, vec2 &cv,
                                list<struct mov>::iterator fi, list<struct mov>::iterator &ci, bool first )
{

    bool col = false;
    vec2 colv = b;
    double mindist = 1000000000;

    for( ;fi != ml.end(); fi++)
    {
        if( get_line_intersection( a,b , fi->start, fi->end, cv ) )
        {
            
            if( first )
            {
                ci = fi;
                return true;
            }
            
            col = true;
            if( mindist > a.dist_squared( cv ) )
            {
                colv = cv;
                ci = fi;
                mindist = a.dist_squared( cv );
            }
            
        }
    }

    cv = colv;
    return col;
}


double path::distance( const vec2 p1, const vec2 p2)
{
    double mindist = 1000000;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        double d = Segment_to_Segment( p1, p2, i->start, i->end );
        if( d < mindist ) mindist = d ;
    }
    return mindist;
}


void path::remove_knots()
{
    //printf("find knots...\n");
    vec2 vi(0,0);

    list<struct mov>::iterator i2,ie;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        if( find_intersection( i->start, i->end, vi, std::next(i,2), i2, false ) )
        {
            //printf("intersec %f %f \n",vi.x,vi.z);
            ie = i;
            ie++;
            if( ie != ml.end())
            {
                ml.erase( ie, i2 );
                i->end = vi;
                i2->start = vi;
            }
            i->vel = vi;
        }
    }
}

void path::create_from_shape( path &c )
{
    ml.clear();
    struct mov cutp( vec2(0,0),MOV_NONE );
    for(list<struct mov>::iterator i = c.ml.begin(); i != c.ml.end(); i++)
    {  
        if( i->type == MOV_LINE )
        {
            create_line( i->end, MOV_CONTOUR );
        }
        
        else if( i->type == MOV_ARC_OUT || i->type == MOV_ARC_IN )
        {
            double r2 = fabs(i->r);
            double l = i->end.dist( i->start ) + 0.00001f;
            if( r2 < l/2.0f )
            {
                i->r = l/2.0f;
            }  
            create_arc( *i, i->start, i->end , i->r, i->type == MOV_ARC_OUT ? OUTSIDE:INSIDE, MOV_CONTOUR );
        }
              
    }
    findminmax();
}

void path::draw( color c )
{
    
    for(list<struct mov>::iterator i = ++(ml.begin()); i != ml.end(); i++)
    {

        if( c != NONE )
        {
            setcolor( c );
        }
        else if( temporary )
        {
            setcolor( DISABLED );
        }
        else
        {
            if( i->type == MOV_FEED )
            {
                setcolor( FEED );
            }
            else if( i->type == MOV_CONTOUR )
            {
                setcolor( CONTOUR_LINE );
            }
            else
            {
                setcolor( RAPID );
            }
        }
        

        glBegin(GL_LINES);

            glVertex2f( i->start.z, -i->start.x );
            glVertex2f( i->end.z, -i->end.x );

            if( i != ml.begin() && 0 ) // draw normals
            {
                glVertex2f( i->end.z, -i->end.x );
                vec2 v( i->start.z, -i->start.x );
                v = v.normal( vec2(i->end.z, -i->end.x) );
                glVertex2f( i->end.z + v.x,  -i->end.x + v.z );
            }

            if( i->type == MOV_CONTOUR )
            {
    
                glVertex2f( i->start.z, i->start.x );
                glVertex2f( i->end.z, i->end.x );
                
            }

        glEnd();
        
    }

}



void path::drawshadows( color c )
{
    
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {

        if( c != NONE )
        {
            setcolor( c );
        }
        
        glBegin(GL_LINES);
            glVertex2f( i->start.z, i->start.x );
            glVertex2f( i->start.z, -i->start.x );
            glVertex2f( i->end.z, i->end.x );
            glVertex2f( i->end.z, -i->end.x );          
        glEnd();
        
        if( i->type == MOV_ARC_IN || i->type == MOV_ARC_OUT )
        {
            setcolor( CROSS );
            drawCross( i->center.z, i->center.x, 3.0/scale );
            drawCross( i->center.z, -i->center.x, 3.0/scale );
        }

    }

}


void path::move( vec2 m )
{
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->end += m;
        i->start += m;
    }
    findminmax();
}


void path::save( FILE *fp )
{
    if (fp == NULL) return;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {

        strbuf[0] = 0;
        if( ! i->comment.empty() )
        {
            sprintf( strbuf, "( %s )", i->comment.c_str() );
        }
        fprintf(fp, "G%s X%.7g Z%.7g%s\n",
            i->type == MOV_FEED ? "1":"0" ,
            (fabs( i->end.x ) < 0.001 ? 0:i->end.x),
            (fabs( i->end.z ) < 0.001 ? 0:i->end.z),
            strbuf
        );
    }
}

void path::savemoves( FILE *fp )
{
    if (fp == NULL) return;
    fprintf(fp, "MOVES\n");
    //fprintf(fp, "   SIDE %i\n", (int)side );
        
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        switch( i->type )
        {
            case MOV_NONE:
            case MOV_FEED:
            case MOV_RAPID:
            case MOV_CONTOUR:
            break;
            
            case MOV_LINE:
                fprintf(fp, "    LINE %.10g %.10g\n", i->end.x , i->end.z );
            break;
            
            case MOV_ARC_OUT:
                fprintf(fp, "    ARC_OUT %.10g %.10g %.10g\n", i->end.x , i->end.z, i->r);
            break;  
            
            case MOV_ARC_IN:
                fprintf(fp, "    ARC_IN %.10g %.10g %.10g\n", i->end.x , i->end.z, i->r);
            break;
        }
    }
        
    fprintf(fp, "ENDMOVES\n" );
}

void path::loadmoves( FILE *fp )
{
    if (fp == NULL) return;
    ml.clear();
        
    char *line = NULL;
    size_t len = 0;   
    ssize_t read;
    char tag[BUFFSIZE+1];
    
    double v1,v2,v3,v4,v5,v6;
    
    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("load %s", line);
        v1 = v2 = v3 = v4 = v5 = v6 = 0;
        
        sscanf(line, "%s %lf %lf %lf %lf %lf %lf", tag, &v1, &v2, &v3, &v4, &v5, &v6 );
            
        if( strcmp( tag, "LINE") == 0 )
        {
            create_line( vec2( v1, v2 ), MOV_LINE );
        }
        
        else if( strcmp( tag, "ARC_OUT") == 0 )
        {
            create_line( vec2( v1, v2 ), MOV_ARC_OUT );
            currentmov->r = v3;
        }      
        
        else if( strcmp( tag, "ARC_IN") == 0 )
        {
            create_line( vec2( v1, v2 ), MOV_ARC_IN );
            currentmov->r = v3;
        }   
               
        free(line);
        line = NULL;
        
        if( strcmp( tag, "ENDMOVES" ) == 0 )
        {
            break;
        }      
        
    } 
}


void path::findminmax()
{
    if( ml.empty() ) return;
    
    min = 1000000000;
    max = -1000000000;
    vec2 start =  ml.front().end;
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->end.findminmax( min, max );
        i->start = start;
        start =  i->end;
    }
}


void path::copy( path &s, move_type t )
{
    for( list<struct mov>::iterator i = s.ml.begin(); i != s.ml.end(); i++)
    {
        if( i->start.dist( i->end ) > 0.001 || i == s.ml.begin() )
        {
            create_line( i->end, t == MOV_NONE ? i->type:t );
            ml.back().start = i->start;
            ml.back().end = i->end;
        }

    }
}



void path::buffer( double r, Side s )
{
    
    vec2 n;
    path tp;
    ml.clear();
    side = s;
    
    // copy to temp list
    tp.copy( *this );
    ml.clear();

    // move normal direction
    bool first = true;
    for(list<struct mov>::iterator i = ++(tp.ml.begin()); i != tp.ml.end(); i++)
    {
        n = i->start.normal( i->end ) * r;
        
        if( side == INSIDE )
        {
            n = -n;
        }
        
        i->start += n;
        i->end += n;
        
        if( first ) // move a start point too.
        {
            first = false;
            tp.ml.front().start += n;
            tp.ml.front().end += n;
        }
        
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
    struct mov cutp( vec2(0,0),MOV_NONE );

    i2 = ++(tp.ml.begin());
    for( list<struct mov>::iterator i1 = tp.ml.begin(); i1 != tp.ml.end(); i1++,i2++)
    {

        create_line( i1->end, i1->type );
        if(  i1 == tp.ml.begin() )  start = i1->start; // first start

        double l = i1->end.dist( i2->start ) ;
        if( l > 0.01 && i2 != tp.ml.end() )
        {

            if( l > 0.2 )
            {
                double r2 = fabs(r);
                double l = i1->end.dist( i2->start ) + 0.00001f;
                if( r2 < l/2.0f )  r2 = l/2.0f;
                create_arc( cutp, i1->end, i2->start , r2, (r < 0), i1->type );
            }
            else
            {
                create_line( i2->start, i1->type );
            }
        }
    }

    // calc start/ends
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->start = start;
        start =  i->end;
    }

    remove_knots();
    findminmax();

}



void path::create_from_contour( path &c, double r, Side s, move_type mtype )
{
    
    vec2 n;
    path tp;
    ml.clear();
    side = s;
    
    // copy to temp list
    for(list<struct mov>::iterator i = c.ml.begin(); i != c.ml.end(); i++)
    {
        if( i->start.dist( i->end ) > 0.001 || i == c.ml.begin() )
        {
            tp.create_line( i->end, mtype );
            tp.ml.back().start = i->start;
            tp.ml.back().end = i->end;
        }

    }

    // move normal direction
    bool first = true;
    for(list<struct mov>::iterator i = ++(tp.ml.begin()); i != tp.ml.end(); i++)
    {
        n = i->start.normal( i->end ) * r;
        
        if( side == INSIDE )
        {
            n = -n;
        }
        
        i->start += n;
        i->end += n;
        
        if( first ) // move a start point too.
        {
            first = false;
            tp.ml.front().start += n;
            tp.ml.front().end += n;
        }
        
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
    
    struct mov cutp( vec2(0,0),MOV_NONE );

    i2 = ++(tp.ml.begin());
    for( list<struct mov>::iterator i1 = tp.ml.begin(); i1 != tp.ml.end(); i1++,i2++)
    {

        create_line( i1->end, mtype );
        if(  i1 == tp.ml.begin() )  start = i1->start; // first start

        double l = i1->end.dist( i2->start ) ;
        if( l > 0.01 && i2 != tp.ml.end() )
        {

            if( l > 0.2 )
            {
				vec2 v1 = i1->start - i1->end;
				vec2 v2 = i2->start - i2->end;
				double a = v1.angle(v2);
                double r2 = fabs(r);
                double l = i1->end.dist( i2->start ) + 0.00001f;
                if( r2 < l/2.0f )  r2 = l/2.0f;
                create_arc( cutp, i1->end, i2->start , r2, a > 0 , mtype);
            }
            else
            {
                create_line( i2->start, mtype );
            }
        }
    }

    // calc start/ends
    for(list<struct mov>::iterator i = ml.begin(); i != ml.end(); i++)
    {
        i->start = start;
        start =  i->end;
    }

    remove_knots();
    findminmax();

}


void path::create_rectangle( const tool &tl, vec2 begin, vec2 end, int feedd )
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

void path::create_rough_from_contour( path &c, const tool &tl, Side s )
{
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    
    side = s;
    path tc;
    tc.create_from_contour( c, tool_r, side, MOV_CONTOUR );
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
            feed( tc, vec2( x, max_z ), len, vec2( 0,-1 ), vec2( 1,0 ) );
            x -= tl.depth;
        }
        
    }
    
    else if( side == INSIDE )
    {
        x = tc.min.x + tl.depth;
       // x = 0;
        create_line( vec2( x, max_z ), MOV_RAPID );
        
        while( x < tc.ml.front().start.x - 0.001 )        {
            feed( tc, vec2( x, max_z ), len, vec2( 0,-1 ), vec2( -1,0 ) );
            x += tl.depth;
        }
        
    }
    
    //move( tool_cpoint( tl.tooln ) ); 
    findminmax();
        
}

void path::create_Xfeed_from_contour( path &c, const tool &tl, Side s )
{
    
    double tool_r = _tools[ tl.tooln ].diameter/2.0f;
    
    if( tool_r <= 0.0 )
		{
			tool_r = 0.1; 
		}
		
    
    side = s;
    path tc;
    tc.create_from_contour( c, tool_r, side, MOV_CONTOUR );
    tc.temporary = true;
    
    double z,x;
    double min_x = tc.min.x;
    double max_x = tc.max.x;
    
    max_x = fmax( max_x, stockdiameter/2.0 ) + tool_r + retract;
    
    double len = fabs( min_x - max_x );
    ml.clear();
    
    if( tc.ml.empty() )
    {
        return;
    }
    
  
    if( side == OUTSIDE )
    {
		z = tc.max.z - 0.1;
        x = max_x;
       // create_line( vec2( x, z ), MOV_RAPID );
        
        while( 1 )
        {
            feed( tc, vec2( x, z ), len, vec2( -1,0 ), vec2( 1,0 ) );
            z -= tool_r*2.0;
            if( z <= tc.min.z )
            {
				  z = tc.min.z + 0.1;
				  feed( tc, vec2( x, z ), len, vec2( -1,0 ), vec2( 1,0 ) );
				  break; 
			}
        }
        
    }
    
    else if( side == INSIDE )
    {
     
        
    }
    //printf( "move %f,%f\n",tool_cpoint( tl.tooln ).x ,tool_cpoint( tl.tooln ).z); 
    //move( tool_cpoint( tl.tooln ) ); 
    findminmax();
        
}
void path::create_undercut_from_contour( path &c, const tool &tl, Side s )
{
    side = s;
    double depth = tl.depth;
    double tool_r =  _tools[ tl.tooln ].diameter/2.0f;
    path tc;
    tc.create_from_contour( c, tool_r, side, MOV_CONTOUR );
    tc.temporary = true;
    
    bool up = false;

    double x = tc.ml.begin()->start.x;
    ml.clear();
    if( ! tc.ml.empty() )
    {
        for(list<struct mov>::iterator i = tc.ml.begin(); i != tc.ml.end(); i++)
        {
    
            if( i->end.x > i->start.x ) // uphill
            {
                while( x < i->end.x )
                {
                    x += depth;
                }
                up = true;
            }
            else  // downhill
            {
                while( x > i->start.x )
                {
                     x -= depth;
                }
                
            
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
                                rapid_move( vec2( x+1, z )  );
                                create_line( vec2( x, z ), MOV_FEED );
                                up = false;
                            }
                           
                            feed( tc, std::next(i,1), vec2( x, z ), fabs( c.min.z -z ), vec2( 0,-1 ), vec2( 1,0 ) );
                        }
                        x -= depth;
                    }
                    
                }
            }
        }
    }
    
  //  move( tool_cpoint( tl.tooln ) ); 
    findminmax();
}

