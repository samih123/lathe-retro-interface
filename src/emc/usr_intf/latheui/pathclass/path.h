
class path
{
    public:
    path()
    {
    }

    ~path()
    {
        ml.clear();
    }
    
    void create_line( const vec2 &v , const int t, const char *comment = NULL );
    void create_arc( struct cut &c, const vec2 v1, const vec2 v2, const double r, const bool side);
    void rapid_move( const vec2 v );
    void remove_knots();
    
    double distance( const vec2 p1, const vec2 p2);
    bool find_intersection( const vec2 a, const vec2 b, vec2 &cv, list<struct mov>::iterator fi,
                            list<struct mov>::iterator &ci, bool first = true );

    void feed_to_left( path &colp, vec2 v, double len , double depth);
    void feed_to_left( path &colp, list<struct mov>::iterator ci, vec2 v, double len , double depth);
    void draw( bool both );
    void save( FILE *fp );
    void findminmax();
    
    //protected:

   // int type;
    std::list<struct mov> ml;
    vec2 min,max;    
    bool outside;
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
    void create( contour_path &c, double r, bool outside = true );
};

class rough_path:public path
{
    public:
    void create( contour_path &c, const tool &tl, bool outside = true );
    void draw( bool both );
    fine_path tc;
};

class undercut_path:public path
{
    public:
    void create( contour_path &c, double depth, double tool_r, double retract, bool outside = true );
    void draw( bool both );
    fine_path tc;
};



