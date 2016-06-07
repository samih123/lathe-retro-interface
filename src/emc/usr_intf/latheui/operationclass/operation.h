

static const char *typestr[] =
{
    "cut_Begin",
    "straight line",
    "Outside arch",
    "Inside arch",
    "Thread",
    "cut_end"
};

class operation
{
    public:

    operation( int t );
    ~operation();
    
    void setz( double z );
    void setdiam( double d );
    //double getdiam();
    //double getz();
    vec2 get_cutend();
    vec2 get_cutstart();
    void set_cutend( const vec2 p );
    void set_cutstart( const vec2 p );
    
    const char* getcutname(){ if( type == CONTOUR && ! cl.empty() ){ return typestr[ currentcut->type ] ;} return "error"; };
    int getcuttype();
    void setcuttype( int );
    void set_inside( bool s ){ inside = s;} 
    bool get_inside(){ return inside; }; 
    void draw( int x1,int y1,int x2,int y2);
    void new_cut( vec2 p, cut_type t );
    
    void erase();
    void previous();
    void next();
    void save( const char *name ); 
    void load( const char *name );
    void create_contour( contour_path &p );
    
    int get_type(){ return type; };
    
    private:
    int type; 
    
    list<struct cut>::iterator currentcut;
    std::list<struct cut> cl;
    bool inside;
    
    contour_path contour;
    double scale;
    vec2 pos;
    vec2 min,max; 
};






