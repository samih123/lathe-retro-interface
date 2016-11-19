#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;


op_shape::op_shape()
{
    createmenu();
}

op_shape::~op_shape()
{
    Menu.clear();
    tagl.clear();
}

const char* op_shape::name()
{
    return "Shape";
}

op_type op_shape::type()
{
    return SHAPE;
}

void op_shape::draw( color c, bool path )
{

}

void op_shape::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );
}

#define MENU_BACK 1

int op_shape::parsemenu()
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

void op_shape::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( name() );
        Menu.select(&Menuselect, MENU_BACK, "Back" );
    Menu.end();
} 

void op_shape::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_shape::update()
{

}

