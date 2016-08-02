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
    createmenu();
}

op_tool::~op_tool()
{
    tagl.clear();
}

const char* op_tool::name()
{
    sprintf( Name, "Tool: %s", ttcomments[ tl.tooln ]);
    return Name;
}

op_type op_tool::type()
{
    return TOOL;
}

void op_tool::draw( color c, bool drawpath )
{

}
/*
void op_tool::save( FILE *fp )
{
    if (fp == NULL) return;
    fprintf(fp, "OPERATION %i %s\n", type(), name() );
    fprintf(fp, "   DEPTH %.10g\n", tl.depth );
    fprintf(fp, "   FEED %.10g\n", tl.feed );
    fprintf(fp, "   SPEED %.10g\n", tl.speed );
    fprintf(fp, "   TOOLN %i\n", tl.tooln );
    fprintf(fp, "END\n" );
}

void op_tool::load( FILE *fp )
{
     if (fp == NULL) return;
    
    char *line = NULL;
    size_t len = 0;   
    ssize_t read;
    char tag[BUFFSIZE+1];
    
    double v1,v2,v3,v4,v5,v6;

    while ((read = getline( &line, &len, fp)) != -1)
    {

        printf("load %s", line);
        v1 = v2 = v3 = v4 = v5 = v6 = 0;
        
        sscanf(line, "%s %lf %lf %lf %lf %lf %lf", tag, &v1, &v2, &v3, &v4, &v5, &v6 );
        
        findtag( tag, "DEPTH", tl.depth, v1 );
        findtag( tag, "FEED",  tl.feed, v1 );
        findtag( tag, "SPEED", tl.speed, v1 );
        findtag( tag, "TOOLN", tl.tooln, v1 );

        free(line);
        line = NULL;
        
        if( strcmp( tag, "END" ) == 0 )
        {
            break;
        }      
    }
    
}
*/
void op_tool::save_program( FILE *fp )
{
    fprintf(fp, "(%s)\n", name() );  
    fprintf(fp, "T%d M6 F%f (change tool)\n", tl.tooln, tl.feed ); 
    fprintf(fp, "G43 (enable tool lenght compensation)\n" ); 
    fprintf(fp, "G96 D%d S%f (set maxrpm & surface speed)\n", maxrpm, tl.speed ); 
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
        CLAMP( tl.speed, 0, 1000 );
        CLAMP( tl.tooln, 0, MAXTOOLS );
        
        return OP_EDITED;
        
    }
    return OP_NOP;
}

bool b;

void op_tool::createmenu()
{
    Menu.clear();
    Menuselect = 0;
    Menu.begin( "Tool" );
        Menu.select(&Menuselect, MENU_BACK, "Back" );
        Menu.edit( &tl.tooln, "Tool number       " );
        Menu.edit( &tl.feed,  "Feedrate mm/rev.  " );
        Menu.edit( &tl.speed, "Surface speed m/s " );
        Menu.edit( &tl.depth, "Depth             " );
    Menu.end();
} 

void op_tool::drawmenu(int x,int y)
{
    Menu.draw(x,y);
} 

void op_tool::update()
{
    printf("%s:update\n",name());
}
