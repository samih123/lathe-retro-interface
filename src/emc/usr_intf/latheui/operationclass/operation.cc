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

    for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
    {

        x1 = i->start.z;
        y1 = -i->start.x;
        x2 = i->end.z;
        y2 = -i->end.x;

       // if( i != cuts.begin() )
        {
            glBegin(GL_LINES);
                setcolor( GREY );
                glVertex2f( x2, -y2 );
                glVertex2f( x2, y2  );
                glVertex2f( x1, -y1 );
                glVertex2f( x1, y1  );
            glEnd();

            if(i->type == THREAD )
            {
                setcolor( GREEN );
                draw_thread( x1,  y1,  x2,  y2, i->pitch, i->depth );
            }

            if( i->type == ARC_IN || i->type == ARC_OUT )
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

    contour.draw( true );
}

void operation::new_cut( vec2 p, cut_type t )
{
    vec2 start(0,0);
    if( ! cl.empty() )
    {
       start = cl.back().end;
    }
    cl.push_back( cut( cl.back().dim.x + p.x, p.z) );
    currentcut = --cl.end();
    currentcut->type = t;
    currentcut->start = start;
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
    if( currentcut != cl.end() )
    {
        currentcut++;
    }
}

void operation::save( const char *name ) 
{
    
}

void operation::load( const char *name )
{
    
}



void operation::create_contour( contour_path &p )
{
    p.ml.clear();

    vec2 start(0,0);

    for(list<struct cut>::iterator i = cl.begin(); i != cl.end(); i++)
    {

       i->start = start;

       i->end.z = start.z + i->dim.z;
       i->end.x = i->dim.x;

       start =  i->end;

        if( i->type == ARC_IN || i->type == ARC_OUT )
        {
            double l = i->start.dist( i->end ) + 0.00001f;
            if( i->r < l/2.0f )  i->r = l/2.0f;
            p.create_arc( *i, i->start, i->end, i->r, i->type == ARC_IN );
        }
        else 
        {
            p.create_line( i->end, i->type );
        }

    }

    //p.remove_knots();
    p.findminmax();
    
}



