#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;
extern char *ttcomments[CANON_POCKETS_MAX];



op_tool::op_tool()
{
    tagl.push_front( ftag( "DEPTH", &tl.depth ) );
    tagl.push_front( ftag( "FEED",  &tl.feed ) );
    tagl.push_front( ftag( "SPEED", &tl.speed ) );
    tagl.push_front( ftag( "TOOLN", &tl.tooln ) );
    tagl.push_front( ftag( "CSS", &tl.csspeed ) );
    createmenu();
}

op_tool::~op_tool()
{
    tagl.clear();
}

const char* op_tool::name()
{
    if( tl.csspeed )
    {
        sprintf( Name, "Tool: %s css %.8g feed %.8g depth %.8g", ttcomments[ tl.tooln ], tl.speed, tl.feed, tl.depth );
    }
    else
    {
        sprintf( Name, "Tool: %s rpm %.8g feed %.8g depth %.8g", ttcomments[ tl.tooln ], tl.speed, tl.feed, tl.depth );
    }
    return Name;
}


op_type op_tool::type()
{
    return TOOL;
}


void op_tool::draw( color c, bool drawpath )
{

}


   
void op_tool::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );  
    fprintf(fp, "T%d M6 F%f (change tool)\n", tl.tooln, tl.feed ); 
    fprintf(fp, "G43 (enable tool lenght compensation)\n" ); 
    if( tl.csspeed )
    {
        fprintf(fp, "G96 D%d S%f (set maxrpm & surface speed)\n", maxrpm, tl.speed ); 
    }
    else
    {
        fprintf(fp, "G97 S%f (set rpm)\n", tl.speed ); 
    }
    fprintf(fp, "M4 (start spindle)\n" ); 
    fprintf(fp, "G4 P1 (wait 1 second)\n" );
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
        
        CLAMP( tl.depth, 0.01, 10 );
        CLAMP( tl.feed, 0, 1000 );
        CLAMP( tl.speed, 0, maxrpm );
        CLAMP( tl.tooln, 0, MAXTOOLS );
        
        if( Menu.edited( &tl.csspeed ) )
        {
            createmenu();
        }
        
        return OP_EDITED;
        
    }
    return OP_NOP;
}

void op_tool::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( "Tool" );
        Menu.select(&Menuselect, MENU_BACK, "Back" );
        Menu.edit( &tl.csspeed, "Constant surface speed " );
        Menu.edit( &tl.tooln, "Tool number       " );
        Menu.edit( &tl.depth, "Depth             " );
        Menu.edit( &tl.feed,  tl.csspeed ? "Feedrate mm/rev. ":"Feedrate mm/min. "  );
        Menu.edit( &tl.speed, tl.csspeed ? "Surface speed m/s ":"RPM " );
    Menu.end();
} 

void op_tool::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_tool::update()
{
    
}
