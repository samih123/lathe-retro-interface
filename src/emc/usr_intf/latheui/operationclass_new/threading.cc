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
    degression = 2;
    spring_passes = 1;
    
    Tool = NULL;
    
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

void op_threading::draw( color c, bool drawpath )
{
    if( Tool != NULL)
    {
        if( ! Tool->tl.csspeed )
        {
            setcolor( c == NONE ? CONTOUR_LINE:c );
            draw_thread( begin.z, -begin.x, end.z, -end.x, pitch, depth );
        }
    }
}

void op_threading::save_program( FILE *fp )
{
    if( Tool != NULL)
    {
        double tool_r = _tools[ Tool->tl.tooln ].diameter/2.0f;
        vec2 dv = vec2( tool_r, tool_r ) + tool_cpoint( Tool->tl.tooln );
        vec2 v1 = begin + dv;
        vec2 v2 = end + dv;
        
        double Z = v2.z;
        double P = pitch;
        double Q = compound_angle;
        double I = -retract;
        double K = depth;
        double J = Tool->tl.depth;
        double R = degression;
        int H = spring_passes;
        
        
        fprintf(fp, "(%s)\n", name() );
        fprintf(fp, "G0 X%.8g Z%.8g\n", v1.x + retract, v1.z );
        fprintf(fp, "G76 " );
        fprintf(fp, "Z%.8g ", Z );
        fprintf(fp, "P%.8g ", P );
        fprintf(fp, "Q%.8g ", Q );
        fprintf(fp, "I%.8g ", I );
        fprintf(fp, "K%.8g ", K );
        fprintf(fp, "J%.8g ", J );
        fprintf(fp, "R%.8g ", R );
        fprintf(fp, "H%i\n", H );
        
    }
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
            CLAMP( degression, 1, 3 );
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
        else if( Tool->tl.csspeed )
        {
            Menu.comment( "Dont use constant surface speed!" );
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
