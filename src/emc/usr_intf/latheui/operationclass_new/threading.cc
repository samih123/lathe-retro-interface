#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;
extern char *ttcomments[CANON_POCKETS_MAX];


op_threading::op_threading()
{
    
    begin.z = 5;
    end.z = -10;
    begin.x = 5;
    end.x = 5;
        
    pitch = 1.5;
    multip = 0.613;
    depth = pitch * multip;
    compound_angle = 29;
    degression = 1.5;
    spring_passes = 1;
    
    tagl.push_front( ftag( "BEGIN_X", &begin.x ) );
    tagl.push_front( ftag( "BEGIN_Z", &begin.z ) );
    //tagl.push_front( ftag( "END_X", &end.x ) );
    tagl.push_front( ftag( "END_Z", &end.z ) );
    
    tagl.push_front( ftag( "PITCH", &pitch ) );
    tagl.push_front( ftag( "DEPTH",  &depth ) );
    tagl.push_front( ftag( "COMPOUND_ANGLE", &compound_angle ) );
    tagl.push_front( ftag( "DEGRESSION", &degression ) );
    tagl.push_front( ftag( "SPING_PASSES", &spring_passes ) );
    
    createmenu();
}

op_threading::~op_threading()
{
    tagl.clear();
}

const char* op_threading::name()
{
    sprintf( Name, "Threading %.8gx%.8g", begin.x*2.0, pitch );
    return Name;
}

op_type op_threading::type()
{
    return THREADING;
}

//void draw_thread(double x1, double y1, double x2, double y2, double pitch, double depth )

            
void op_threading::draw( color c, bool drawpath )
{
    if( Tool != NULL)
    {
        setcolor( c == NONE ? CONTOUR_LINE:c );
        draw_thread( begin.z, -begin.x, end.z, -end.x, pitch, depth );
    }
}

void op_threading::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );  
}

#define MENU_BACK 1


int op_threading::parsemenu()
{
    
    Menuselect = 0;
    if( Menu.parse() )
    {
        if( Menuselect == MENU_BACK )
        {
            return OP_EXIT;
        }
        
        if( Tool != NULL)
        {
            
            CLAMP( depth, 0.01, 20 );
            CLAMP( multip, 0.01, 20 );
            CLAMP( pitch, 0.01, 1000 );
            CLAMP( compound_angle, 0, 90 );
            CLAMP( degression, 1, 2 );
            CLAMP( spring_passes, 0, 10 );
            
            end.x = begin.x;
            
            if( Menu.edited( &multip) )
            {
                depth = pitch * multip;
            }
            if( Menu.edited( &depth ) )
            {
                multip = depth / pitch;
            }        
            
            return OP_EDITED;
        }
    }
    return OP_NOP;
}

void op_threading::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    multip = pitch / depth;
    Menu.begin( "Threading" );
        Menu.select(&Menuselect, MENU_BACK, "Back" );
        if( Tool == NULL )
        {
            Menu.comment( "Missing Tool!" );
        }
        else
        {
            Menu.edit( &begin.x , "Diameter " );Menu.diameter_mode();
            Menu.edit( &begin.z, "Begin Z" );
            Menu.edit(&end.z, "End Z" );
            Menu.edit( &pitch, "Pitch " );
            Menu.edit( &multip, "Depth multiplier " );
            Menu.edit( &depth, "Depth " );
            Menu.edit( &compound_angle, "Compound angle " );
            Menu.edit( &degression, "Degression ") ;
            Menu.edit( &spring_passes, "Spring passes " );
        }
    Menu.end();
} 

void op_threading::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_threading::update()
{
}
