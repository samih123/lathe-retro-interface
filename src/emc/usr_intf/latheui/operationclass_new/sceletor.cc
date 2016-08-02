#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern double scale;
extern char *ttcomments[CANON_POCKETS_MAX];

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

void op_tool::save( FILE *fp )
{
    if (fp == NULL) return;
    fprintf(fp, "OPERATION %i %s\n", type(), name() );
    
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

       // findtag( tag, "DEPTH", tl.depth, v1 );

        free(line);
        line = NULL;
        
        if( strcmp( tag, "END" ) == 0 )
        {
            break;
        }      
    }

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

