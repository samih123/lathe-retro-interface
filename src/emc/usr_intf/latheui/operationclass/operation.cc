#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;



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
        setcolor( GREEN );
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
        setcolor( GREEN );
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

operation::operation( int t )
{
    cl.clear();
    scale = 1;
    pos.x = pos.z = 0;
    type = t;
    inside = false;
    if( type == CONTOUR )
    {
      //  cl.clear();
      //  new_cut( vec2(13,12), CUT_LINE );
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
        setcolor( GREY );
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
                    setcolor( GREY );
                    glVertex2f( x2, -y2 );
                    glVertex2f( x2, y2  );
                    glVertex2f( x1, -y1 );
                    glVertex2f( x1, y1  );
                glEnd();

                if(i->type == CUT_THREAD )
                {
                    setcolor( GREEN );
                    draw_thread( x1,  y1,  x2,  y2, i->pitch, i->depth );
                }

                if( i->type == CUT_ARC_IN || i->type == CUT_ARC_OUT )
                {
                    setcolor( GREEN );
                    drawCross( i->center.z, i->center.x, 3.0f / scale );
                    drawCross( i->center.z, -i->center.x, 3.0f / scale );
                }

                if( i == currentcut )
                {
                    setcolor( RED );
                    drawCircle( x2, y2, 3.0f / scale );
                }
            }

        }

        create_contour( contour );
        contour.draw( true );
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

}


void operation::erase()
{
    if( cl.size() > 1 )
    {
         currentcut =  cl.erase( currentcut );
         if( currentcut == cl.end() ) currentcut--;
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


void operation::set_cut( cut &c )
{
    if( type == CONTOUR && cl.size() > 0 )
    {

        if( currentcut->type != CUT_BEGIN )
        {
            CLAMP( c.type, CUT_BEGIN+1 , CUT_END-1 );
            currentcut->type = c.type;
        }

       // if( c.end.x < 0 ) c.end.x = 0;

        vec2 d = c.end - currentcut->end;
        currentcut->end += d;

        list<struct cut>::iterator i = currentcut;
        i++;

        i->start = currentcut->end;
        /*
        for(; i != cl.end(); i++)
        {
            i->end += d;
            i->start += d;
        }
        */

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

void operation::load( FILE *fp )
{
    if (fp == NULL) return;
    
    char *line = NULL;
    size_t len = 0;   
    ssize_t read;
    char tag[BUFFSIZE+1];
    double val=0;

    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("load %s", line);
        
        sscanf(line, "%s %lf", tag, &val );

        if( type == TOOL )
        {
            findtag( tag, "DEPTH", tl.depth, val );
            findtag( tag, "FEED",  tl.feed, val );
            findtag( tag, "SPEED", tl.speed, val );
            findtag( tag, "TOOLN", tl.tooln, val );
            findtag( tag, "COUNT", tl.count, val );
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
    
    p.ml.clear();
  //  vec2 start(0,0);

    for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
    {

      //  if( i->type != CUT_BEGIN )
        {
            if( i->type == CUT_ARC_IN || i->type == CUT_ARC_OUT )
            {
                double l = i->start.dist( i->end ) + 0.00001f;
                if( i->r < l/2.0f )  i->r = l/2.0f;
                p.create_arc( *i, i->start, i->end, i->r, i->type == CUT_ARC_IN );
            }
            else
            {
                p.create_line( i->end, i->type );
            }
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

    ccontour.create_contour( contour );
    r_path.create( ccontour.contour, ctool.get_tool() );

}














