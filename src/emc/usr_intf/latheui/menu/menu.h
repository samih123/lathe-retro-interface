/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

struct menuitem
{
    menuitem()
    {
        name[0] = 0;
        type = 0;
        hidden = false;
        edited = false;
        shortcut = NULL;
        val = NULL;
        tcolor = TEXT;
        edit_color = WARNING;
        divider = 1;
        diam_mode = false;
    };
    ~menuitem()
    {
        ml.clear();
    }
    
    void *val;;
    void *cval[3];
    int axis;
    int type;
    int num;
    
    char bstr[3][BUFFSIZE];
    int bnum[3];
    int bsel;
    
    char name[BUFFSIZE];
    char str[BUFFSIZE];
    
    const char *shortcut;     
    list<struct menuitem> ml;
    list<struct menuitem>::iterator it;
    menuitem *up;
    bool hidden;
    bool edited;
    color tcolor;
    color edit_color;
    int divider;
    bool diam_mode;
}; 


class menu
{
    public:
    menu();
    void clear();
    void begin( const char *name );
    void begin( int *i, int num, const char *n );

    void end();
    void setmaxlines( int l );
    
    void hiddenvalue();
    void setcolor( color c );
    void shortcut( const char *shortcut );
    void diameter_mode();
    
    
    void edit( int *i, const char *n);
    void select( int *i, int num, const char *n );
    void radiobuttons( int *i, const char *n, int num1, const char *n1, int num2, const char *n2, int num3=0, const char *n3 = NULL );
    void edit( double *d, const char *n );
    void edit( char *s, const char *n );
    void edit( bool *b, const char *n );
    void coordinate( double *x, double *z, double *c, const char *n);
    void back( const char *n );
    void comment( const char *n );
    void clean( menuitem &m );
    bool edited( void *v );
    bool current_menu( const char *n );
    void draw( int x, int y);
    bool parse();
    
    private:
    bool edited( menuitem &m, void *v );
    void update_str( menuitem &m );
    void update_val( menuitem &m, int v );
    void update_val( menuitem &m, double v );
    
    char strbuf[BUFFSIZE];
    menuitem rm;
    menuitem *cmi; // current menuitem
    bool usewheel;
    bool point;
    int maxlines;
};







