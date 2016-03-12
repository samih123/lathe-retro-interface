/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

#include "canon.hh"
#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include <list>
using namespace std;

/* where to print */
//extern FILE * _outfile;
FILE * _outfile=NULL;      /* where to print, set in main */

/* Dummy world model */

static CANON_PLANE       _active_plane = CANON_PLANE_XZ;
static int               _active_slot = 1;
static int               _feed_mode = 0;
static double            _feed_rate = 0.0;
static int               _flood = 0;
static double            _length_unit_factor = 1; /* 1 for MM 25.4 for inch */
static CANON_UNITS       _length_unit_type = CANON_UNITS_MM;
static int               _line_number = 1;
static int               _mist = 0;
static CANON_MOTION_MODE _motion_mode = CANON_CONTINUOUS;
char                     _parameter_file_name[PARAMETER_FILE_NAME_LENGTH];/*Not static.Driver writes*/
static double            _probe_position_a = 0; /*AA*/
static double            _probe_position_b = 0; /*BB*/
static double            _probe_position_c = 0; /*CC*/
static double            _probe_position_x = 0;
static double            _probe_position_y = 0;
static double            _probe_position_z = 0;
static double _g5x_x, _g5x_y, _g5x_z;
static double _g5x_a, _g5x_b, _g5x_c;
static double _g5x_u, _g5x_v, _g5x_w;
static double _g92_x, _g92_y, _g92_z;
static double _g92_a, _g92_b, _g92_c;
static double _g92_u, _g92_v, _g92_w;
static double            _program_position_a = 0; /*AA*/
static double            _program_position_b = 0; /*BB*/
static double            _program_position_c = 0; /*CC*/
static double            _program_position_x = 0;
static double            _program_position_y = 0;
static double            _program_position_z = 0;
static double            _spindle_speed;
static CANON_DIRECTION   _spindle_turning;
int                      _pockets_max = CANON_POCKETS_MAX; /*Not static. Driver reads  */
CANON_TOOL_TABLE         _tools[CANON_POCKETS_MAX]; /*Not static. Driver writes */
/* optional program stop */
static bool optional_program_stop = OFF; //set enabled by default (previous EMC behaviour)
/* optional block delete */
static bool block_delete = OFF; //set enabled by default (previous EMC behaviour)
static double motion_tolerance = 0.;
static double naivecam_tolerance = 0.;
/* Dummy status variables */
static double            _traverse_rate;

static EmcPose _tool_offset;
static bool _toolchanger_fault;
static int  _toolchanger_reason;

/************************************************************************/

/* Canonical "Do it" functions

This is a set of dummy definitions for the canonical machining functions
given in canon.hh. These functions just print themselves and, if necessary,
update the dummy world model. On each output line is printed:
1. an output line number (sequential, starting with 1).
2. an input line number read from the input (or ... if not provided).
3. a printed representation of the function call which was made.

If an interpreter which makes these calls is compiled with this set of
definitions, it can be used as a translator by redirecting output from
stdout to a file.

*/

//extern void rs274ngc_line_text(char * line_text, int max_size);
extern InterpBase *pinterp;
#define interp_new (*pinterp)

double rotation_cos, rotation_sin;




static void unrotate(double &x, double &y, double c, double s) {
    double tx = x * c + y * s;
    y = -x * s + y * c;
    x = tx;
}

static void rotate(double &x, double &y, double c, double s) {
    double tx = x * c - y * s;
    y = x * s + y * c;
    x = tx;
}

void rs274_arc_to_segments(
        double x1, double y1, double cx, double cy, int rot, double z1, double a,
        double b, double c, double u, double v, double w
    )
    {

//printf("arc_to_segments %f,%f %f,%f\n", x1, y1, cx, cy);

    double o[9], n[9], g5xoffset[9], g92offset[9];
    int X, Y, Z;
    int max_segments = 128;


    g5xoffset[0]  = _g5x_x;
    g5xoffset[1]  = _g5x_y;
    g5xoffset[2]  = _g5x_z;
    g5xoffset[3]  = _g5x_a;
    g5xoffset[4]  = _g5x_b;
    g5xoffset[5]  = _g5x_c;
    g5xoffset[6]  = _g5x_u;
    g5xoffset[7]  = _g5x_v;
    g5xoffset[8]  = _g5x_w;
    
    g92offset[0]  = _g92_x;
    g92offset[1]  = _g92_y;
    g92offset[2]  = _g92_z;
    g92offset[3]  = _g92_a;
    g92offset[4]  = _g92_b;
    g92offset[5]  = _g92_c;
    g92offset[6]  = _g92_u;
    g92offset[7]  = _g92_v;
    g92offset[8]  = _g92_w;
    
    o[0] = _program_position_x;
    o[1] = _program_position_y;
    o[2] = _program_position_z;
    o[3] = _program_position_a;
    o[4] = _program_position_b;
    o[5] = _program_position_c;
    o[6] = 0;
    o[7] = 0;
    o[8] = 0;
        
        
    // numbering of axes, depending on plane
    if(_active_plane == 1) { // XY-plane
        X=0; Y=1; Z=2;
    } else if(_active_plane == 3) { // YZ-plane
        X=2; Y=0; Z=1;
    } else {
        X=1; Y=2; Z=0; // XZ-plane
    }
    n[X] = x1; // end-point, first-coord
    n[Y] = y1; // end-point, second-coord
    n[Z] = z1; // end-point, third-coord, i.e helix translation
    
    n[3] = a;
    n[4] = b;
    n[5] = c;
    
    n[6] = u;
    n[7] = v;
    n[8] = w;

   //for(int ax=0; ax<9; ax++) 
     //   o[ax] -= g5xoffset[ax]; // offset
    //unrotate(o[0], o[1], rotation_cos, rotation_sin); // only rotates in XY (?)
    //for(int ax=0; ax<9; ax++) 
      //  o[ax] -= g92offset[ax]; // offset

    double theta1 = atan2( o[Y]-cy, o[X]-cx); // vector from center to start
    
    double theta2 = atan2( n[Y]-cy, n[X]-cx); // vector from center to end
    
    // "unwind" theta2 ?
    if(rot < 0) { // rot = -1
        while(theta2 - theta1 > -CIRCLE_FUZZ) 
            theta2 -= 2*M_PI;
    } else { // rot = 1
        while(theta2 - theta1 < CIRCLE_FUZZ) 
            theta2 += 2*M_PI;
    }

    // if multi-turn, add the right number of full circles
    if(rot < -1) 
        theta2 += 2*M_PI*(rot+1);
    if(rot > 1) 
        theta2 += 2*M_PI*(rot-1);
        
    // number of steps
    int steps = std::max( 3, int(max_segments * fabs(theta1 - theta2) / M_PI) ); //  max_segments per M_PI of rotation
    double rsteps = 1. / steps;
    double dtheta = theta2 - theta1; // the rotation angle for this helix
    // n is endpoint
    // o is startpoint
    
    // d is diff.   x,y,z,a,b,c,u,v,w 
                // 0  1  2  3          4          5          6          7
    double d[9] = {0, 0, 0, n[4]-o[4], n[5]-o[5], n[6]-o[6], n[7]-o[7], n[8]-o[8]};
    d[Z] = n[Z] - o[Z]; // helix-translation diff

    double tx = o[X] - cx; // center to start 
    double ty = o[Y] - cy; // center to start
    double dc = cos(dtheta*rsteps);  
    double ds = sin(dtheta*rsteps);
    
    double ex_x = 0, ex_y = 0;
    bool first = true;
    
    // generate segments:
    for(int i=0; i<steps-1; i++) {
        double f = (i+1) * rsteps; // varies from 0..1 (?)
        double p[9]; // point
        rotate(tx, ty, dc, ds); // rotate center-start vector by a small amount
        p[X] = tx + cx; // center + rotated point
        p[Y] = ty + cy;
        p[Z] = o[Z] + d[Z] * f; // start + helix-translation
        
        // simple proportional translation
        p[3] = o[3] + d[3] * f;
        p[4] = o[4] + d[4] * f;
        p[5] = o[5] + d[5] * f;
        p[6] = o[6] + d[6] * f;
        p[7] = o[7] + d[7] * f;
        p[8] = o[8] + d[8] * f;
        

        // offset
       // for(int ax=0; ax<9; ax++) 
         //   p[ax] += g92offset[ax];
       // rotate(p[0], p[1], rotation_cos, rotation_sin); // rotation offset on XY
        // offset
        //for(int ax=0; ax<9; ax++) 
          //  p[ax] += g5xoffset[ax];
       
         //printf("%f %f %f %f %f %f %f %f %f\n", p[0], p[1], p[2],p[3], p[4], p[5],p[6], p[7], p[8]);
         
         if( first )
         {
             first = false;
             preview_addfeed( _program_position_z , -_program_position_x , p[X] , -p[Y]  );
         }
         else
         {
             preview_addfeed( ex_x , ex_y , p[X] , -p[Y]  ); 
         }
         
         ex_x = p[X];
         ex_y = -p[Y];
    }
    
    preview_addfeed( ex_x , ex_y , n[X] , -n[Y]  ); 

// since we reversed g92, rotation-offset, and g5x, these need to be reapplied before
// adding the final point.
    // apply g92 offset
    for(int ax=0; ax<9; ax++) 
        n[ax] += g92offset[ax];
    // rotate
    rotate(n[0], n[1], rotation_cos, rotation_sin);
    // apply g5x offset
    for(int ax=0; ax<9; ax++) 
        n[ax] += g5xoffset[ax];
    
    // apply a final point at the end of the list.
    //PyList_SET_ITEM(segs, steps-1,
      //
        //    Py_BuildValue("ddddddddd", n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8]));
}






void print_nc_line_number()
{
    char text[256];
    int k;
    int m;

    if(NULL == _outfile)
    {
        _outfile = stdout;
    }

    interp_new.line_text(text, 256);
    for (k = 0;
            ((k < 256) &&
             ((text[k] == '\t') || (text[k] == ' ') || (text[k] == '/')));
            k++);
    if ((k < 256) && ((text[k] == 'n') || (text[k] == 'N')))
    {
        fputc('N', _outfile);
        for (k++, m = 0;
                ((k < 256) && (text[k] >= '0') && (text[k] <= '9'));
                k++, m++)
            fputc(text[k], _outfile);
        for (; m < 6; m++)
            fputc(' ', _outfile);
    }
    else if (k < 256)
        fprintf(_outfile, "N..... ");
}


#define PRINT0(control) if (0)                        \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++); \
           print_nc_line_number();                    \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control);                \
          } 
#define PRINT1(control, arg1) if (0)                  \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++); \
           print_nc_line_number();                    \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1);          \
          } 
#define PRINT2(control, arg1, arg2) if (1)            \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++); \
           print_nc_line_number();                    \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1, arg2);    \
          } else
#define PRINT3(control, arg1, arg2, arg3) if (1)         \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);    \
           print_nc_line_number();                       \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1, arg2, arg3); \
          } else
#define PRINT4(control, arg1, arg2, arg3, arg4) if (1)         \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);          \
           print_nc_line_number();                             \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1, arg2, arg3, arg4); \
          } else
#define PRINT5(control, arg1, arg2, arg3, arg4, arg5) if (1)         \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                \
           print_nc_line_number();                                   \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1, arg2, arg3, arg4, arg5); \
          } else
#define PRINT6(control, arg1, arg2, arg3, arg4, arg5, arg6) if (1)         \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                      \
           print_nc_line_number();                                         \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control, arg1, arg2, arg3, arg4, arg5, arg6); \
          } else
#define PRINT7(control, arg1, arg2, arg3, arg4, arg5, arg6, arg7) if (1) \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                    \
           print_nc_line_number();                                       \
           {if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  control,                                    \
                           arg1, arg2, arg3, arg4, arg5, arg6, arg7);    \
          } else
#define PRINT9(control,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9) \
          if (1)                                                            \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                       \
           print_nc_line_number();                                          \
           fprintf(_outfile, control,                                       \
                   arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9);           \
          } else
#define PRINT10(control,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10) \
          if (1)                                                            \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                       \
           print_nc_line_number();                                          \
           fprintf(_outfile, control,                                       \
                   arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);     \
          } else
#define PRINT14(control,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13,arg14) \
          if (1)                                                            \
          {{if(_outfile==NULL){_outfile=stdout;}} fprintf(_outfile,  "%5d ", _line_number++);                       \
           print_nc_line_number();                                          \
           fprintf(_outfile, control,                                       \
                   arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13,arg14); \
          } else

/* Representation */

void SET_XY_ROTATION(double theta) {
    fprintf(_outfile, "%5d ", _line_number++);
   print_nc_line_number();
    fprintf(_outfile, "SET_XY_ROTATION(%.4f)\n", theta);
    // CJR XXX    
    /*
    def set_xy_rotation(self, theta):
        self.rotation_xy = theta
        t = math.radians(theta)
        self.rotation_sin = math.sin(t)
        self.rotation_cos = math.cos(tD
        DEGREES_TO_RADIANS(degrees)((M_PI * degrees)/180)
        */
     
        double t = (M_PI * theta)/180.0f;
        rotation_sin = sin(t);
        rotation_cos = cos(t);
}


void set_previewXoffset( double x )
{
    _program_position_x = _program_position_x + _g5x_x - x;
    _g5x_x = x;
}

void set_previewZoffset( double z )
{
    _program_position_z = _program_position_z + _g5x_z - z;
    _g5x_z = z;
}

void SET_G5X_OFFSET(int index,
                    double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w) {
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "SET_G5X_OFFSET(%d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)\n",
            index, x, y, z, a, b, c);
    _program_position_x = _program_position_x + _g5x_x - x;
    _program_position_y = _program_position_y + _g5x_y - y;
    _program_position_z = _program_position_z + _g5x_z - z;
    _program_position_a = _program_position_a + _g5x_a - a;/*AA*/
    _program_position_b = _program_position_b + _g5x_b - b;/*BB*/
    _program_position_c = _program_position_c + _g5x_c - c;/*CC*/

    _g5x_x = x;
    _g5x_y = y;
    _g5x_z = z;
    _g5x_a = a;  /*AA*/
    _g5x_b = b;  /*BB*/
    _g5x_c = c;  /*CC*/
    _g5x_u = u;
    _g5x_v = v;
    _g5x_w = w;
}

void SET_G92_OFFSET(double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w) {
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "SET_G92_OFFSET(%.4f, %.4f, %.4f, %.4f, %.4f, %.4f)\n",
            x, y, z, a, b, c);
            
    _program_position_x = _program_position_x + _g92_x - x;
    _program_position_y = _program_position_y + _g92_y - y;
    _program_position_z = _program_position_z + _g92_z - z;
    _program_position_a = _program_position_a + _g92_a - a;/*AA*/
    _program_position_b = _program_position_b + _g92_b - b;/*BB*/
    _program_position_c = _program_position_c + _g92_c - c;/*CC*/

    _g92_x = x;
    _g92_y = y;
    _g92_z = z;
    _g92_a = a;  /*AA*/
    _g92_b = b;  /*BB*/
    _g92_c = c;  /*CC*/
    _g92_u = u;
    _g92_v = v;
    _g92_w = w;
}

void USE_LENGTH_UNITS(CANON_UNITS in_unit)
{
    if (in_unit == CANON_UNITS_INCHES)
    {
        printf("USE_LENGTH_UNITS(CANON_UNITS_INCHES)\n");
        if (_length_unit_type == CANON_UNITS_MM)
        {
            _length_unit_type = CANON_UNITS_INCHES;
            _length_unit_factor = 25.4;

            _program_position_x /= 25.4;
            _program_position_y /= 25.4;
            _program_position_z /= 25.4;

            _g5x_x /= 25.4;
            _g5x_y /= 25.4;
            _g5x_z /= 25.4;

            _g92_x /= 25.4;
            _g92_y /= 25.4;
            _g92_z /= 25.4;
        }
    }
    else if (in_unit == CANON_UNITS_MM)
    {
        printf("USE_LENGTH_UNITS(CANON_UNITS_MM)\n");
        if (_length_unit_type == CANON_UNITS_INCHES)
        {
            _length_unit_type = CANON_UNITS_MM;
            _length_unit_factor = 1.0;

            _program_position_x *= 25.4;
            _program_position_y *= 25.4;
            _program_position_z *= 25.4;

            _g5x_x *= 25.4;
            _g5x_y *= 25.4;
            _g5x_z *= 25.4;

            _g92_x *= 25.4;
            _g92_y *= 25.4;
            _g92_z *= 25.4;
        }
    }
    else
        printf("USE_LENGTH_UNITS(UNKNOWN)\n");
}

/* Free Space Motion */
void SET_TRAVERSE_RATE(double rate)
{
    PRINT1("SET_TRAVERSE_RATE(%.4f)\n", rate);
    _traverse_rate = rate;
}

void STRAIGHT_TRAVERSE( int line_number,
                        double x, double y, double z
                        , double a /*AA*/
                        , double b /*BB*/
                        , double c /*CC*/
                        , double u, double v, double w
                      )
{
    /*
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "STRAIGHT_TRAVERSE(%.4f, %.4f, %.4f"
            ", %.4f" 
            ", %.4f" 
            ", %.4f" 
            ")\n", x, y, z
            , a 
            , b 
            , c 
           );
      */
      
    preview_addrapid( z, -x, _program_position_z, -_program_position_x);
    
    _program_position_x = x;
    _program_position_y = y;
    _program_position_z = z;
    _program_position_a = a; /*AA*/
    _program_position_b = b; /*BB*/
    _program_position_c = c; /*CC*/
}

/* Machining Attributes */
void SET_FEED_MODE(int mode)
{
    PRINT1("SET_FEED_MODE(%d)\n", mode);
    _feed_mode = mode;
}
void SET_FEED_RATE(double rate)
{
    PRINT1("SET_FEED_RATE(%.4f)\n", rate);
    _feed_rate = rate;
}

void SET_FEED_REFERENCE(CANON_FEED_REFERENCE reference)
{
    PRINT1("SET_FEED_REFERENCE(%s)\n",
           (reference == CANON_WORKPIECE) ? "CANON_WORKPIECE" : "CANON_XYZ");
}

extern void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode, double tolerance)
{
    motion_tolerance = 0;
    if (mode == CANON_EXACT_STOP)
    {
        PRINT0("SET_MOTION_CONTROL_MODE(CANON_EXACT_STOP)\n");
        _motion_mode = CANON_EXACT_STOP;
    }
    else if (mode == CANON_EXACT_PATH)
    {
        PRINT0("SET_MOTION_CONTROL_MODE(CANON_EXACT_PATH)\n");
        _motion_mode = CANON_EXACT_PATH;
    }
    else if (mode == CANON_CONTINUOUS)
    {
        motion_tolerance = tolerance;
        PRINT1("SET_MOTION_CONTROL_MODE(CANON_CONTINUOUS, %f)\n", tolerance);
        _motion_mode = CANON_CONTINUOUS;
    }
    else
        PRINT0("SET_MOTION_CONTROL_MODE(UNKNOWN)\n");
}

extern void SET_NAIVECAM_TOLERANCE(double tolerance)
{
    naivecam_tolerance = tolerance;
    PRINT1("SET_NAIVECAM_TOLERANCE(%.4f)\n", tolerance);
}

void SELECT_PLANE(CANON_PLANE in_plane)
{
    printf("SELECT_PLANE(CANON_PLANE_%s)\n",
           ((in_plane == CANON_PLANE_XY) ? "XY" :
            (in_plane == CANON_PLANE_YZ) ? "YZ" :
            (in_plane == CANON_PLANE_XZ) ? "XZ" : "UNKNOWN"));
    _active_plane = in_plane;
}

void SET_CUTTER_RADIUS_COMPENSATION(double radius)
{
    printf("SET_CUTTER_RADIUS_COMPENSATION(%.4f)\n", radius);
}

void START_CUTTER_RADIUS_COMPENSATION(int side)
{   printf("START_CUTTER_RADIUS_COMPENSATION(%s)\n",
           (side == CANON_SIDE_LEFT)  ? "LEFT"  :
           (side == CANON_SIDE_RIGHT) ? "RIGHT" : "UNKNOWN");
}

void STOP_CUTTER_RADIUS_COMPENSATION()
{
    PRINT0 ("STOP_CUTTER_RADIUS_COMPENSATION()\n");
}

void START_SPEED_FEED_SYNCH()
{
    PRINT0 ("START_SPEED_FEED_SYNCH()\n");
}

void STOP_SPEED_FEED_SYNCH()
{
    PRINT0 ("STOP_SPEED_FEED_SYNCH()\n");
}

/* Machining Functions */

void NURBS_FEED(int lineno,
                std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k)
{
    //fprintf(_outfile, "%5d ", _line_number++);
    //print_nc_line_number();
    //fprintf(_outfile, "NURBS_FEED(%lu, ...)\n", (unsigned long)nurbs_control_points.size());

    _program_position_x = nurbs_control_points[nurbs_control_points.size()].X;
    _program_position_y = nurbs_control_points[nurbs_control_points.size()].Y;
}


/*
void rs274_arc_to_segments(
        double x1, double y1, double cx, double cy, int rot, double z1, double a,
        double b, double c, double u, double v, double w
    )*/


void ARC_FEED(int line_number,
              double first_end, double second_end,
              double first_axis, double second_axis, int rotation, double axis_end_point
              , double a /*AA*/
              , double b /*BB*/
              , double c /*CC*/
              , double u, double v, double w
             )
{   
    if(0)
    {
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "ARC_FEED(%.4f, %.4f, %.4f, %.4f, %d, %.4f"
            ", %.4f" /*AA*/
            ", %.4f" /*BB*/
            ", %.4f" /*CC*/
            ")\n", first_end, second_end, first_axis, second_axis,
            rotation, axis_end_point
            , a /*AA*/
            , b /*BB*/
            , c /*CC*/
           );
       }
       
    rs274_arc_to_segments( first_end, second_end, first_axis, second_axis, rotation, 0,
                            a,b,c,u,v,w );
       
    if (_active_plane == CANON_PLANE_XY)
    {
        _program_position_x = first_end;
        _program_position_y = second_end;
        _program_position_z = axis_end_point;
    }
    else if (_active_plane == CANON_PLANE_YZ)
    {
        _program_position_x = axis_end_point;
        _program_position_y = first_end;
        _program_position_z = second_end;
    }
    else /* if (_active_plane == CANON_PLANE_XZ) */
    {
        _program_position_x = second_end;
        _program_position_y = axis_end_point;
        _program_position_z = first_end;
    }
    _program_position_a = a; /*AA*/
    _program_position_b = b; /*BB*/
    _program_position_c = c; /*CC*/
}

void STRAIGHT_FEED(int line_number,
                   double x, double y, double z
                   , double a /*AA*/
                   , double b /*BB*/
                   , double c /*CC*/
                   , double u, double v, double w
                  )
{
    if(0){
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "STRAIGHT_FEED(%.4f, %.4f, %.4f"
            ", %.4f" /*AA*/
            ", %.4f" /*BB*/
            ", %.4f" /*CC*/
            ")\n", x, y, z
            , a /*AA*/
            , b /*BB*/
            , c /*CC*/
           );
       }
       
    preview_addfeed( z, -x, _program_position_z, -_program_position_x);
       
    _program_position_x = x;
    _program_position_y = y;
    _program_position_z = z;
    _program_position_a = a; /*AA*/
    _program_position_b = b; /*BB*/
    _program_position_c = c; /*CC*/
}


/* This models backing the probe off 0.01 inch or 0.254 mm from the probe
point towards the previous location after the probing, if the probe
point is not the same as the previous point -- which it should not be. */

void STRAIGHT_PROBE(int line_number,
                    double x, double y, double z
                    , double a /*AA*/
                    , double b /*BB*/
                    , double c /*CC*/
                    , double u, double v, double w, unsigned char probe_type
                   )
{
    double distance;
    double dx, dy, dz;
    double backoff;

    dx = (_program_position_x - x);
    dy = (_program_position_y - y);
    dz = (_program_position_z - z);
    distance = sqrt((dx * dx) + (dy * dy) + (dz * dz));

    if(0){
        
    fprintf(_outfile, "%5d ", _line_number++);
    print_nc_line_number();
    fprintf(_outfile, "STRAIGHT_PROBE(%.4f, %.4f, %.4f"
            ", %.4f" /*AA*/
            ", %.4f" /*BB*/
            ", %.4f" /*CC*/
            ")\n", x, y, z
            , a /*AA*/
            , b /*BB*/
            , c /*CC*/
           );
       }
    _probe_position_x = x;
    _probe_position_y = y;
    _probe_position_z = z;
    _probe_position_a = a; /*AA*/
    _probe_position_b = b; /*BB*/
    _probe_position_c = c; /*CC*/
    if (distance != 0)
    {
        backoff = ((_length_unit_type == CANON_UNITS_MM) ? 0.254 : 0.01);
        _program_position_x = (x + (backoff * (dx / distance)));
        _program_position_y = (y + (backoff * (dy / distance)));
        _program_position_z = (z + (backoff * (dz / distance)));
    }
    _program_position_a = a; /*AA*/
    _program_position_b = b; /*BB*/
    _program_position_c = c; /*CC*/
}


void RIGID_TAP(int line_number, double x, double y, double z)
{


    //fprintf(_outfile, "%5d ", _line_number++);
    //print_nc_line_number();
    //fprintf(_outfile, "RIGID_TAP(%.4f, %.4f, %.4f)\n", x, y, z);

}


void DWELL(double seconds)
{
    PRINT1("DWELL(%.4f)\n", seconds);
}

/* Spindle Functions */
void SPINDLE_RETRACT_TRAVERSE()
{
    PRINT0("SPINDLE_RETRACT_TRAVERSE()\n");
}

void SET_SPINDLE_MODE(double arg) {
    PRINT1("SET_SPINDLE_MODE(%.4f)\n", arg);
}

void START_SPINDLE_CLOCKWISE()
{
    PRINT0("START_SPINDLE_CLOCKWISE()\n");
    _spindle_turning = ((_spindle_speed == 0) ? CANON_STOPPED :
                        CANON_CLOCKWISE);
}

void START_SPINDLE_COUNTERCLOCKWISE()
{
    PRINT0("START_SPINDLE_COUNTERCLOCKWISE()\n");
    _spindle_turning = ((_spindle_speed == 0) ? CANON_STOPPED :
                        CANON_COUNTERCLOCKWISE);
}

void SET_SPINDLE_SPEED(double rpm)
{
    PRINT1("SET_SPINDLE_SPEED(%.4f)\n", rpm);
    _spindle_speed = rpm;
}

void STOP_SPINDLE_TURNING()
{
    PRINT0("STOP_SPINDLE_TURNING()\n");
    _spindle_turning = CANON_STOPPED;
}

void SPINDLE_RETRACT()
{
    PRINT0("SPINDLE_RETRACT()\n");
}

void ORIENT_SPINDLE(double orientation, int mode)
{   PRINT2("ORIENT_SPINDLE(%.4f, %d)\n", orientation,mode);
}

void WAIT_SPINDLE_ORIENT_COMPLETE(double timeout)
{
    PRINT1("SPINDLE_WAIT_ORIENT_COMPLETE(%.4f)\n", timeout);
}

void USE_NO_SPINDLE_FORCE()
{
    PRINT0("USE_NO_SPINDLE_FORCE()\n");
}

/* Tool Functions */
void SET_TOOL_TABLE_ENTRY(int pocket, int toolno, EmcPose offset, double diameter,
                          double frontangle, double backangle, int orientation) {
    _tools[pocket].toolno = toolno;
    _tools[pocket].offset = offset;
    _tools[pocket].diameter = diameter;
    _tools[pocket].frontangle = frontangle;
    _tools[pocket].backangle = backangle;
    _tools[pocket].orientation = orientation;
    printf("SET_TOOL_TABLE_ENTRY(%d, %d, %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f, %.4f, %.4f, %d)\n",
            pocket, toolno,
            offset.tran.x, offset.tran.y, offset.tran.z, offset.a, offset.b, offset.c, offset.u, offset.v, offset.w,
            frontangle, backangle, orientation);
}

void USE_TOOL_LENGTH_OFFSET(EmcPose offset)
{
    _tool_offset = offset;
    printf("USE_TOOL_LENGTH_OFFSET(%.4f %.4f %.4f, %.4f %.4f %.4f, %.4f %.4f %.4f)\n",
           offset.tran.x, offset.tran.y, offset.tran.z, offset.a, offset.b, offset.c, offset.u, offset.v, offset.w);
}

void CHANGE_TOOL(int slot)
{
    printf("CHANGE_TOOL(%d)\n", slot);
    _active_slot = slot;
    _tools[0] = _tools[slot];
}

void SELECT_POCKET(int slot, int tool)
{
    PRINT1("SELECT_POCKET(%d)\n", slot);
}

void CHANGE_TOOL_NUMBER(int slot)
{
    PRINT1("CHANGE_TOOL_NUMBER(%d)\n", slot);
    _active_slot = slot;
}


/* Misc Functions */

void CLAMP_AXIS(CANON_AXIS axis)
{   PRINT1("CLAMP_AXIS(%s)\n",
           (axis == CANON_AXIS_X) ? "CANON_AXIS_X" :
           (axis == CANON_AXIS_Y) ? "CANON_AXIS_Y" :
           (axis == CANON_AXIS_Z) ? "CANON_AXIS_Z" :
           (axis == CANON_AXIS_A) ? "CANON_AXIS_A" :
           (axis == CANON_AXIS_B) ? "CANON_AXIS_B" :
           (axis == CANON_AXIS_C) ? "CANON_AXIS_C" : "UNKNOWN");
}

void COMMENT(const char *s)
{
    PRINT1("COMMENT(\"%s\")\n", s);
}

void DISABLE_ADAPTIVE_FEED()
{
    PRINT0("DISABLE_ADAPTIVE_FEED()\n");
}

void DISABLE_FEED_HOLD()
{
    PRINT0("DISABLE_FEED_HOLD()\n");
}

void DISABLE_FEED_OVERRIDE()
{
    PRINT0("DISABLE_FEED_OVERRIDE()\n");
}

void DISABLE_SPEED_OVERRIDE()
{
    PRINT0("DISABLE_SPEED_OVERRIDE()\n");
}

void ENABLE_ADAPTIVE_FEED()
{
    PRINT0("ENABLE_ADAPTIVE_FEED()\n");
}

void ENABLE_FEED_HOLD()
{
    PRINT0("ENABLE_FEED_HOLD()\n");
}

void ENABLE_FEED_OVERRIDE()
{
    PRINT0("ENABLE_FEED_OVERRIDE()\n");
}

void ENABLE_SPEED_OVERRIDE()
{
    PRINT0("ENABLE_SPEED_OVERRIDE()\n");
}

void FLOOD_OFF()
{
    PRINT0("FLOOD_OFF()\n");
    _flood = 0;
}

void FLOOD_ON()
{
    PRINT0("FLOOD_ON()\n");
    _flood = 1;
}

void INIT_CANON()
{
}

void MESSAGE(char *s)
{
    PRINT1("MESSAGE(\"%s\")\n", s);
}

void LOG(char *s)
{
    PRINT1("LOG(\"%s\")\n", s);
}
void LOGOPEN(char *s)
{
    PRINT1("LOGOPEN(\"%s\")\n", s);
}
void LOGAPPEND(char *s)
{
    PRINT1("LOGAPPEND(\"%s\")\n", s);
}
void LOGCLOSE()
{
    PRINT0("LOGCLOSE()\n");
}

void MIST_OFF()
{
    PRINT0("MIST_OFF()\n");
    _mist = 0;
}

void MIST_ON()
{
    PRINT0("MIST_ON()\n");
    _mist = 1;
}

void PALLET_SHUTTLE()
{
    PRINT0("PALLET_SHUTTLE()\n");
}

void TURN_PROBE_OFF()
{
    PRINT0("TURN_PROBE_OFF()\n");
}

void TURN_PROBE_ON()
{
    PRINT0("TURN_PROBE_ON()\n");
}

void UNCLAMP_AXIS(CANON_AXIS axis)
{   PRINT1("UNCLAMP_AXIS(%s)\n",
           (axis == CANON_AXIS_X) ? "CANON_AXIS_X" :
           (axis == CANON_AXIS_Y) ? "CANON_AXIS_Y" :
           (axis == CANON_AXIS_Z) ? "CANON_AXIS_Z" :
           (axis == CANON_AXIS_A) ? "CANON_AXIS_A" :
           (axis == CANON_AXIS_B) ? "CANON_AXIS_B" :
           (axis == CANON_AXIS_C) ? "CANON_AXIS_C" : "UNKNOWN");
}

/* Program Functions */

void PROGRAM_STOP()
{
    PRINT0("PROGRAM_STOP()\n");
}

void SET_BLOCK_DELETE(bool state)
{
    block_delete = state;   //state == ON, means we don't interpret lines starting with "/"
}

bool GET_BLOCK_DELETE()
{
    return block_delete;   //state == ON, means we  don't interpret lines starting with "/"
}

void SET_OPTIONAL_PROGRAM_STOP(bool state)
{
    optional_program_stop = state;   //state == ON, means we stop
}

bool GET_OPTIONAL_PROGRAM_STOP()
{
    return optional_program_stop;   //state == ON, means we stop
}

void OPTIONAL_PROGRAM_STOP()
{
    PRINT0("OPTIONAL_PROGRAM_STOP()\n");
}

void PROGRAM_END()
{
    PRINT0("PROGRAM_END()\n");
}


/*************************************************************************/

/* Canonical "Give me information" functions

In general, returned values are valid only if any canonical do it commands
that may have been called for have been executed to completion. If a function
returns a valid value regardless of execution, that is noted in the comments
below.

*/

/* The interpreter is not using this function
// Returns the system angular unit factor, in units / degree
extern double GET_EXTERNAL_ANGLE_UNIT_FACTOR()
{
  return 1;
}
*/

/* MGS - FIXME - These functions should not be stubbed out to return constants.
They should return variable values... */
int GET_EXTERNAL_ADAPTIVE_FEED_ENABLE() {
    return 0;
}
int GET_EXTERNAL_FEED_OVERRIDE_ENABLE() {
    return 1;
}
double GET_EXTERNAL_MOTION_CONTROL_TOLERANCE() {
    return motion_tolerance;
}
double GET_EXTERNAL_LENGTH_UNITS() {
    return _length_unit_factor;
    //return 0.03937007874016;
}
int GET_EXTERNAL_FEED_HOLD_ENABLE() {
    return 1;
}
int GET_EXTERNAL_AXIS_MASK() {
    return 0x3f;   // XYZABC machine
}
double GET_EXTERNAL_ANGLE_UNITS() {
    return 1.0;
}
int GET_EXTERNAL_SELECTED_TOOL_SLOT() {
    return 0;
}
int GET_EXTERNAL_SPINDLE_OVERRIDE_ENABLE() {
    return 1;
}
void START_SPEED_FEED_SYNCH(double sync, bool vel)
{
    PRINT2("START_SPEED_FEED_SYNC(%f,%d)\n", sync, vel);
}
CANON_MOTION_MODE motion_mode;

int GET_EXTERNAL_DIGITAL_INPUT(int index, int def) {
    return def;
}
double GET_EXTERNAL_ANALOG_INPUT(int index, double def) {
    return def;
}
int WAIT(int index, int input_type, int wait_type, double timeout) {
    return 0;
}
int UNLOCK_ROTARY(int line_no, int axis) {
    return 0;
}
int LOCK_ROTARY(int line_no, int axis) {
    return 0;
}

/* Returns the system feed rate */
double GET_EXTERNAL_FEED_RATE()
{
    return _feed_rate;
}

/* Returns the system flood coolant setting zero = off, non-zero = on */
int GET_EXTERNAL_FLOOD()
{
    return _flood;
}

/* Returns the system length unit type */
CANON_UNITS GET_EXTERNAL_LENGTH_UNIT_TYPE()
{
    return _length_unit_type;
}

/* Returns the system mist coolant setting zero = off, non-zero = on */
extern int GET_EXTERNAL_MIST()
{
    return _mist;
}

// Returns the current motion control mode
extern CANON_MOTION_MODE GET_EXTERNAL_MOTION_CONTROL_MODE()
{
    return _motion_mode;
}


void GET_EXTERNAL_PARAMETER_FILE_NAME(
    char * file_name,       /* string: to copy file name into       */
    int max_size)           /* maximum number of characters to copy */
{
    // Paranoid checks
    if (0 == file_name)
        return;

    if (max_size < 0)
        return;

    if (strlen(_parameter_file_name) < (unsigned int)max_size)
        strcpy(file_name, _parameter_file_name);
    else
        file_name[0] = 0;
}

CANON_PLANE GET_EXTERNAL_PLANE()
{
    return _active_plane;
}

/* returns the current a-axis position */
double GET_EXTERNAL_POSITION_A()
{
    return _program_position_a;
}

/* returns the current b-axis position */
double GET_EXTERNAL_POSITION_B()
{
    return _program_position_b;
}

/* returns the current c-axis position */
double GET_EXTERNAL_POSITION_C()
{
    return _program_position_c;
}

/* returns the current x-axis position */
double GET_EXTERNAL_POSITION_X()
{
    return _program_position_x;
}

/* returns the current y-axis position */
double GET_EXTERNAL_POSITION_Y()
{
    return _program_position_y;
}

/* returns the current z-axis position */
double GET_EXTERNAL_POSITION_Z()
{
    return _program_position_z;
}

// XXX fix sai for uvw
double GET_EXTERNAL_POSITION_U()
{
    return 0.;
}

double GET_EXTERNAL_POSITION_V()
{
    return 0.;
}

double GET_EXTERNAL_POSITION_W()
{
    return 0.;
}

double GET_EXTERNAL_PROBE_POSITION_U()
{
    return 0.;
}

double GET_EXTERNAL_PROBE_POSITION_V()
{
    return 0.;
}

double GET_EXTERNAL_PROBE_POSITION_W()
{
    return 0.;
}

/* returns the a-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_A()
{
    return _probe_position_a;
}

/* returns the b-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_B()
{
    return _probe_position_b;
}

/* returns the c-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_C()
{
    return _probe_position_c;
}

/* returns the x-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_X()
{
    return _probe_position_x;
}

/* returns the y-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_Y()
{
    return _probe_position_y;
}

/* returns the z-axis position at the last probe trip. This is only valid
   once the probe command has executed to completion. */
double GET_EXTERNAL_PROBE_POSITION_Z()
{
    return _probe_position_z;
}

/* Returns the value for any analog non-contact probing. */
/* This is a dummy of a dummy, returning a useless value. */
/* It is not expected this will ever be called. */
extern double GET_EXTERNAL_PROBE_VALUE()
{
    return 1.0;
}

extern int GET_EXTERNAL_PROBE_TRIPPED_VALUE()
{
    return 0;
}

/* Returns zero if queue is not empty, non-zero if the queue is empty */
/* In the stand-alone interpreter, there is no queue, so it is always empty */
extern int GET_EXTERNAL_QUEUE_EMPTY()
{
    return 1;
}

/* Returns the system value for spindle speed in rpm */
double GET_EXTERNAL_SPEED()
{
    return _spindle_speed;
}

/* Returns the system value for direction of spindle turning */
extern CANON_DIRECTION GET_EXTERNAL_SPINDLE()
{
    return _spindle_turning;
}

/* Returns the system value for the carousel slot in which the tool
currently in the spindle belongs. Return value zero means there is no
tool in the spindle. */
extern int GET_EXTERNAL_TOOL_SLOT()
{
    return _active_slot;
}

/* Returns maximum number of pockets */
int GET_EXTERNAL_POCKETS_MAX()
{
    return _pockets_max;
}

/* Returns the CANON_TOOL_TABLE structure associated with the tool
   in the given pocket */
extern CANON_TOOL_TABLE GET_EXTERNAL_TOOL_TABLE(int pocket)
{
    return _tools[pocket];
}

/* Returns the system traverse rate */
double GET_EXTERNAL_TRAVERSE_RATE()
{
    return _traverse_rate;
}

USER_DEFINED_FUNCTION_TYPE USER_DEFINED_FUNCTION[USER_DEFINED_FUNCTION_NUM] = {0};

int USER_DEFINED_FUNCTION_ADD(USER_DEFINED_FUNCTION_TYPE func, int num)
{
    if (num < 0 || num >= USER_DEFINED_FUNCTION_NUM) {
        return -1;
    }

    USER_DEFINED_FUNCTION[num] = func;

    return 0;
}

void SET_MOTION_OUTPUT_BIT(int index)
{
    PRINT1("SET_MOTION_OUTPUT_BIT(%d)\n", index);
    return;
}

void CLEAR_MOTION_OUTPUT_BIT(int index)
{
    PRINT1("CLEAR_MOTION_OUTPUT_BIT(%d)\n", index);
    return;
}

void SET_MOTION_OUTPUT_VALUE(int index, double value)
{
    PRINT2("SET_MOTION_OUTPUT_VALUE(%d,%f)\n", index, value);
    return;
}

void SET_AUX_OUTPUT_BIT(int index)
{
    PRINT1("SET_AUX_OUTPUT_BIT(%d)\n", index);
    return;
}

void CLEAR_AUX_OUTPUT_BIT(int index)
{
    PRINT1("CLEAR_AUX_OUTPUT_BIT(%d)\n", index);
    return;
}

void SET_AUX_OUTPUT_VALUE(int index, double value)
{
    PRINT2("SET_AUX_OUTPUT_VALUE(%d,%f)\n", index, value);
    return;
}

double GET_EXTERNAL_TOOL_LENGTH_XOFFSET()
{
    return _tool_offset.tran.x;
}

double GET_EXTERNAL_TOOL_LENGTH_YOFFSET()
{
    return _tool_offset.tran.y;
}

double GET_EXTERNAL_TOOL_LENGTH_ZOFFSET()
{
    return _tool_offset.tran.z;
}

double GET_EXTERNAL_TOOL_LENGTH_AOFFSET()
{
    return _tool_offset.a;
}

double GET_EXTERNAL_TOOL_LENGTH_BOFFSET()
{
    return _tool_offset.b;
}

double GET_EXTERNAL_TOOL_LENGTH_COFFSET()
{
    return _tool_offset.c;
}

double GET_EXTERNAL_TOOL_LENGTH_UOFFSET()
{
    return _tool_offset.u;
}

double GET_EXTERNAL_TOOL_LENGTH_VOFFSET()
{
    return _tool_offset.v;
}

double GET_EXTERNAL_TOOL_LENGTH_WOFFSET()
{
    return _tool_offset.w;
}

void FINISH(void) {
    PRINT0("FINISH()\n");
}

void START_CHANGE(void) {
    PRINT0("START_CHANGE()\n");
}


int GET_EXTERNAL_TC_FAULT()
{
    return _toolchanger_fault;
}

int GET_EXTERNAL_TC_REASON()
{
    return _toolchanger_reason;
}

/* Sends error message */
void CANON_ERROR(const char *fmt, ...)
{
    va_list ap;

    if (fmt != NULL) {
        va_start(ap, fmt);
        char *err;
        int res = vasprintf(&err, fmt, ap);
        if(res < 0) err = 0;
        va_end(ap);
        if(err)
        {
            PRINT1("CANON_ERROR(%s)\n", err);
            free(err);
        }
        else
        {
            PRINT1("CANON_ERROR(vasprintf failed: %s)\n", strerror(errno));
        }
    }
}
void PLUGIN_CALL(int len, const char *call)
{
    printf("PLUGIN_CALL(%d)\n",len);
}

void IO_PLUGIN_CALL(int len, const char *call)
{
    printf("IO_PLUGIN_CALL(%d)\n",len);
}














