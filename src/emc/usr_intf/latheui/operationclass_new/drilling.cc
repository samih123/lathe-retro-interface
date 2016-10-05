#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;


op_drilling::op_drilling()
{   
    
    begin.z = 5;
    end.z = -10;
    begin.x = 0;
    end.x = 0;
    peck = 5;
    
    Tool = NULL;

    tagl.push_front( ftag( "BEGIN_Z", &begin.z ) );
    tagl.push_front( ftag( "END_Z", &end.z ) );
    tagl.push_front( ftag( "PECK", &peck ) );

    createmenu();
}

op_drilling::~op_drilling()
{
   tagl.clear();
   Menu.clear();
}

const char* op_drilling::name()
{
    return "Drilling";
}

op_type op_drilling::type()
{
    return DRILL;
}

void op_drilling::draw( color c , bool drawpath )
{
    
}

void op_drilling::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );
}

#define MENU_BACK 1

int op_drilling::parsemenu()
{
    Menuselect = 0;
    if( Menu.parse() )
    {
        if( Menuselect == MENU_BACK )
        {
            return OP_EXIT;
        }
        
    }
    return OP_NOP;
}

void op_drilling::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( name() );
        Menu.select(&Menuselect, MENU_BACK, "Back" );        if( Tool == NULL )
        {
            Menu.comment( "Missing Tool!" );
        }
        else if( Tool->tl.csspeed )
        {
            Menu.comment( "Dont use constant surface speed!" );
        }
        else
        {
            Menu.edit( &begin.z, "Begin Z" );
            Menu.edit(&end.z, "End Z" );
            Menu.edit( &peck, "Pecking " );
        }

    Menu.end();
} 

void op_drilling::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_drilling::update()
{

}

