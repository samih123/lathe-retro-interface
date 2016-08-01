

class op_tool;
class op_contour;

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
    virtual bool parse() { return true; };
    virtual void createmenu() {};
    virtual void drawmenu(int x,int y) {};
    void set_tool( new_operation *t ){ Tool = t; };
    void set_contour( new_operation *c ){ Contour = c; };
    
    protected:
    menu Menu;
    int Menuselect;
    new_operation *Tool;
    new_operation *Contour;
    
};


class op_tool:public new_operation
{
    public:
    op_tool();
    ~op_tool();
    const char* name() ;
    op_type type();
    void draw( color c=NONE );
    void save( FILE *fp ); 
    void save_program( FILE *fp ); 
    void load( FILE *fp );
    bool parse();
    void createmenu();
    void drawmenu(int x,int y);    
    
};


