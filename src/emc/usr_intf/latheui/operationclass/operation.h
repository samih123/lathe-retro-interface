
class operation
{
    public:
    operation()
    {
       scale = 1;
       pos.x = pos.z = 0;
    }
    
    operation( op_type t )
    {
       type = type;
    }
    
    ~operation()
    {
       
    }
    
    void draw( int x1,int y1,int x2,int y2);
    void new_cut( vec2 p, cut_type t );
    
    void erase();
    void previous();
    void next();
    void save( const char *name ); 
    void load( const char *name );
    
    int get_type(){ return (int)type; };
    void set_type( int t ){ type = (op_type)CLAMP(t,CUT_BEGIN,CUT_END); };
    
    //protected:
    op_type type;
    std::list<struct cut> cl;
    list<struct cut>::iterator currentcut;
    contour_path contour;
    double scale;
    vec2 pos;
    vec2 min,max;    
};






