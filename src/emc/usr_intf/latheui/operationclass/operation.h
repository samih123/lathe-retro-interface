
const char* phase_name( int t );

class operation
{
    public:

    operation( op_type t );
    ~operation();
    
    const char* get_name();
    
    // shape
    cut get_cut();
    void set_cut( cut &c );
    
    // tool
    tool get_tool();
    void set_tool( tool &c );
    
    // facing
    vec2 getf_begin(){ return face_begin; };
    vec2 getf_end(){ return face_end; };
    void setf_begin_end( vec2 fbeg, vec2 fend );
    
    //move
    vec2 get_move(){ return rapid_move; };
    void set_move( vec2 m ){ rapid_move = m; changed = true; };
    
    //void set_inside( bool s ){ inside = s;} 
    Side get_side(){ return side; }; 
    void draw( color c=NONE, bool all = true );
    void new_cut( vec2 p, cut_type t );
    void clear(){ f_path.clear();r_path.clear(); changed = true; };
    
    void erase();
    void previous();
    void next();
    
    void save( FILE *fp ); 
    void save_program( FILE *fp ); 
    
    void load( FILE *fp );
    
    void create_contour( contour_path &p );
    void create_contour(){ create_contour( contour ); };
    
    void create_path( operation &ccontour, operation &ctool );

    op_type get_type(){ return type; };
    
    private:
    op_type type; 
    
    // shape
    list<struct cut>::iterator currentcut;
    std::list<struct cut> cl;
    Side side;
    contour_path contour;
    bool changed;
    
    // tool
    tool tl;
    
    // rough 
    rough_path r_path;
    
    // facing
    box_path f_path;
    vec2 face_begin,face_end;
    
    //move
    vec2 rapid_move;
     
    vec2 min,max;
    char name[BUFFSIZE]; 
};






