#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;
extern char *ttcomments[CANON_POCKETS_MAX];

op_rectangle::op_rectangle()
{   
    Tool = NULL;
    begin.z = 5;
    end.z = 0;
    begin.x = stockdiameter/2.0;
    end.x = 0;
    feed_dir = DIRX;
    
    tagl.push_front( ftag( "FEED_DIR", &feed_dir ) );
    tagl.push_front( ftag( "BEGIN_X", &begin.x ) );
    tagl.push_front( ftag( "BEGIN_Z", &begin.z ) );
    tagl.push_front( ftag( "END_X", &end.x ) );
    tagl.push_front( ftag( "END_Z", &end.z ) );
    
    createmenu();
}

op_rectangle::~op_rectangle()
{
    tagl.clear();
}

const char* op_rectangle::name()
{
    return "Rectangle";
}

op_type op_rectangle::type()
{
    return RECTANGLE;
}

void op_rectangle::draw( color c, bool drawpath )
{
    setcolor( DISABLED );
    drawBox( begin, end );
    drawCross( begin.z, -begin.x , 3.0/scale);
    drawCircle( begin.z, -begin.x , 3.0/scale);
    if( Tool != NULL )
    {    
        if( drawpath ) rect_path.draw( c );
    }
}

void op_rectangle::save_program( FILE *fp )
{
    update();
    fprintf(fp, "(%s)\n", name() );
    rect_path.save( fp );
}

#define MENU_BACK 1

int op_rectangle::parsemenu()
{
    Menuselect = 0;
    if( Menu.parse() )
    {
        if( Menuselect == MENU_BACK )
        {
            return OP_EXIT;
        }
        printf("dir =%i\n",feed_dir);
        return OP_EDITED;
        
    }
    return OP_NOP;
}

void op_rectangle::createmenu()
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
            Menu.radiobuttons( &feed_dir, "Feed direction", DIRZ, "Z", DIRX, "X" );
            Menu.edit( &begin.x, "Start diameter " ); Menu.diameter_mode();
            Menu.edit( &end.x,   "End diameter   " ); Menu.diameter_mode();
            Menu.edit( &begin.z, "Start Z        " );
            Menu.edit( &end.z,   "End Z          " );
        }           
    Menu.end();
} 

void op_rectangle::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_rectangle::update()
{
    rect_path.clear();
    if( Tool != NULL )
    {
        rect_path.create( Tool->tl, begin, end, feed_dir );
    }
}
    
    
    
    
