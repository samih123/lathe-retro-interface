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
#define TYPESHOW 7

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

void menu::color( int c )
{
    cmi->ml.back().color = c;
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
    sprintf( cmi->ml.back().str, "%d", *(int *)cmi->ml.back().val );
    usewheel = false;
}

void menu::edit( double *d, const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPEDOUBLE;
    cmi->ml.back().val = d;  
    strcpy( cmi->ml.back().name, n);
    sprintf( cmi->ml.back().str, "%.10g", *(double *)cmi->ml.back().val );
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

void menu::show( const char *n )
{
    cmi->ml.push_back( menuitem() );
    cmi->ml.back().type = TYPESHOW; 
    strcpy( cmi->ml.back().name, n);   
}
 

 
void menu::draw( int x, int y)
{

    
    println( x, y, 20, cmi->color );
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
                    case TYPESHOW:
                    case TYPEBACK:
                    case TYPESELECT:
                    case TYPEBEGIN:
                        sprintf(strbuf,"%s%s", arrow, i->name );
                        println( strbuf, i->color );
                    break;                 
                    case TYPEINT:
                        sprintf(strbuf,"%s%s %s", arrow, i->name, i->str );
                        println( strbuf, i->color );
                    break;
                    case TYPESTR:
                        sprintf(strbuf,"%s%s %s", arrow, i->name, (char *)i->val );
                        println( strbuf,i->color );
                    break;                
                    case TYPEDOUBLE:
                        sprintf(strbuf,"%s%s%s", arrow, i->name, i->str );
                        println( strbuf, i->color );
                    break;      
                    case TYPEBOOL:
                        sprintf(strbuf,"%s%s %s", arrow, i->name, *(bool *)i->val ? "On":"Off" );
                        println( strbuf, i->color );
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
        i->edited = false;
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
        if( cmi->it != --cmi->ml.end() ) cmi->it++;
    }
    else if( isprefix( "UP" ,NULL ) || (usewheel && isprefix( "JG-" ,NULL )) )
    {
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
                (*(int *)cmi->it->val) = atoi( cmi->it->str );
                sprintf( cmi->it->str, "%d", *(int *)cmi->it->val );
                return true;
            case TYPEBOOL:
            case TYPESTR:
                cmi->it->edited = true;
                return true;
            break;
            case TYPEDOUBLE:
                (*(double *)cmi->it->val) = atof( cmi->it->str );
                sprintf( cmi->it->str, "%.10g", *(double *)cmi->it->val );
                cmi->it->edited = true;
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
                  (*(int *)cmi->it->val)++;
                  sprintf( cmi->it->str, "%d", *(int *)cmi->it->val );
                  cmi->it->edited = true;
                  return true;
             }             
             if( cmi->it->type == TYPEDOUBLE )
             {
                  (*(double *)cmi->it->val) += status.incr;
                  sprintf( cmi->it->str, "%.10g", *(double *)cmi->it->val );
                  cmi->it->edited = true;
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
                  (*(int *)cmi->it->val)--;
                  sprintf( cmi->it->str, "%d", *(int *)cmi->it->val );
                  cmi->it->edited = true;
                  return true;
             }             
             if( cmi->it->type == TYPEDOUBLE )
             {
                  (*(double *)cmi->it->val) -= status.incr; 
                  sprintf( cmi->it->str, "%.10g", *(double *)cmi->it->val );
                  cmi->it->edited = true;
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
                sprintf( cmi->it->str, "%d", *(int *)cmi->it->val );
                cmi->it->edited = true;
                return true;
            }   
            if( cmi->it->type == TYPEDOUBLE )
            {
                (*(double *)cmi->it->val) = -fabs( (*(double *)cmi->it->val) );
                sprintf( cmi->it->str, "%.10g", *(double *)cmi->it->val );
                cmi->it->edited = true;
                return true;
            }   
        }
        else if( *c == '+' )
        {
            if( cmi->it->type == TYPEINT )
            {
                (*(int *)cmi->it->val) = abs( (*(int *)cmi->it->val) );
                sprintf( cmi->it->str, "%d", *(int *)cmi->it->val );
                cmi->it->edited = true;
                return true;
            }   
            if( cmi->it->type == TYPEDOUBLE )
            {
                (*(double *)cmi->it->val) = fabs( (*(double *)cmi->it->val) );
                sprintf( cmi->it->str, "%.10g", *(double *)cmi->it->val );
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









