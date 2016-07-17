/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

extern struct machinestatus status;

extern int screenw, screenh;
extern char buf[BUFFSIZE];

#define TYPEBEGIN 0
#define TYPEINT 1
#define TYPEDOUBLE 2
#define TYPESTR 3
#define TYPEBOOL 4
#define TYPESELECT 5
#define TYPEBACK 6
#define TYPECOMMENT 7

#define SELECT 1
#define SET 2

menu::menu()
{
    clear();
}

void menu::clear()
{
    rm.ml.clear();
    rm.type = TYPEBEGIN; 
    rm.it = rm.ml.begin();
    rm.shortcut = NULL;
    rm.val = NULL;
    cmi = &rm;
    rm.up = &rm;
    usewheel = true;
    maxlines = 24;
}

void menu::begin( const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEBEGIN;   
    cmi->ml.back().up =  cmi;  
    cmi->ml.back().shortcut = NULL; 
    strcpy( cmi->ml.back().name, n);  
    cmi->ml.back().val = NULL;
    cmi = &cmi->ml.back();
}

void menu::begin( int *i, int num, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEBEGIN;   
    cmi->ml.back().up =  cmi;  
    cmi->ml.back().shortcut = NULL; 
    strcpy( cmi->ml.back().name, n);  
    cmi->ml.back().val = (void *)i;
    cmi->ml.back().num = num;
    cmi = &cmi->ml.back();
}

void menu::update_str( menuitem &m )
{
    switch( m.type )
    { 
        case TYPEBEGIN:
        break;
        
        case TYPEBACK:
        break;
        
        case TYPESELECT:
        break;   
          
        case TYPEINT:
            sprintf( m.str, "%d", *(int *)m.val * m.divider );
        break;  
         
        case TYPEBOOL:
        break; 
        
        case TYPESTR:
        break;
        
        case TYPEDOUBLE:
            sprintf( m.str, "%.10g", *(double *)m.val * (double)m.divider );
        break;
    }
}

void menu::update_val( menuitem &m, int v )
{    
    if( m.type != TYPEINT )
    {
        printf( "MENU:update_val type ERROR." );
        return;
    }
    
    if( v != *(int *)m.val )
    { 
        m.edited = true;
        *(int *)m.val = (int)v;
    }
    sprintf( m.str, "%d", *(int *)m.val * m.divider );

}
            

void menu::update_val( menuitem &m, double v )
{

    if( m.type != TYPEDOUBLE )
    {
        printf( "MENU:update_val type ERROR." );
        return;
    }
    
    if( v != *(double *)m.val )
    { 
        m.edited = true;
        *(double *)m.val = (double)v;
    }
    sprintf( m.str, "%.10g", *(double *)m.val * (double)m.divider );

}
                       
void menu::setmaxlines( int l )
{
    maxlines = l;
}
 
void menu::shortcut( const char *n )
{
    cmi->ml.back().shortcut = n; 
}

void menu::hiddenvalue()
{
    cmi->ml.back().hidden = true;
}  

void menu::setcolor( color c )
{
    cmi->ml.back().tcolor = c;
}

void menu::diameter_mode()
{
    cmi->ml.back().divider = 2;
    update_str(cmi->ml.back());
}
 
void menu::end()
{
    cmi = cmi->up;
    if( cmi == &rm ) cmi = &cmi->ml.back();
    cmi->it = cmi->ml.begin();
}  

void menu::edit( int *i, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEINT;
    cmi->ml.back().val = (void *)i;
    strcpy( cmi->ml.back().name, n);
    update_str( cmi->ml.back() );
    usewheel = false;
}

void menu::edit( double *d, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEDOUBLE;
    cmi->ml.back().val = d;  
    strcpy( cmi->ml.back().name, n);
    update_str( cmi->ml.back() );
    usewheel = false;
}

void menu::edit( char *s, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPESTR;
    cmi->ml.back().val = s; 
    strcpy( cmi->ml.back().name, n);       
}

void menu::edit( bool *b, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEBOOL;
    cmi->ml.back().val = b; 
    strcpy( cmi->ml.back().name, n); 
    usewheel = false;
}

void menu::select( int *i, int num, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPESELECT;
    cmi->ml.back().val = (void *)i;
    strcpy( cmi->ml.back().name, n);
    cmi->ml.back().num = num;
}

void menu::back( const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEBACK; 
    strcpy( cmi->ml.back().name, n);   
}

void menu::comment( const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPECOMMENT; 
    strcpy( cmi->ml.back().name, n);   
}
 

 
void menu::draw( int x, int y)
{

    // update edited strings
    clean( rm );
    
    println( x, y, 18, cmi->tcolor );
    if( cmi->name[0] != 0 ) println( cmi->name );
   
    int selected = std::distance( cmi->ml.begin(), cmi->it);
    int line = 0, drawedline = 0;
    for(list<struct menuitem>::iterator i = cmi->ml.begin(); i != cmi->ml.end(); i++)
    {
        line++;
        if( line > selected - maxlines + 1 )
        {

            if( ++drawedline > maxlines )
            {
                break;
            }
            
            const char *arrow = (i == cmi->it ? "->":"  ");
            if( ! i->hidden ){
                switch( i->type )
                {
                    case TYPECOMMENT:
                    case TYPEBACK:
                    case TYPESELECT:
                    case TYPEBEGIN:
                        sprintf(strbuf,"%s%s", arrow, i->name );
                        println( strbuf, i->tcolor );
                    break;                 
                    case TYPEINT:
                        sprintf(strbuf,"%s%s[%s]", arrow, i->name, i->str );
                        println( strbuf, i->tcolor );
                    break;
                    case TYPESTR:
                        sprintf(strbuf,"%s%s[%s]", arrow, i->name, (char *)i->val );
                        println( strbuf,i->tcolor );
                    break;                
                    case TYPEDOUBLE:
                        sprintf(strbuf,"%s%s[%s]", arrow, i->name, i->str );
                        println( strbuf, i->tcolor );
                    break;      
                    case TYPEBOOL:
                        sprintf(strbuf,"%s%s[%s]", arrow, i->name, *(bool *)i->val ? "On":"Off" );
                        println( strbuf, i->tcolor );
                    break;                                     
                }
            }
            else
            {
                sprintf(strbuf,"%s%s", arrow, i->name ); println( strbuf );
            }
        }
    }        
}

// recursive cleaning 
void menu::clean( menuitem &m )
{
    for(list<struct menuitem>::iterator i = m.ml.begin(); i != m.ml.end(); i++)
    {
        if( (i->type == TYPEBEGIN || i->type == TYPESELECT ) && i->val != NULL )
        {
             *(int *)i->val = 0;
        }
        
        if( i->edited )
        {
            update_str( *i );
            i->edited = false;
        }
        
        clean( *i );
    }
}


bool menu::parse()
{
    
    // check shortcuts
    for(list<struct menuitem>::iterator i = cmi->ml.begin(); i != cmi->ml.end(); i++)
    {
        if( i->shortcut != NULL )
        {
            if( isprefix( i->shortcut ,NULL ) )
            {
                cmi->it = i;
                return false;
            }
        }
    }
    
    // clear selections &&b edited flag
    clean( rm );  
      
    // menu up,down,select
    if( isprefix( "DOWN" ,NULL ) || (usewheel && isprefix( "JG+" ,NULL )) )
    {
        update_str( *cmi->it );
        if( cmi->it != --cmi->ml.end() ) cmi->it++;
    }
    else if( isprefix( "UP" ,NULL ) || (usewheel && isprefix( "JG-" ,NULL )) )
    {
        update_str( *cmi->it );
        if( cmi->it != cmi->ml.begin() ) cmi->it--;
    }
    else if( isprefix( "RET" ,NULL ) )
    {
        switch( cmi->it->type )
        { 
            
            case TYPEBEGIN:
                if( cmi->it->val != NULL )
                {
                    *(int *)cmi->it->val = cmi->it->num;
                    cmi->it->edited = true;
                }
                cmi = &*cmi->it;
                cmi->it = cmi->ml.begin();
                return true;
            break;
            
            case TYPEBACK:
                cmi = cmi->up;
                cmi->it = cmi->ml.begin();
            break;
            
            case TYPESELECT:
                *(int *)cmi->it->val = cmi->it->num;
                cmi->it->edited = true;
                return true;
            break;   
              
            case TYPEINT:
                update_val( *cmi->it, atoi( cmi->it->str ) / (int)cmi->it->divider );
                return true;
            case TYPEBOOL:
            
            case TYPESTR:
                cmi->it->edited = true;
                return true;
            break;
            
            case TYPEDOUBLE:
                update_val( *cmi->it, atof( cmi->it->str ) / (double)cmi->it->divider );
                return true;
            break;
            
        }
        
    }
    
    if( ! usewheel ){
        if( isprefix( "JG+" ,NULL ) )
        {
             if( cmi->it->type == TYPEBOOL)
             {
                  (*(bool *)cmi->it->val) = true;
                  cmi->it->edited = true;
                  return true;
             }
             if( cmi->it->type == TYPEINT )
             {
                  update_val( *cmi->it, (*(int *)cmi->it->val)+1 );
                  return true;
             }             
             if( cmi->it->type == TYPEDOUBLE )
             {
                  update_val( *cmi->it, (*(double *)cmi->it->val) + status.incr / (double)cmi->it->divider );
                  return true;
             }      
        }
        else if( isprefix( "JG-" ,NULL ) )
        {
             if( cmi->it->type == TYPEBOOL)
             {
                  (*(bool *)cmi->it->val) = false;
                  cmi->it->edited = true;
                  return true;
             }
             if( cmi->it->type == TYPEINT )
             {
                 update_val( *cmi->it, (*(int *)cmi->it->val) - 1 );
                  return true;
             }             
             if( cmi->it->type == TYPEDOUBLE )
             {
                  update_val( *cmi->it, (*(double *)cmi->it->val) - status.incr / (double)cmi->it->divider );
                  return true;
             }             
        }
    }

    if( isprefix( "LEFT" ,NULL ) )
    {
        if( cmi->it->type == TYPEBOOL)
        { 
            (*(bool *)cmi->it->val) = false;
            cmi->it->edited = true;
            return true;
        }        
    }
    if( isprefix( "RIGHT" ,NULL ) )
    {
        if( cmi->it->type == TYPEBOOL)
        {
            (*(bool *)cmi->it->val) = true;
            cmi->it->edited = true;
            return true;
        }
    }
    
    if( isprefix( "DEL" ,NULL ) || isprefix( "BACK" ,NULL ) )
    {
        
        if( cmi->it->type == TYPEDOUBLE || cmi->it->type == TYPEINT )
        {
            int l = strlen( cmi->it->str ); 
            if( l > 0 )
            {
               cmi->it->str[l-1] = 0; 
            }            
        }   
        
        if( cmi->it->type == TYPESTR )
        {
            int l = strlen( (char *)cmi->it->val ); 
            if( l > 0 )
            {
               ((char *)cmi->it->val)[l-1] = 0; 
            }
        }                
    }
    
    const char *c = isprefix( "CH=" ,NULL );
    if( c )
    {
        
        if( cmi->it->type == TYPESTR )
        {
            int l = strlen( (char *)cmi->it->val ); 
            if( l < BUFFSIZE-2 )
            {
               ((char *)cmi->it->val)[ l ] = *c; 
               ((char *)cmi->it->val)[ l+1 ] = 0; 
            }
        }
        
        
        if( *c == '-' )
        {
            if( cmi->it->type == TYPEINT )
            {
                (*(int *)cmi->it->val) = -abs( (*(int *)cmi->it->val) );
                update_str( *cmi->it );
                cmi->it->edited = true;
                return true;
            }   
            if( cmi->it->type == TYPEDOUBLE )
            {
                (*(double *)cmi->it->val) = -fabs( (*(double *)cmi->it->val) );
                update_str( *cmi->it );
                cmi->it->edited = true;
                return true;
            }   
        }
        else if( *c == '+' )
        {
            if( cmi->it->type == TYPEINT )
            {
                (*(int *)cmi->it->val) = abs( (*(int *)cmi->it->val) );
                update_str( *cmi->it );
                cmi->it->edited = true;
                return true;
            }   
            if( cmi->it->type == TYPEDOUBLE )
            {
                (*(double *)cmi->it->val) = fabs( (*(double *)cmi->it->val) );
                update_str( *cmi->it );
                cmi->it->edited = true;
                return true;
            }   
        }     
           
        else if( isdigit( *c ) || ( *c == '.' && cmi->it->type == TYPEDOUBLE ) )
        {
            if( cmi->it->type == TYPEINT || cmi->it->type == TYPEDOUBLE )
            {
                int l = strlen( cmi->it->str ); 
                if( l < BUFFSIZE-2 )
                {
                    cmi->it->str[ l ] = *c; 
                    cmi->it->str[ l+1 ] = 0; 
                }
            }
        }
       point = false;    
    } 
    return false;
}

bool menu::edited( menuitem &m, void *v )
{
    if( v == m.val && m.edited ) return true;
    
    for(list<struct menuitem>::iterator i = m.ml.begin(); i != m.ml.end(); i++)
    {
        if( edited( *i, v ) ) return true;
    }
    return false;
}

bool menu::edited( void *v )
{
    return edited( rm ,v );
}

bool menu::current_menu( const char *name )
{
 return ( strcmp( cmi->name, name  ) == 0 ) ? true:false;
}









