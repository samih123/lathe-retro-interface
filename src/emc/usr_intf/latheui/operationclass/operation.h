


class operation
{
    public:

    operation( int t );
    ~operation();
    
    // shape
    cut get_cut();
    void set_cut( cut &c );
    
    // tool
    tool get_tool();
    void set_tool( tool &c );
    
    void set_inside( bool s ){ inside = s;} 
    bool is_inside(){ return inside; }; 
    void draw( int x1,int y1,int x2,int y2);
    void new_cut( vec2 p, cut_type t );
    
    void erase();
    void previous();
    void next();
    void save( FILE *fp ); 
    void load( FILE *fp );
    void create_contour( contour_path &p );
    void create_path( operation &ccontour, operation &ctool );

    int get_type(){ return type; };
    
    private:
    int type; 
    
    // shape
    list<struct cut>::iterator currentcut;
    std::list<struct cut> cl;
    bool inside;
    contour_path contour;
    
    // tool
    tool tl;
    
    // rough 
    rough_path r_path;
    
    double scale;
    vec2 pos;
    vec2 min,max; 
};






