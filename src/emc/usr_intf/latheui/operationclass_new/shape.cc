#include "../latheintf.h"

extern const double retract;
extern const double stockdiameter;
extern char strbuf[BUFFSIZE];
extern const int maxrpm;
extern struct machinestatus status;
extern double scale;


op_shape::op_shape()
{    
    tagl.push_front( ftag( "SIDE", &side ) );
    tagl.push_front( ftag( "FINISH_COUNT", &fcount ) );
    createmenu();
    p.create_line( vec2(0,0), MOV_LINE );
    changed = true;
}

op_shape::~op_shape()
{
    Menu.clear();
    tagl.clear();
    p.clear();
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
    if( changed )
    {
        tp.create_from_shape( p );
        if( Tool != NULL )
        {
            double tool_r = _tools[ Tool->tl.tooln ].diameter/2.0f;
            for( int i = 0; i < fcount; i++ )
            {
                fp[i].create_from_contour( tp, tool_r + ((double)i) * Tool->tl.depth, side, MOV_FEED );
            }
        }
        
     //   rp.create_rough_from_contour( fp[fcount-1], Tool->tl, side );
        
     //   if(side == OUTSIDE) up.create_undercut_from_contour( fp[fcount-1], Tool->tl, side );
        
        for( int i = 0; i < fcount; i++ )
        {
            fp[i].move( tool_cpoint( Tool->tl.tooln ) );
            if( i>0)
            { 
                fp[i].rapid_move_and_feed_close( fp[i-1] );
            }
        }
        
        fp[0].rapid_move_and_feed_close( fp[0] );
        
        rp.rapid_move( up.start() );
        
        up.rapid_move_and_feed_close( fp[ fcount-1 ] );
        
        changed = false;
    }
    
    p.drawshadows( DISABLED );
    tp.draw( NONE );
    
    if( path )
    {
        for( int i = 0; i < fcount; i++ )
        {
            fp[i].draw( NONE );
        }
     //   rp.draw( NONE );
       // up.draw( NONE );
        
        drawCross( p.current().z, -p.current().x , 3.0/scale);
        drawCircle( p.current().z,-p.current().x, 3.0/scale);
    }
    
    sprintf(strbuf,"X%4.2f Z%4.2f", p.cur_start().x, p.cur_start().z  ); println( strbuf, 40, 55, 40, TEXT );  
     println( "TEXTI" );
     println( "TEXTI" );
     println( "TEXTI" );
    
}

void op_shape::save_program( FILE *f )
{
    fprintf(f, "(%s)\n", name() );
    
    fprintf(f, "(rough)\n" );
    //rp.save( f );
  
   // fprintf(f, "(undercut)\n" );
   // up.save( f );   
    
    for( int i = fcount-1 ; i >= 0; i-- )
    {
        fprintf(f, "(fine pass %d)\n", i );
        fp[i].save( f );
    }
    
}

#define MENU_BACK 1

int op_shape::parsemenu()
{ 
    if( Menu.current_menu( "Edit" ) )
    {
        if( status.jogged != 0 )
        {
            printf("jog\n");
            if( status.axis == AXISX )
            {
                p.movecurpos( vec2( status.jogged / 2.0, 0 ));
            }
            else if( status.axis == AXISZ )
            {
                p.movecurpos( vec2( 0,status.jogged ));
            } 
            else if( status.axis == AXISC )
            {
                p.setcurradius( p.cur_radius() + status.jogged );
            } 
            changed = true;
        } 
        
        if( isprefix( "RIGH" ,NULL ) )
        {
            p.previous();
        }
        else if( isprefix( "LEFT" ,NULL ) )
        {
            p.next();
        } 
        else if( isprefix( "UP" ,NULL ) )
        {
            p.setcur_type( (move_type)((int)p.cur_type() + 1) );
            changed = true;
        } 
        else if( isprefix( "DOWN" ,NULL ) )
        {
            p.setcur_type( (move_type)((int)p.cur_type() - 1) );
            changed = true;
        }    
        else if( isprefix( "DEL" ,NULL ) )
        {
            p.erase();
            changed = true;
        }         
        
        const char *c = isprefix( "CH=" ,NULL );
        if( c )
        {
            if( *c == 'n' )
            {
                printf("line\n");
                p.create_line( p.end(), MOV_LINE );
                changed = true;
            }    
        }
    }
    
    Menuselect = 0;
    if( Menu.parse() )
    {
        if( Menuselect == MENU_BACK )
        {
            return OP_EXIT;
        }
        
        if( Tool != NULL)
        {
            CLAMP( fcount,1,MAX_FINISH );
            changed = true;
            return OP_EDITED;
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
        if( Tool == NULL )
        {
            Menu.comment( "Missing Tool!" );
        }
        else
        {
            Menu.begin( "Edit" );
                Menu.back( "Back " );
            Menu.end();
            Menu.radiobuttons( (int *)&side , "Side", (int)OUTSIDE, "Outside", (int)INSIDE, "Inside" );
            Menu.edit( &fcount, "Finishing passes " );
        }
    Menu.end();
} 

void op_shape::drawmenu(int x,int y)
{
    Menu.draw(x,y);
    
    if( Menu.current_menu( "Edit" ) )
    {
        sprintf(strbuf,"start D%4.2f Z%4.2f", p.cur_start().x*2.0, p.cur_start().z  ); println( strbuf, 450, 55, 20, TEXT );  
        sprintf(strbuf,"end   D%4.2f Z%4.2f", p.cur_end().x*2.0, p.cur_end().z  ); println( strbuf );  
        sprintf(strbuf,"rel   D%4.2f Z%4.2f", p.cur_end().x*2.0 - p.cur_start().x*2.0, p.cur_end().z - p.cur_start().z  ); println( strbuf );  
        if( p.cur_type() == MOV_ARC_OUT || p.cur_type() == MOV_ARC_IN ) 
        {
            sprintf(strbuf,"R%4.2f", p.cur_radius() ); println( strbuf ); 
        } 
    }
} 

void op_shape::update()
{
    changed = true;
}


void op_shape::load( FILE *fp )
{
    new_operation::load( fp );
    p.loadmoves( fp );
    //loadtagl( fp, tagl );
}

void op_shape::save( FILE *fp )
{
    new_operation::save( fp );
    p.savemoves( fp );
    //if (fp == NULL) return;
    //fprintf(fp, "OPERATION %i %s\n", type(), name() );
    //savetagl( fp, tagl );
}

