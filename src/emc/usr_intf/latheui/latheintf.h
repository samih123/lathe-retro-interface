/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#pragma once

//#include <plib/pw.h>
#include <GL/freeglut.h>

#include <stdio.h>    
#include <stdlib.h>
#include <stdint.h>   
#include <string.h>   
#include <unistd.h>   
#include <errno.h>    
#include <termios.h>  
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>

#include "rcs.hh"
#include "posemath.h"       // PM_POSE, TO_RAD
#include "emc.hh"       // EMC NML
#include "canon.hh"     // CANON_UNITS, CANON_UNITS_INCHES,MM,CM
#include "emcglb.h"     // EMC_NMLFILE, TRAJ_MAX_VELOCITY, etc.
#include "emccfg.h"     // DEFAULT_TRAJ_MAX_VELOCITY
#include "inifile.hh"       // INIFILE
#include "config.h"     // Standard path definitions
#include "rcs_print.hh"
#include "tool_parse.h"
#include "emc.hh"
#include "../shcom.hh"
#include <list>
using namespace std;

#define CLAMP(x, l, h) (x = (((x) > (h)) ? (h) : (((x) < (l)) ? (l) : (x))))
//#define CLAMP( a, l , h ) (a=min( max(a, (l)), (h)))

#define AXISX 0
#define AXISZ 2
#define AXISC 5

#define SCREENHOME 8
#define SCREENSET 7
#define SCREENMANUAL 6
#define SCREENMDI 5
#define SCREENWIZARDS 4
#define SCREENEDIT 3
#define SCREENFILE 2
#define SCREENAUTO 1

#define REFRESHRATE 100  // per second.

#define BUFFSIZE 1000

#define RED 1
#define GREEN 2
#define BLUE 3
#define ORANGE 4
#define YELLOW 5
#define GREY 6
#define WHITE 7
#define MAGENTA 8

#define MAXTOOLS 20

class vec2
{
public:
    vec2(double X = 0, double Z = 0)
    {
        x = X;
        z = Z;
    };
    ~vec2() {} ;
 
    double x, z;
 
    vec2 &operator=(const vec2 &v) {
        x = v.x;
        z = v.z;
        return *this;
    }
    
    vec2 &operator+=(const vec2 &v) {
        x += v.x;
        z += v.z;
        return *this;
    }
    
    vec2 &operator+=(double a) {
        x += a;
        z += a;
        return *this;
    }
    
    vec2 &operator-=( const vec2 &v) {
        x -= v.x;
        z -= v.z;
        return *this;
    }
    
    vec2 &operator-=(double a) {
        x -= a;
        z -= a;
        return *this;
    }
    
    vec2 &operator*=(double a) {
        x *= a;
        z *= a;
        return *this;
    } 
      
    vec2 operator*(double s) const
    {
        return vec2(x * s, z * s);
    }
    
    vec2 operator*(const vec2 &v) const
    {
        return vec2(x * v.x, z * v.z);
    } 
    
    vec2 operator/(const vec2 &v) const
    {
        return vec2(x / v.x, z / v.z);
    }  
       
    vec2 operator/(double s) const
    {
        return vec2(x / s, z / s);
    }    
    
    vec2 operator+(const vec2 &v) const
    {
        return vec2(x + v.x, z + v.z);
    }
 
    vec2 operator-(const vec2 &v) const
    {
        return vec2(x - v.x, z - v.z);
    }
    
    vec2 operator+(const double a) const
    {
        return vec2(x + a, z + a);
    } 
     
    vec2 operator-(const double a) const
    {
        return vec2(x - a, z - a);
    } 

     friend bool operator==(const vec2 &v1, const vec2 &v2) 
     {
         return v1.x==v2.x && v1.z==v2.z;
     }   
     
     friend bool operator!=(const vec2 &v1, const vec2 &v2) 
     {
         return v1.x!=v2.x || v1.z!=v2.z;
     }   
     
    void rotate(double angle)
    {
        double xt = (x * cosf(angle)) - (z * sinf(angle));
        double zt = (z * cosf(angle)) + (x * sinf(angle));
        x = xt;
        z = zt;
    }
 
    double perp(const vec2 &v) const
    {
        return (this->x * v.z) - (this->z * v.x);
    }
 
    void normalise()
    {
        double l = sqrtf(x*x + z*z);
        this->x = x / l;
        this->z = z / l;
    }
    
    void findminmax( vec2 &min, vec2 &max )
    {
        if( min.x > this->x ) min.x = this->x;
        if( min.z > this->z ) min.z = this->z;
        if( max.x < this->x ) max.x = this->x;
        if( max.z < this->z ) max.z = this->z;        
    }
    
    vec2 dir() const
    {
        vec2 v2(this->x, this->z);
        v2.normalise();
        return v2;
    }
    
    vec2 normal( const vec2 &v ) const
    {
        vec2 v2(this->z - v.z , -(this->x - v.x) );
        v2.normalise();
        return v2;
    }   
    
    
    double dot(const vec2 &v) const
    {
        return (x * v.x) + (z * v.z);
    }
    
    double det(const vec2 &v) const
    {
        return (x * v.z) - (z * v.x);
    }  
      
    double angle(const vec2 &v) const
    {   
        double dot1 = (x)*(v.x) + (z)*(v.z);
        double det1 = (x)*(v.z) - (z)*(v.x);
        return atan2(det1, dot1);
       // return atan2( this->det(v), this->dot(v) );
    }  
         
             
    double dist(const vec2 v) const {
        return sqrt( (x-v.x)*(x-v.x) + (z-v.z)*(z-v.z) );
    }
    
    double dist_squared(const vec2 v) const {
        return (x-v.x)*(x-v.x) + (z-v.z)*(z-v.z);
    }    
    
    double length() const {
        return sqrt(x * x + z * z);
    }
    
    double length_squared() const {
        return x * x + z * z;
    }
};

struct machinestatus
{
    int screenmode;
    int mode;
    int axis;
    int feedoverride, feedoverride_new;
    int jogfeedrate;
    int maxrpm;
    double incr;
    bool singleblock;
    bool skipblock;
    bool optionalstop;
    bool slowrapid;
};


struct menuitem
{
    menuitem()
    {
        name[0] = 0;
        type = 0;
        hidden = false;
        edited = false;
        shortcut = NULL;
        color = GREEN;
    };
    ~menuitem()
    {
        ml.clear();
    }
    
    void *val;
    int type;
    int num;
    char name[BUFFSIZE];
    char str[BUFFSIZE];
    const char *shortcut;     
    list<struct menuitem> ml;
    list<struct menuitem>::iterator it;
    menuitem *up;
    bool hidden;
    bool edited;
    int color;
}; 


class menu
{
    public:
    menu();
    void clear();
    void begin( const char *name );
    void end();
    void setmaxlines( int l );
    
    void hiddenvalue();
    void color( int c );
    void shortcut( const char *shortcut );
    
    void edit( int *i, const char *n);
    void select( int *i, int num, const char *n );
    void edit( double *d, const char *n );
    void edit( char *s, const char *n );
    void edit( bool *b, const char *n );
    void back( const char *n );
    void show( const char *n );
    
    bool edited( void *v );
    bool current_menu( const char *n );
    void draw( int x, int y);
    bool parse();
    
    private:
    bool edited( menuitem &m, void *v );
    char strbuf[BUFFSIZE];
    menuitem rm;
    menuitem *cmi; // current menuitem
    bool usewheel;
    bool point;
    int maxlines;
};

void init_opengl( int argc, char **argv );
void setcolor( int color );
void print(const char *s, int x, int y ,int size);
void print(const char *s, int x, int y ,int size, int color );
void println(const char *s, int x, int y, int size = 10 , int color = GREEN );
void println(const char *s, int color = GREEN);

void draw_dro();
void draw_statusbar( const char *s );
void updatescreen();
void drawCross(GLfloat x, GLfloat y, GLfloat size);
void drawCircle(GLfloat x, GLfloat y, GLfloat radius);
void draw_tool( int i );
    
void file_init();
void file_parse_serialdata();
void file_draw();

void home_init();
void home_parse_serialdata();
void home_draw();

void manual_init();
void manual_parse_serialdata();
void manual_draw();

void mdi_init();
void mdi_parse_serialdata();
void mdi_draw();
void mdicommand( const char *c );

void set_init();
void set_parse_serialdata();
void set_draw();

void edit_init();
void edit_parse_serialdata();
void edit_draw();
void edit_load( const char *file );
void edit_save( const char *file );

void auto_init();
void auto_parse_serialdata();
void auto_draw();
void auto_load( char *n );

void wizards_init();
void wizards_parse_serialdata();
void wizards_draw();
void wizards_load( const char *name );

const char* isprefix( const char*  prefix, int* n);

#define FEED_RAPID 1
#define FEED_NORMAL 2

int preview( char *file );
void preview_addfeed( float x, float y, float x2, float y2);
void preview_addrapid( float x, float y, float x2, float y2);
void preview_draw();

void set_previewZoffset( double z );
void set_previewXoffset( double z );





