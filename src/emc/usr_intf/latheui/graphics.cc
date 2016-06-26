/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

extern int screenw, screenh;
extern char strbuf[BUFFSIZE];
extern struct machinestatus status;
extern bool flasher;
extern list<string> errors;
extern bool show_last_msg;

int print_y = 0;
int print_x = 0;
int print_s = 20;
list<string> ser_emul;

enum rawcolor
{
   RED, 
   GREEN,
   BLUE, 
   ORANGE,
   YELLOW,
   GREY,
   WHITE,
   MAGENTA,
   BLACK
};

void setrawcolor( rawcolor color )
{
    switch( color )
    {
        case RED    : glColor3f ( 1.0f, 0, 0 ); break;
        case BLUE   : glColor3f ( 0, 0, 1.0f ); break;
        case GREEN  : glColor3f ( 0, 1.0f, 0 ); break;
        case ORANGE : glColor3f ( 1.0f, 0.65f, 0.0f ); break;
        case YELLOW : glColor3f ( 1.0f, 1.0f, 0.0f ); break;
        case WHITE  : glColor3f ( 1.0f, 1.0f, 1.0f ); break;
        case GREY   : glColor3f ( 0.5f, 0.5f, 0.5f ); break;
        case MAGENTA: glColor3f ( 1.0f, 0.0f, 1.0f ); break;
        case BLACK  : glColor3f ( 0.0f, 0.0f, 0.0f ); break;
    }
}
   
void setcolor( color c )
{
    switch( c )
    {
        case BACKROUND: setrawcolor( BLACK ); break;
        case FEED: setrawcolor( YELLOW ); break;
        case RAPID: setrawcolor( RED ); break;
        case CONTOUR_LINE: setrawcolor( GREEN ); break;
        case CONTOUR_SHADOW: setrawcolor( GREY ); break;
        case CROSS: setrawcolor( GREY ); break;
        case OUTLINE: setrawcolor( GREY ); break;
        case CENTERLINE: setrawcolor( GREY ); break;
        case TEXT: setrawcolor( GREEN ); break;
        case WARNING: setrawcolor( RED ); break;
        case ERROR: setrawcolor( YELLOW ); break;
        case DISABLED: setrawcolor( GREY ); break;
        case DIRECTORY: setrawcolor( RED ); break;
        
    }
}

void printStringUsingGlutVectorFont(const char *string, int x, int y, float size, color c )
{
    setcolor( c );
   // glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(x, screenh-y, 0);
    glScalef(size, size, size);
    //glLineWidth( 2.0f);
    glutStrokeString(GLUT_STROKE_MONO_ROMAN, (const unsigned char*)string);
    glPopMatrix();
}

void print(const char *s, int x, int y ,int size, color c )
{
    if(s) printStringUsingGlutVectorFont(s,x,y, (float)size / glutStrokeHeight(GLUT_STROKE_MONO_ROMAN), c );
}

void println(const char *s, int x, int y, int size, color c )
{
    print( s, x, y, size, c );
    print_y = y;
    print_x = x;
    print_s = size;
}

void println( int x, int y, int size, color c )
{
    print_x = x;
    print_s = size;
    print_y = y - ( print_s + print_s/16 );
}

void println(const char *s, color c )
{
    print_y += print_s + print_s/16;
    print(s, print_x, print_y , print_s, c);
}


void draw_statusbar( const char *s )
{
    char *istat = (char*)"unknow";
    char *mode = (char*)"unknow";
    char *state = (char*)"unknow";
    
    switch( emcStatus->task.interpState )
    {
        case EMC_TASK_INTERP_IDLE: istat = (char *)"idle"; break;
        case EMC_TASK_INTERP_PAUSED: istat = (char *)"paused"; break;
        case EMC_TASK_INTERP_READING: istat = (char *)"reading"; break;
        case EMC_TASK_INTERP_WAITING: istat = (char *)"waiting"; break;        
    } 
    
    switch( emcStatus->task.mode )
    {
        case EMC_TASK_MODE_MANUAL: mode = (char *)"manual"; break;
        case EMC_TASK_MODE_AUTO: mode = (char *)"auto"; break;
        case EMC_TASK_MODE_MDI: mode = (char *)"mdi"; break;
    }
    
    switch( emcStatus->task.state )
    {
        case EMC_TASK_STATE_ESTOP: state = (char *)"Estop"; break;
        case EMC_TASK_STATE_ESTOP_RESET: state = (char *)"Estop reset"; break;
        case EMC_TASK_STATE_OFF: state = (char *)"Off"; break;
        case EMC_TASK_STATE_ON: state = (char *)"On"; break;
    }    
    
    sprintf( strbuf, " %s mode:%s intr:%s state:%s" , s, mode, istat, state );
    print( strbuf ,0,19,15);
    
    glBegin(GL_LINES);
        glVertex2i(0,screenh-24);
        glVertex2i(800,screenh-24);
    glEnd();
    
    if( show_last_msg ) print( errors.back().c_str(), 0, 570, 15 ); 
    
}

void drawCross(GLfloat x, GLfloat y, GLfloat size){
    glBegin(GL_LINES);
        glVertex2f( x + size, y );
        glVertex2f( x - size, y );
        glVertex2f( x, y + size );
        glVertex2f( x, y - size );
    glEnd();
}

void drawCircle(GLfloat x, GLfloat y, GLfloat radius){
    int i;
    int lines = 16;
    GLfloat twicePi = 2.0f * M_PI;
    glBegin(GL_LINE_LOOP);
        for(i = 0; i <= lines;i++) {
            glVertex2f(
                x + (radius * cos(i *  twicePi / lines)),
                y + (radius* sin(i * twicePi / lines))
            );
        }
    glEnd();

    drawCross( x, y, radius);

}

color axiscolor( int n )
{

    if( emcStatus->motion.axis[ n ].minSoftLimit ||
        emcStatus->motion.axis[ n ].maxSoftLimit ||
        emcStatus->motion.axis[ n ].minHardLimit ||
        emcStatus->motion.axis[ n ].maxHardLimit )
    {
        return flasher ? WARNING:DISABLED;
    }   
    
    if( ! emcStatus->motion.axis[ n ].enabled ) return DISABLED;
    if( emcStatus->motion.axis[ n ].homed ) return TEXT;
    
    if( emcStatus->motion.axis[ n ].homing ){
        return flasher ? WARNING:DISABLED;
    }
    
    return WARNING;
}


void draw_dro( vec2 *cpos )
{
            
    char label[10];
    if( emcStatus->task.g5x_index < 7 )
        sprintf( label, "G5%d", emcStatus->task.g5x_index + 3 );
    else
        sprintf( label, "G59.%d", emcStatus->task.g5x_index - 6 );
    
    double x = emcStatus->motion.traj.actualPosition.tran.x;
    double z = emcStatus->motion.traj.actualPosition.tran.z;
    double c = 0;
    
    double cx = 0; 
    double cz = 0;
    double cc = 0;
    
    if( cpos != NULL )
    {
        cx = cpos->x;
        cz = cpos->z;
    }
    
    x -= emcStatus->task.g5x_offset.tran.x;
    z -= emcStatus->task.g5x_offset.tran.z;
    x -= emcStatus->task.g92_offset.tran.x;
    z -= emcStatus->task.g92_offset.tran.z;
    x -= emcStatus->task.toolOffset.tran.x;
    z -= emcStatus->task.toolOffset.tran.z;

    cx -= emcStatus->task.g5x_offset.tran.x;
    cz -= emcStatus->task.g5x_offset.tran.z;
    cx -= emcStatus->task.g92_offset.tran.x;
    cz -= emcStatus->task.g92_offset.tran.z;
    cx -= emcStatus->task.toolOffset.tran.x;
    cz -= emcStatus->task.toolOffset.tran.z;        
/*    
    sprintf(strbuf,"X%4.3f", x     ); print( strbuf ,550,55+40*0,40, axiscolor( AXISX ));  
    sprintf(strbuf,"D%4.3f", x*2.0f); print( strbuf ,550,55+40*1,40, axiscolor( AXISX ));  
    sprintf(strbuf,"Z%4.3f", z     ); print( strbuf ,550,55+40*2,40, axiscolor( AXISZ ));  
    sprintf(strbuf,"C%4.3f", c     ); print( strbuf ,550,55+40*3,40, axiscolor( AXISC ));  
*/
    if( cpos != NULL )
    {
        sprintf(strbuf,"X%4.2f %4.2f", x, cx           ); println( strbuf, 400, 55, 40, axiscolor( AXISX ));  
        sprintf(strbuf,"D%4.2f %4.2f", x*2.0f, cx*2.0f ); println( strbuf, axiscolor( AXISX ));  
        sprintf(strbuf,"Z%4.2f %4.2f", z, cz           ); println( strbuf, axiscolor( AXISZ ));  
        sprintf(strbuf,"C%4.2f %4.2f", c, cc           ); println( strbuf, axiscolor( AXISC ));
    }
    else
    {
        sprintf(strbuf,"X%4.2f", x      ); println( strbuf, 550, 55, 40, axiscolor( AXISX ));  
        sprintf(strbuf,"D%4.2f", x*2.0f ); println( strbuf, axiscolor( AXISX ));  
        sprintf(strbuf,"Z%4.2f", z      ); println( strbuf, axiscolor( AXISZ ));  
        sprintf(strbuf,"C%4.2f", c     ); println( strbuf, axiscolor( AXISC ));
    }   


    sprintf(strbuf,"%s X %4.3f",label, emcStatus->task.g5x_offset.tran.x );                          print( strbuf ,550,200+20*0,20 );
    sprintf(strbuf,"%s Z %4.3f",label, emcStatus->task.g5x_offset.tran.z );                          print( strbuf ,550,200+20*1,20 );
    sprintf(strbuf,"G92 X %4.3f",emcStatus->task.g92_offset.tran.x );                                print( strbuf ,550,200+20*2,20 );
    sprintf(strbuf,"G92 Z %4.3f",emcStatus->task.g92_offset.tran.z );                                print( strbuf ,550,200+20*3,20 );
    sprintf(strbuf,"Tool %d",   emcStatus->io.tool.toolInSpindle );                                  print( strbuf ,550,200+20*4,20 );
    sprintf(strbuf,"ToolX %4.3f",emcStatus->task.toolOffset.tran.x );                                print( strbuf ,550,200+20*5,20 );
    sprintf(strbuf,"ToolZ %4.3f",emcStatus->task.toolOffset.tran.z );                                print( strbuf ,550,200+20*6,20 );
    sprintf(strbuf,"ToolD %4.3f",_tools[ emcStatus->io.tool.toolInSpindle ].diameter );              print( strbuf ,550,200+20*7,20 );
    sprintf(strbuf,"Vel.%4.1f %d%%",emcStatus->motion.traj.current_vel*60.0f, status.feedoverride ); print( strbuf ,550,200+20*8,20 );

}


void draw_tool( int i )
{    
    float diameter;
    float frontangle;
    float backangle;
    int orientation;
    
    if( i )
    {
        diameter = _tools[ i ].diameter;
        frontangle = _tools[ i ].frontangle;
        backangle  = _tools[ i ].backangle;
        orientation =  _tools[ i ].orientation; 
    } 
    else
    {
        diameter = 0.1f;
        frontangle = 80;
        backangle  = 10;
        orientation = 6; 
    }
    
    float lathe_shapes[10][2] = {
        {0,0},                           
        {1,-1}, {1,1}, {-1,1}, {-1,-1}, 
        {0,-1}, {1,0}, {0,1}, {-1,0},   
        {0,0}                           
    };
    
    float radius = diameter / 2.0f;
    setrawcolor( YELLOW );
    glBegin(GL_LINES);
        glVertex2f(-radius/2.0,0.0);
        glVertex2f(radius/2.0,0.0);
        glVertex2f(0.0,-radius/2.0);
        glVertex2f(0.0,radius/2.0);
    glEnd();
    
    float dx = lathe_shapes[orientation][0];
    float dy = lathe_shapes[orientation][1];

    float min_angle = fmin(backangle, frontangle) * M_PI / 180.0f;
    float max_angle = fmax(backangle, frontangle) * M_PI / 180.0f;
    
    float sinmax = sin(max_angle);
    float cosmax = cos(max_angle);
    //float tanmax = cos(max_angle);
    float sinmin = sin(min_angle);
    float cosmin = cos(min_angle);
    //float tanmin = cos(min_angle);
    
    float circleminangle = - M_PI /2.0f + min_angle;
    float circlemaxangle = - 3.0f * M_PI/2.0f + max_angle;
  //  float d0 = 0;
    float w = 10;
   // float x1 = (w - d0);
    float sz = fmax( w, 3.0f*radius);
        
    glBegin(GL_TRIANGLE_FAN);
        
    glColor3f( 1.0f * 0.2f , 0.8f * 0.2f , 0.1f * 0.2f );
    glVertex2f( radius * dy ,-(radius *dx));  
    glColor3f( 1.0f , 0.8f , 0.1f );
    
    if( orientation == 9 )
    {
        for(int i = 0;i < 37;i++ )
        {
            float t = (float)i * M_PI / 18.0f;
            glVertex2f(radius * cos(t),  radius * sin(t));
        }
    }
    else
    {
    
        glVertex2f( radius * dy + radius * cos(circleminangle) + sz * cosmin ,
                  -(radius * dx + radius * sin(circleminangle) + sz * sinmin ));  
        for(int i = 0;i < 37;i++ )//37
        {
            float t = circleminangle + (float)i * (circlemaxangle - circleminangle)/36.0f;
            glVertex2f( radius*dy + radius * cos(t),-(radius*dx + radius * sin(t)));
            
        }
        glVertex2f( radius * dy + radius * cos(circlemaxangle) + sz * cosmax,
                 -( radius * dx + radius * sin(circlemaxangle) + sz * sinmax ) );
        glVertex2f( radius * dy + radius * cos(circleminangle) + sz * cosmin ,
                  -(radius * dx + radius * sin(circleminangle) + sz * sinmin ));      
    }  
    glEnd();   
            
}


void updatescreen(void)
{
    float c[4];
    setcolor( BACKROUND );
    glGetFloatv(GL_CURRENT_COLOR, c);

    glClearColor ( c[0], c[1], c[2], 1.0f );
    glClear      ( GL_COLOR_BUFFER_BIT );
    
    switch( status.screenmode )
    {
        case SCREENHOME:
            home_draw();
        break;
        
        case SCREENSET:
            set_draw();
        break;
        
        case SCREENMANUAL:
            manual_draw();
        break;
        
        case SCREENMDI:
            mdi_draw();
        break;
        
        case SCREENWIZARDS:
            wizards_draw();
        break;
        
        case SCREENEDIT:
            edit_draw();
        break;
        
        case SCREENFILE:
            file_draw();
        break;
        
        case SCREENAUTO:
            auto_draw();
        break;
        
    }
    glutSwapBuffers();
}


void exFunc ()
{
  fprintf ( stderr, "Exiting.\n" );
  exit ( 0 );
}

void keyboardfunc(unsigned char key, int x, int y)
{
   // printf("char %c value %d\n", key, key );
    switch( key )
    {
        case 13:
          ser_emul.push_back( "RET" );  
        break;
        
        case 127:
          ser_emul.push_back( "DEL" );  
        break;  
              
        default:;
        sprintf(strbuf,"CH=%c", key );
        ser_emul.push_back( strbuf );  
        
    }
}

void specialfunc(int key, int x, int y)
{
   // printf("specialkey %d\n", key );
    switch( key )
    {
        case 101:
          ser_emul.push_back( "UP" );  
        break;        
        case 103:
          ser_emul.push_back( "DOWN" );  
        break;
        case 100:
          ser_emul.push_back( "LEFT" );  
        break;        
        case 102:
          ser_emul.push_back( "RIGHT" );  
        break;
        case 108:
          ser_emul.push_back( "AX=0" );  
        break;        
        case 106:
          ser_emul.push_back( "AX=2" );  
        break;          
        case 104:
          ser_emul.push_back( "AX=5" );  
        break;    
        case 105:
          ser_emul.push_back( "START" );  
        break; 
        case 107:
          ser_emul.push_back( "HOLD" );  
        break;        
              
    } 
    
    if( key > 0 && key < 9 )
    {
        sprintf(strbuf,"MO=%d", 9-key );
        ser_emul.push_back( strbuf );   
    }
    
    if( key > 8 && key < 13 )
    {
        sprintf(strbuf,"IC=%d", key-8 );
        ser_emul.push_back( strbuf );   
    }    
    
}

void mousefunc(int button, int state, int x, int y)
{
   if ((button == 3) || (button == 4)) // It's a wheel event
   {
       if (state == GLUT_UP) return; // Disregard redundant GLUT_UP events
       if( button == 3 )
       {
            ser_emul.push_back( "JG+" );
       }
       if( button == 4 )
       {
            ser_emul.push_back( "JG-" );
       }  
   }
}

void init_opengl( int argc, char **argv )
{
    glutInit(&argc, argv);

    
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(800,600);
    glutCreateWindow("SAMICNC 0.0");
    //glutFullScreen();
    //glutPositionWindow(0,0);
    glutDisplayFunc( updatescreen );
    glutKeyboardFunc( keyboardfunc );
    glutSpecialFunc( specialfunc );
    glutMouseFunc( mousefunc );
    
    screenw = glutGet(GLUT_WINDOW_WIDTH);
    screenh = glutGet(GLUT_WINDOW_HEIGHT);
    
    glEnable       ( GL_ALPHA_TEST );
    glEnable       ( GL_BLEND );
    glAlphaFunc    ( GL_GREATER, 0.1f );
    glBlendFunc    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glMatrixMode   ( GL_MODELVIEW );
    glLoadIdentity ();
    glOrtho        ( 0, screenw, 0, screenh, -1, 1 );
}


