
#define OP_EXIT 1
#define OP_EDITED 2
#define OP_NOP 3

class op_tool;
class op_contour;
class op_rectangle;
class op_threading;
class op_drilling;
class op_shape;

class new_operation
{
    
    friend void savetagl( FILE *fp, list<ftag> &tagl );
    friend void loadtagl( FILE *fp, list<ftag> &tagl );
    
    public:
    virtual ~new_operation(){};
    
    virtual const char* name() { return "None"; }
    virtual op_type type() { return NOOP; };
    virtual void draw( color c=NONE, bool path = true ) {};
    virtual void save_program( FILE *fp ) {}; 
    virtual int parsemenu() { return OP_EXIT; };
    virtual void createmenu() {};
    virtual void drawmenu(int x,int y) {};
    virtual void update(){};
    
    void set_tool( op_tool *t ){ Tool = t; };
   // void set_contour( op_contour *c ){ //Contour = c; };
    virtual void load( FILE *fp );
    virtual void save( FILE *fp );
    
    protected:
    menu Menu;
    int Menuselect;
    op_tool *Tool;
    //op_contour *Contour;
    char Name[BUFFSIZE]; 
    list <ftag> tagl;
    
};


class op_tool:public new_operation
{
    friend op_rectangle;
    friend op_threading;
    friend op_drilling;
    friend op_shape;
    public:
    op_tool();
    ~op_tool();
    const char* name() ;
    op_type type();
    void draw( color c=NONE, bool path = true );
    void save_program( FILE *fp ); 
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);
    void update();
    
    protected:
    
    tool tl;

};


class op_rectangle:public new_operation
{
    public:
    op_rectangle();
    ~op_rectangle();
    const char* name() ;
    op_type type();
    void draw( color c=NONE, bool path = true  );
    void save_program( FILE *fp );
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);   
    void update();
    
    protected:
    
    path rect_path;
    vec2 begin,end;
    int feed_dir;
    
};


class op_threading:public new_operation
{
    public:
    op_threading();
    ~op_threading();
    const char* name() ;
    op_type type();
    void draw( color c=NONE, bool path = true  );
    void save_program( FILE *fp );
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);   
    void update();
    
    protected:
    
    vec2 begin,end;
    double pitch, depth, degression, compound_angle, multip;
    int count, spring_passes;
    Side side;
    
};

class op_drilling:public new_operation
{
    public:
    op_drilling();
    ~op_drilling();
    const char* name() ;
    op_type type();
    void draw( color c=NONE, bool path = true  );
    void save_program( FILE *fp );
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);   
    void update();
    
    protected:
    vec2 begin,end;
    double peck;
    
};

#define MAX_FINISH 5

class op_shape:public new_operation
{
    public:
    op_shape();
    ~op_shape();
    const char* name() ;
    op_type type();
    void draw( color c=NONE, bool path = true  );
    void save_program( FILE *fp );
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);   
    void update();
    void load( FILE *fp );
    void save( FILE *fp );
    protected:
    path p,tp,rp,fp[MAX_FINISH+1];
    int fcount;
    bool changed;
    Side side;
};

