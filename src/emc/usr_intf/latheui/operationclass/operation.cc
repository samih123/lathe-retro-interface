#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;
extern char *ttcomments[CANON_POCKETS_MAX];

const char* operation_name( int t )
{
    switch( t )
    {
        case TOOL:            return "Tool";
        case CONTOUR:         return "Contour";
        case INSIDE_CONTOUR:  return "Inside contour";
        case TURN:            return "Turning";
        case UNDERCUT:        return "Undercut";
        case FINISHING:       return "Finishing";
        case THREADING:       return "Thread";
        case FACING:          return "Simple Box";
        case DRILL:           return "Drilling";
        case PARTING:         return "Parting off";
        case MOVE:            return "Rapid move";
    }

    return "ERROR";
};

const char* operation::get_name()
{  
    
    if( type == CONTOUR && side == INSIDE )
    {
        return operation_name( INSIDE_CONTOUR );
    }   
    
    if( type == TOOL && tl.tooln > 0 && tl.tooln < CANON_POCKETS_MAX)
    {
        sprintf( name, "%s %s", operation_name( type ), ttcomments[ tl.tooln ]);
    }

    return name;
    
};


static void draw_thread(double x1, double y1, double x2, double y2, double pitch, double depth )
{
    double dx =  x1 - x2;
    double dy =  y1 - y2;
    double l = sqrtf( dx*dx + dy*dy );
    if( l == 0.0f ) return;

    int n = l / pitch;

    vec2 v(x1,y1);
    v = v.normal( vec2(x2,y2) ) * depth;

    double nx = v.x;
    double ny = v.z;

    double dl = 1.0f / l;
    dl *= l / (double)n;

    dx *= dl;
    dy *= dl;

    double x = x1;
    double y = y1;
    glBegin(GL_LINE_STRIP);
        setcolor( CONTOUR_LINE );
        glVertex2f( x1, y1 );
        for( int i=0 ; i < n-1 ; i++ )
        {
            x -= dx;
            y -= dy;
            if( i & 1 )
            {
                glVertex2f( x , y );
            }
            else
            {
                glVertex2f( x - nx, y - ny );
            }

        }
        glVertex2f( x2, y2 );
    glEnd();

    x = x1;
    y = y1;
    glBegin(GL_LINE_STRIP);
        setcolor( CONTOUR_LINE );
        glVertex2f( x1, -y1 );
        for( int i=0 ; i < n-1 ; i++ )
        {
            x -= dx;
            y -= dy;
            if( i & 1 )
            {
                glVertex2f( x , -y );
            }
            else
            {
                glVertex2f( x - nx, -(y - ny) );
            }

        }
        glVertex2f( x2, -y2 );
    glEnd();

}

operation::operation( op_type t )
{
    cl.clear();
    
    type = t;
    side = OUTSIDE;
    changed = true;
   
    strcpy( name, operation_name(t) );
    printf("name =%s\n",name);
    if( type == INSIDE_CONTOUR )
    {
        side = INSIDE;
        type = CONTOUR;
    }
    
    if( type == FACING )
    {
        face_begin.z = 5;
        face_end.z = 0;
        face_begin.x = stockdiameter/2.0;
        face_end.x = 0;
        feed_dir = DIRX;
    }

}

operation::~operation()
{

}


void operation::draw( color c, bool draw_all )
{

    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;
    

    if( type == CONTOUR )
    {
        for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
        {

            x1 = i->start.z;
            y1 = -i->start.x;
            x2 = i->end.z;
            y2 = -i->end.x;

            {
                

                glBegin(GL_LINES);
                    setcolor( CONTOUR_SHADOW );
                    glVertex2f( x2, -y2 );
                    glVertex2f( x2, y2  );
                    glVertex2f( x1, -y1 );
                    glVertex2f( x1, y1  );
                glEnd();
                
                if(i->type == CUT_THREAD )
                {
                    draw_thread( x1,  y1,  x2,  y2, i->pitch, i->depth );
                }

                if( i->type == CUT_ARC_IN || i->type == CUT_ARC_OUT )
                {
                    setcolor( CROSS );
                    if( draw_all )
                    {
                        drawCross( i->center.z, i->center.x, 3.0/scale );
                        drawCross( i->center.z, -i->center.x, 3.0/scale );
                    }
                }

                if( i == currentcut && draw_all )
                {
                    setcolor( WARNING );
                    drawCircle( x2, y2, 3.0/scale );
                }
            }

        }

        create_contour();
        contour.draw( c );
        
        if( side == OUTSIDE )
        {
            glBegin(GL_LINES);
                setcolor( DISABLED );
                glVertex2f( x2,  (stockdiameter/2.0) );
                glVertex2f( x2, -(stockdiameter/2.0) );
                glVertex2f( x2, -(stockdiameter/2.0) );
                glVertex2f( -1000, -(stockdiameter/2.0) );
                glVertex2f( x2, (stockdiameter/2.0) );
                glVertex2f( -1000, (stockdiameter/2.0) );
            glEnd();
        }
        
    }

    if( draw_all )
    {
        
        if( type == TURN )
        {
            r_path.draw( c );
        }
        
        else if( type == FACING )
        {
            setcolor( DISABLED );
            drawBox( face_begin, face_end );
            drawCross( face_begin.z, -face_begin.x , 3.0/scale);
            drawCircle( face_begin.z, -face_begin.x , 3.0/scale);
            f_path.draw( c );
        }   
        
        else if( type == MOVE )
        {
            setcolor( WARNING );
            drawCross( rapid_move.z, -rapid_move.x , 3.0/scale);
        }         
          
    }


}

void operation::new_cut( vec2 p, cut_type t )
{
    vec2 end(0,0);

    if( ! cl.empty() )
    {
       end = cl.back().end;
    }

    cl.push_back( cut() );

    currentcut = --cl.end();
    currentcut->type = t;
    currentcut->start = end;
    currentcut->end = end+p;
    currentcut->r = 1.0f;
    currentcut->pitch = 1.0f;
    currentcut->depth = 0.65f * currentcut->pitch;
    changed = true;
}


void operation::erase()
{
    if( cl.size() > 1 )
    {
         currentcut =  cl.erase( currentcut );
         if( currentcut == cl.end() ) currentcut--;
         changed = true;
    }
}

void operation::previous()
{
    if(  currentcut != cl.begin() )
    {
        currentcut--;
    }
}

void operation::next()
{
    if( currentcut != --cl.end() )
    {
        currentcut++;
    }
}



cut operation::get_cut()
{
    if( type == CONTOUR && cl.size() > 0 )
    {
        return *currentcut;
    }
    return cut();
}

double limit_radius( const double r, const list<struct cut>::iterator c )
{
    double l = c->start.dist( c->end ) + 0.00001f;
    if( r < l/2.0f )  return l/2.0f;
    return r;
}

void operation::set_cut( cut &c )
{
    if( type == CONTOUR && cl.size() > 0 )
    {

        if( currentcut->type != CUT_BEGIN )
        {
            CLAMP( c.type, CUT_BEGIN+1 , CUT_END-1 );
            currentcut->type = c.type;
        }

        if( c.end.x < 0 ) c.end.x = 0;

        vec2 d = c.end - currentcut->end;
        currentcut->end += d;

        list<struct cut>::iterator i = currentcut;
        i++;

        i->start = currentcut->end;
        
        for(; i != cl.end(); i++)
        {
            i->end.z += d.z;
            i->start.z += d.z;
        }
        
        currentcut->r = c.r = limit_radius( c.r, currentcut );//c.r;
    
        changed = true;
    }
}

void operation::setf_begin_end_dir( vec2 fbeg, vec2 fend, int d )
{ 
    if( type != FACING ) printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
    
    face_begin = fbeg;
    face_end = fend;
    changed = true;
    feed_dir = d;

}


tool operation::get_tool()
{
    if( type != TOOL ) printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
    return tl;
}

void operation::set_tool( tool &T )
{
    if( type != TOOL ) printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
    if( T.depth < 0.01 ) T.depth = 0.01;
    tl = T;
    changed = true;
}


void operation::save_program( FILE *fp )
{
    
    fprintf(fp, "(%s)\n", get_name() );
    
    if( type == TURN )
    {
        r_path.save( fp );
    }
    
    else if( type == FACING )
    {
        f_path.save( fp );
    }   
    
    else if( type == TOOL )
    {
        sprintf( strbuf, "T%d M6 F%f (change tool)\n", tl.tooln, tl.feed ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "G43 (enable tool compensation)\n" ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "G96 D%d S%f (set maxrpm & surface speed)\n", maxrpm, tl.speed ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "M4 (start spindle)\n" ); fprintf(fp, "%s", strbuf );
    }   
    
    else if( type == MOVE )
    {
        fprintf(fp, "G0 X%.10g Z%.10g\n", rapid_move.x, rapid_move.z );
    }    
    
}    
    
void operation::save( FILE *fp )
{
    if (fp == NULL) return;
    
    if( type == TOOL )
    {
        fprintf(fp, "OPERATION %i //tool\n", type );
        fprintf(fp, "   DEPTH %.10g\n", tl.depth );
        fprintf(fp, "   FEED %.10g\n", tl.feed );
        fprintf(fp, "   SPEED %.10g\n", tl.speed );
        fprintf(fp, "   TOOLN %i\n", tl.tooln );
        fprintf(fp, "END\n" );
    }
    
    else if( type == MOVE )
    {
        fprintf(fp, "OPERATION %i //rapid move\n", type );
        fprintf(fp, "   MOVE %.10g %.10g\n", rapid_move.x, rapid_move.z );
        fprintf(fp, "END\n" );
    }   
    
    else if( type == FACING )
    {
        fprintf(fp, "OPERATION %i //facing\n", type );
        fprintf(fp, "   BEGIN_END %.10g %.10g %.10g %.10g\n", face_begin.x, face_begin.z, face_end.x, face_end.z );
        fprintf(fp, "   FEED_DIR %i\n", (int)feed_dir );
        fprintf(fp, "END\n" );
    }
    
    else if( type == CONTOUR )
    {
        fprintf(fp, "OPERATION %i //contour\n", type );
        fprintf(fp, "   SIDE %i\n", (int)side );
        
        for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
        {
            vec2 dim;
            dim = i->end - i->start;

            fprintf(fp, "   CUT %i %.10g %.10g", i->type, dim.x , dim.z );
            switch( i->type )
            {
                case CUT_BEGIN:
                case CUT_LINE:
                break;
                case CUT_ARC_OUT:
                case CUT_ARC_IN:
                    fprintf(fp, " %.10g %.10g %.10g //arc", i->center.x, i->center.z, i->r);
                break;
                case CUT_THREAD:
                    fprintf(fp, " %.10g %.10g //thread", i->pitch, i->depth );
                break;
            }
            fprintf(fp, "\n" ); 
        }
        
        fprintf(fp, "END\n" );
    }
    else
    {
        fprintf(fp, "OPERATION %i\n", type );
        fprintf(fp, "END\n" );
    }
    
}


void findtag( const char *line, const char *tag, double &val,const double v )
{
    if( strcmp( tag, line ) == 0 )
    {
        val = v;
    }      
}

void findtag( const char *line, const char *tag, int &val,const int v )
{
    if( strcmp( tag, line ) == 0 )
    {
        val = v;
    }      
}

void findtag( const char *line, const char *tag, bool &val,const int v )
{
    if( strcmp( tag, line ) == 0 )
    {
        val = (bool)v;
    }      
}


void findtag( const char *line, const char *tag, char *str )
{
    
    if( strstr( line, line ) == line )
    {
        sscanf( line, "%*[^\"]\"%255[^\"]\"", str);
    }      
    
}

void operation::load( FILE *fp )
{
    if (fp == NULL) return;
    
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

        if( type == TOOL )
        {
            findtag( tag, "DEPTH", tl.depth, v1 );
            findtag( tag, "FEED",  tl.feed, v1 );
            findtag( tag, "SPEED", tl.speed, v1 );
            findtag( tag, "TOOLN", tl.tooln, v1 );
        }
        
        else if( type == FACING )
        {
            if( strcmp( tag, "BEGIN_END" ) == 0 )
            {
                face_begin.x = v1;
                face_begin.z = v2;
                face_end.x = v3;
                face_end.z = v4;
            }
            findtag( tag, "FEED_DIR", feed_dir, v1 );
        }    
        
        else if( type == MOVE )
        {
            if( strcmp( tag, "MOVE") == 0 )
            {
                rapid_move.x = v1;
                rapid_move.z = v2;
            }
        }    
                       
        else if( type == CONTOUR )
        {
            int s = (int)side;
            findtag( tag, "SIDE", s, v1 );
            side = (Side)s;
            
            if( strcmp( tag, "CUT") == 0 )
            {
                
                new_cut( vec2( v2, v3 ), (cut_type)(int)v1 );
                list<struct cut>::iterator i = --cl.end();
                
                switch( i->type )
                {
                    case CUT_BEGIN:
                    case CUT_LINE:

                    break;
                    case CUT_ARC_OUT:
                    case CUT_ARC_IN:
                        i->center.x = v4;
                        i->center.z = v5;
                        i->r = v6;
                    break;
                    case CUT_THREAD:
                        i->pitch = v4;
                        i->depth = v5; 
                    break;
                }
                
            }  
            
        }
               
        
        free(line);
        line = NULL;
        
        if( strcmp( tag, "END" ) == 0 )
        {
            break;
        }      
        
    }

}



void operation::create_contour( contour_path &p )
{
    if( type != CONTOUR )
    {
         printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
         return;
    }
    
    if( ! changed ) return;
    changed = false;
    
    printf("create contour\n");
    p.ml.clear();

    for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
    {

        if( i->type == CUT_ARC_IN || i->type == CUT_ARC_OUT )
        {
            i->r = limit_radius( i->r, i );
            p.create_arc( *i, i->start, i->end, i->r, i->type == CUT_ARC_IN, MOV_CONTOUR );
            
        }
        else
        {
            p.create_line( i->end, MOV_CONTOUR );
        }

    }

    //p.remove_knots();
    p.findminmax();

}


void operation::create_path( operation &ccontour, operation &ctool )
{
    
    if( type == FACING && changed )
    {
         f_path.create( ctool.get_tool(), face_begin, face_end, feed_dir );
         return;
    }
    
    
    if( type != TURN || ccontour.type != CONTOUR || ctool.type != TOOL )
    {
         printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
         return;
    }
    
    if( changed )
    {
        ccontour.create_contour( contour );
        r_path.create( ccontour.contour, ctool.get_tool(), ccontour.get_side() );
    }

}














