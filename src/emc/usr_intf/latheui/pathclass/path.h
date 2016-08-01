
class path
{
    public:
    path()
    {
        temporary = false;
    }

    ~path()
    {
        ml.clear();
    }
    
    void set_temporary(){ temporary = true; };
    void create_line( const vec2 &v , const move_type t, const char *comment = NULL );
    void create_arc( struct cut &c, const vec2 v1, const vec2 v2, const double r, const bool side, const move_type t );
    void rapid_move( const vec2 v );
    void remove_knots();
    void clear(){ ml.clear(); };
    
    double distance( const vec2 p1, const vec2 p2);
    bool find_intersection( const vec2 a, const vec2 b, vec2 &cv, list<struct mov>::iterator fi,
                            list<struct mov>::iterator &ci, bool first = true );

    void feed( path &colp, list<struct mov>::iterator fi, vec2 v, double len, const vec2 dir, const vec2 ret );
    void feed( path &colp, vec2 v, double len, const vec2 dir, const vec2 ret );
    void move( vec2 m );
    void draw( color c = NONE );
    void save( FILE *fp );
    void findminmax();
    
    //protected:

   // int type;
    std::list<struct mov> ml;
    vec2 min,max;    
    Side side;
    bool temporary;
    //friend class fine_path;
    //friend class rough_path;
    //friend class undercut_path;

};

class contour_path:public path
{
    public:

};

class fine_path:public path
{
    public:
    void create( contour_path &c, double r, Side side, const move_type t );
};

class rough_path:public path
{
    public:
    void create( contour_path &c, const tool &tl, Side side );
    void draw( color c = NONE );
    fine_path tc;
};

class undercut_path:public path
{
    public:
    void create( contour_path &c, double depth, double tool_r, double retract, Side side );
    void draw( color c = NONE );
    fine_path tc;
};

class rectangle_path:public path
{
    public:
    void create( const tool &tl, vec2 start, vec2 end, int dir );
};
