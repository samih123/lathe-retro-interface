


class operation
{
    public:

    operation( int t );
    ~operation();
    
    cut get_cut();
    void set_cut( cut &c );
    
    void set_inside( bool s ){ inside = s;} 
    bool is_inside(){ return inside; }; 
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
    
    // shape
    list<struct cut>::iterator currentcut;
    std::list<struct cut> cl;
    bool inside;
    contour_path contour;
    
    // tool
    
    
    
    double scale;
    vec2 pos;
    vec2 min,max; 
};






