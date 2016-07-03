#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;

const char* phase_name( int t )
{

    if(t == TOOL ) return "Tool";
    else if(t == CONTOUR ) return "Contour";
    else if(t == INSIDE_CONTOUR ) return "Inside contour";
    else if(t == TURN ) return "Turning";
    else if(t == UNDERCUT ) return "Undercut";
    else if(t == FINISHING ) return "Finishing";
    else if(t == THREADING ) return "Thread";
    else if(t == FACING ) return "Facing";
    else if(t == DRILL ) return "Drilling";
    else if(t == PARTING ) return "Parting off";
    return "ERROR";
};

const char* operation::get_name()
{
    if( type == CONTOUR && side == INSIDE )
    { 
        return phase_name( INSIDE_CONTOUR );
    }
    return phase_name( type );
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
    scale = 1;
    pos.x = pos.z = 0;
    type = t;
    side = OUTSIDE;
    changed = true;
    if( type == INSIDE_CONTOUR )
    {
        side = INSIDE;
        type = CONTOUR;
    }

}

operation::~operation()
{

}


void operation::draw( int x,int y,int xs,int ys)
{

    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;

    //glEnable(GL_LINE_STIPPLE);
    //glLineStipple(1,0x5555);

    glPushMatrix();
    glTranslatef( 750 ,300 , 0);
    glScalef(scale*3.0f, scale*3.0f,scale*3.0f);
    glTranslatef( pos.x ,pos.z , 0);
    glBegin(GL_LINES);
        setcolor( CENTERLINE );
        glVertex2f( 10,0 );
        glVertex2f( -600,0 );
    glEnd();

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
                    drawCross( i->center.z, i->center.x, 3.0f / scale );
                    drawCross( i->center.z, -i->center.x, 3.0f / scale );
                }

                if( i == currentcut )
                {
                    setcolor( WARNING );
                    drawCircle( x2, y2, 3.0f / scale );
                }
            }

        }

        create_contour();
        contour.draw( true );
        
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

    if( type == TURN )
    {
        r_path.draw( true );
    }

    glPopMatrix();

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

tool operation::get_tool()
{
    if( type != TOOL ) printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
    return tl;
}

void operation::set_tool( tool &T )
{
    if( type != TOOL ) printf( "TYPE ERROR %s\n", __PRETTY_FUNCTION__ );
    tl = T;
    changed = true;
}


void operation::save_program( FILE *fp )
{
    
    if( type == TURN )
    {
        r_path.save( fp );
    }
    
    if( type == TOOL )
    {
        sprintf( strbuf, "T%d M6 F%f (change tool)\n", tl.tooln, tl.feed ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "G43 (enable tool compensation)\n" ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "G96 D%d S%f (set maxrpm & surface speed)\n", maxrpm, tl.speed ); fprintf(fp, "%s", strbuf );
        sprintf( strbuf, "M4 (start spindle)\n" ); fprintf(fp, "%s", strbuf );
        
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
        fprintf(fp, "   COUNT %i\n", tl.count );
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
            findtag( tag, "COUNT", tl.count, v1 );
        }
        
        if( type == CONTOUR )
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














