#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;

#define width end.z

op_grooving::op_grooving()
{

    begin.z = 0;
    begin.x = 10;
    end.z = 0;
    end.x = 0;   
    peck = 0;
    finish = 0;
    
    Tool = NULL;

    tagl.push_front( ftag( "BEGIN_Z", &begin.z ) );
    tagl.push_front( ftag( "BEGIN_X", &begin.x ) );
    tagl.push_front( ftag( "WIDTH", &width ) );
    tagl.push_front( ftag( "END_X", &end.x ) );
    tagl.push_front( ftag( "PECK", &peck ) );
	tagl.push_front( ftag( "FINISH", &finish ) );

    createmenu();
}

op_grooving::~op_grooving()
{
   tagl.clear();
   Menu.clear();
}

const char* op_grooving::name()
{
    sprintf( Name, "Grooving: Z%.8g D%.8g to z%.8g D%.8g peck %.8g", begin.z, begin.x*2.0, begin.z+end.z, end.x*2.0 , peck );
    return Name;
}

op_type op_grooving::type()
{
    return GROOVING;
}

void op_grooving::draw( color c , bool drawpath )
{
    if( Tool != NULL)
    {
        if( ! Tool->tl.csspeed )
        {
            setcolor( c == NONE ? CONTOUR_LINE:c );
            double w = _tools[ Tool->tl.tooln ].diameter;
            if ( w < width ) w = width;
            drawBox( vec2(begin.x, begin.z), vec2( end.x, begin.z - w) );
            drawBox( vec2(-begin.x, begin.z), vec2( -end.x, begin.z - w) );
        }
    }
}

void op_grooving::save_program( FILE *fp )
{
	double toolw = _tools[ Tool->tl.tooln ].diameter;
	
	if( end.z < toolw ) end.z = toolw;
	
	double x = begin.x;
    double z = begin.z - toolw;
    double endz = begin.z - end.z;
    double endx = end.x; 
    
    if( end.z > toolw )
    {
		z -=finish;
		endz += finish;
		endx += finish; 
	}
	
    fprintf(fp, "(%s)\n", name() );
    
    while( z >= endz )
    {
		
		x = begin.x;
		fprintf(fp, "G0 X%.8g\n", x + retract );
        fprintf(fp, "G0 Z%.8g\n", z );
        
		while( x > endx )
		{
			x -= ( peck <= 0 ? 1000:peck ); // 0 -> no pecking
			if( x < endx ) x = endx;
			fprintf(fp, "G1 X%.8g\n", x );
			fprintf(fp, "G0 X%.8g\n", begin.x + retract );
		}
		
		if( z <= endz ) break;
		z -= toolw;
		if( z < endz ) z = endz;
	}
	
	
	if( end.z > toolw )
	{
		double midz = ( begin.z - toolw + begin.z - end.z ) /2.0;
		fprintf(fp, "G0 X%.8g\n", begin.x + retract );
		fprintf(fp, "G0 Z%.8g\n", begin.z - toolw);
		fprintf(fp, "G1 X%.8g\n", end.x );
		fprintf(fp, "G1 Z%.8g\n", midz );
		
		fprintf(fp, "G0 X%.8g\n", begin.x + retract );
		fprintf(fp, "G0 Z%.8g\n", begin.z - end.z);
		fprintf(fp, "G1 X%.8g\n", end.x );
		fprintf(fp, "G1 Z%.8g\n", midz );
		
		fprintf(fp, "G0 X%.8g\n", begin.x + retract );
	}
    
    fprintf(fp, "G0 X%.8g\n", begin.x + retract );
}

#define MENU_BACK 1

int op_grooving::parsemenu()
{
    Menuselect = 0;
    if( Menu.parse() )
    {
        if( Menuselect == MENU_BACK )
        {
            return OP_EXIT;
        }
        CLAMP( peck  , 0, 1000 );
        CLAMP( end.x , 0, begin.x );
        CLAMP( width , _tools[ Tool->tl.tooln ].diameter, 1000 );
        CLAMP( finish  , 0, 1 );
        
    }
    return OP_NOP;
}

void op_grooving::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( name() );
        Menu.select(&Menuselect, MENU_BACK, "Back" );

        if( Tool == NULL )
        {
            Menu.comment( "Missing Tool!" );
        }
        else
        {
            Menu.edit( &begin.x, "Start diameter " ); Menu.diameter_mode();
            Menu.edit( &end.x,   "End diameter   " ); Menu.diameter_mode();
            Menu.edit( &begin.z, "Start Z        " );
            Menu.edit( &end.z,   "Width          " );
            Menu.edit( &peck,    "Pecking " );
            Menu.edit( &finish,  "Finish  " );
        }

    Menu.end();
}

void op_grooving::drawmenu(int x,int y)
{
    Menu.draw(x,y);
}

void op_grooving::update()
{

}

