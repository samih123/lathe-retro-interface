
class path;

class path
{
    public:
    path()
    {
        temporary = false;
        currentmov = ml.end();
    }

    ~path()
    {
        ml.clear();
    }
    
    void set_temporary(){ temporary = true; };
    void create_line( const vec2 &v , const move_type t, const char *comment = NULL );
    void create_arc( struct mov &c, const vec2 v1, const vec2 v2, const double r, const bool side, const move_type t );
    void rapid_move( const vec2 v );
    void rapid_move_and_feed_close( path &nextp );
    void remove_knots();
    void clear(){ ml.clear(); };
    void copy( path &s, move_type t = MOV_NONE );
    void buffer( double r, Side s );
    
    double distance( const vec2 p1, const vec2 p2);
    bool find_intersection( const vec2 a, const vec2 b, vec2 &cv, list<struct mov>::iterator fi,
                            list<struct mov>::iterator &ci, bool first = true );

    void feed( path &colp, list<struct mov>::iterator fi, vec2 v, double len, const vec2 dir, const vec2 ret );
    void feed( path &colp, vec2 v, double len, const vec2 dir, const vec2 ret );
    void move( vec2 m );
    void movecurpos( vec2 m );
    void draw( color c = NONE );
    void drawshadows( color c = NONE );
    void save( FILE *fp );
    void savemoves( FILE *fp );
    void loadmoves( FILE *fp );
    void findminmax();
    vec2 end();
    vec2 start();
    vec2 current();
    void setcur_type( move_type t );
    void setcurradius( double r );
    double cur_radius();
    vec2 cur_end();
    vec2 cur_start();
    move_type cur_type();
    void erase();
    void next();
    void previous();
    void sanitize();
    
    
    void create_rectangle( const tool &tl, vec2 start, vec2 end, int dir );
    void create_from_contour( path &c, double r, Side s, move_type mtype );
    void create_from_shape( path &c );
    
    void create_rough_from_contour( path &c, const tool &tl, Side s );
    void create_undercut_from_contour( path &c, const tool &tl, Side s );
    
    protected:
    
    list<struct mov>::iterator currentmov;
    std::list<struct mov> ml;
    
    vec2 min,max;    
    Side side;
    bool temporary;
    
};




