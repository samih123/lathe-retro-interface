
#define OP_EXIT 1
#define OP_EDITED 2
#define OP_NOP 3

class op_tool;
class op_contour;
class op_rectangle;

class new_operation
{
    public:
    virtual ~new_operation(){};
    
    virtual const char* name() { return "None"; }
    virtual op_type type() { return NOOP; };
    virtual void draw( color c=NONE ) {};
    virtual void save( FILE *fp ) {}; 
    virtual void save_program( FILE *fp ) {}; 
    virtual void load( FILE *fp ) {};
    virtual int parsemenu() { return OP_EXIT; };
    virtual void createmenu() {};
    virtual void drawmenu(int x,int y) {};
    virtual void update(){};
    void set_tool( op_tool *t ){ Tool = t; };
    void set_contour( op_contour *c ){ Contour = c; };
    
    protected:
    menu Menu;
    int Menuselect;
    op_tool *Tool;
    op_contour *Contour;
    char Name[BUFFSIZE]; 
};


class op_tool:public new_operation
{
    friend op_rectangle;
    public:
    op_tool();
    ~op_tool();
    const char* name() ;
    op_type type();
    void draw( color c=NONE );
    void save( FILE *fp ); 
    void save_program( FILE *fp ); 
    void load( FILE *fp );
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
    void draw( color c=NONE );
    void save( FILE *fp ); 
    void save_program( FILE *fp ); 
    void load( FILE *fp );
    int parsemenu();
    void createmenu();
    void drawmenu(int x,int y);   
    void update();
    
    protected:
    
    rectangle_path rect_path;
    vec2 begin,end;
    int feed_dir;
    
};
