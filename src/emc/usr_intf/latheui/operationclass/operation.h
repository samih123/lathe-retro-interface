
class operation
{
    public:
    operation()
    {
       
    }
    
    operation( int t )
    {
       type = type;
    }
    
    ~operation()
    {
       
    }
    
    void draw( int x1,int y1,int x2,int y2);
    
    //protected:
    cutparam cp;
    int type;
    std::list<struct cut> cl;
    vec2 min,max;    

};






