#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern struct machinestatus status;


op_shape::op_shape()
{
    createmenu();
    p.create_line( p.end(), MOV_CONTOUR );
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
    p.draw( NONE );
}

void op_shape::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );
    
}

#define MENU_BACK 1

int op_shape::parsemenu()
{ 
    
    if( status.jogged != 0 )
    {
        printf("jog\n");
        if( status.axis == AXISX )
        {
            p.movecurpos( vec2( status.jogged, 0 ));
        }
        else if( status.axis == AXISZ )
        {
            p.movecurpos( vec2( 0,status.jogged ));
        } 
    } 
    
    if( isprefix( "RIGH" ,NULL ) )
    {
        p.previous();
    }
    else if( isprefix( "LEFT" ,NULL ) )
    {
        p.next();
    } 
    
    const char *c = isprefix( "CH=" ,NULL );
    if( c )
    {
        
        if( *c == 'l' )
        {
            printf("line\n");
            p.create_line( p.end()-10, MOV_CONTOUR );
        }
        else if( *c == 'c' )
        {
            printf("arch\n");
        }     
          
    }
    
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


void op_shape::load( FILE *fp )
{
    loadtagl( fp, tagl );
}

void op_shape::save( FILE *fp )
{
    if (fp == NULL) return;
    fprintf(fp, "OPERATION %i %s\n", type(), name() );
    savetagl( fp, tagl );
}

