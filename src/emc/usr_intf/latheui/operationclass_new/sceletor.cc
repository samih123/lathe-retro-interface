#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;


op_tool::op_tool()
{
    createmenu();
}

op_tool::~op_tool()
{
    Menu.clean();
}

const char* op_tool::name()
{
    return "Operation";
}

op_type op_tool::type()
{
    return TOOL;
}

void op_tool::draw( color c )
{
    
}

void op_tool::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );
}

#define MENU_BACK 1

int op_tool::parsemenu()
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

void op_tool::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( name() );
        Menu.select(&Menuselect, MENU_BACK, "Back" );
    Menu.end();
} 

void op_tool::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_rectangle::update()
{

}

