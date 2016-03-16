/********************************************************************
* Lathe user interface
* Author: Sami Helin
* License: GPL Version 2
* Copyright (c) 2016 All rights reserved.
********************************************************************/

#include "latheintf.h"

#define BAUDRATE B57600
#define DEVICE "/dev/ttyUSB0"
#define DEVICE2 "/dev/ttyUSB1"
int serport;

int _task = 0; // control preview behaviour when remapping

char buf[BUFFSIZE] = {0};
char strbuf[BUFFSIZE];
char programPrefix[LINELEN+1] = "";
list<string> errors;
bool show_last_msg = false; 

extern list<string> ser_emul;

int screenw, screenh;
int timer = 0;
bool flasher;
    
char ttfile[LINELEN+1];
char *ttcomments[CANON_POCKETS_MAX];


int  emcOperatorError(int id, const char *fmt, ...)
{
    va_list ap;
    char error[BUFFSIZE];
    if (id)
        fprintf(stderr,"[%d] ", id);

    va_start(ap, fmt);
    vsprintf(error, fmt, ap);
    printf("ERROR:%s\n",error);
    
    va_end(ap);
    return 0;
}


/*
 * 
emcStatus->motion.axis[t].homed


class EMC_AXIS_STAT:public EMC_AXIS_STAT_MSG {
  public:
    EMC_AXIS_STAT();

    // For internal NML/CMS use only.
    void update(CMS * cms);

    // configuration parameters
    unsigned char axisType;	// EMC_AXIS_LINEAR, EMC_AXIS_ANGULAR
    double units;		// units per mm, deg for linear, angular
    double backlash;
    double minPositionLimit;
    double maxPositionLimit;
    double maxFerror;
    double minFerror;

    // dynamic status
    double ferrorCurrent;	// current following error
    double ferrorHighMark;	// magnitude of max following error
    
    double output;		// commanded output position
    double input;		// current input position
    double velocity;		// current velocity
    unsigned char inpos;	// non-zero means in position
    unsigned char homing;	// non-zero means homing
    unsigned char homed;	// non-zero means has been homed
    unsigned char fault;	// non-zero means axis amp fault
    unsigned char enabled;	// non-zero means enabled
    unsigned char minSoftLimit;	// non-zero means min soft limit exceeded
    unsigned char maxSoftLimit;	// non-zero means max soft limit exceeded
    unsigned char minHardLimit;	// non-zero means min hard limit exceeded
    unsigned char maxHardLimit;	// non-zero means max hard limit exceeded
    unsigned char overrideLimits; // non-zero means limits are overridden
};

class EMC_TRAJ_STAT:public EMC_TRAJ_STAT_MSG {
  public:
    EMC_TRAJ_STAT();

    // For internal NML/CMS use only.
    void update(CMS * cms);

    double linearUnits;		// units per mm
    double angularUnits;	// units per degree
    double cycleTime;		// cycle time, in seconds
    int axes;			// maximum axis number
    int axis_mask;		// mask of axes actually present
    enum EMC_TRAJ_MODE_ENUM mode;	// EMC_TRAJ_MODE_FREE,
    // EMC_TRAJ_MODE_COORD
    bool enabled;		// non-zero means enabled

    bool inpos;			// non-zero means in position
    int queue;			// number of pending motions, counting
    // current
    int activeQueue;		// number of motions blending
    bool queueFull;		// non-zero means can't accept another motion
    int id;			// id of the currently executing motion
    bool paused;			// non-zero means motion paused
    double scale;		// velocity scale factor
    double rapid_scale;		// rapid scale factor
    double spindle_scale;	// spindle velocity scale factor

    EmcPose position;		// current commanded position
    EmcPose actualPosition;	// current actual position, from forward kins
    double velocity;		// system velocity, for subsequent motions
    double acceleration;	// system acceleration, for subsequent
    // motions
    double maxVelocity;		// max system velocity
    double maxAcceleration;	// system acceleration

    EmcPose probedPosition;	// last position where probe was tripped.
    bool probe_tripped;		// Has the probe been tripped since the last
    // clear.
    bool probing;		// Are we currently looking for a probe
    // signal.
    int probeval;		// Current value of probe input.
    int kinematics_type;	// identity=1,serial=2,parallel=3,custom=4
    int motion_type;
    double distance_to_go;         // in current move
    EmcPose dtg;
    double current_vel;         // in current move
    bool feed_override_enabled;
    bool spindle_override_enabled;
    bool adaptive_feed_enabled;
    bool feed_hold_enabled;
};

class EMC_STAT:public EMC_STAT_MSG {
  public:
    EMC_STAT();

    // For internal NML/CMS use only.
    void update(CMS * cms);

    // the top-level EMC_TASK status class
    EMC_TASK_STAT task;

    // subordinate status classes
    EMC_MOTION_STAT motion;
    EMC_IO_STAT io;

    int debug;			// copy of EMC_DEBUG global
};
class EMC_TASK_STAT:public EMC_TASK_STAT_MSG {
  public:
    EMC_TASK_STAT();

    // For internal NML/CMS use only.
    void update(CMS * cms);

    enum EMC_TASK_MODE_ENUM mode;	// EMC_TASK_MODE_MANUAL, etc.
    enum EMC_TASK_STATE_ENUM state;	// EMC_TASK_STATE_ESTOP, etc.

    enum EMC_TASK_EXEC_ENUM execState;	// EMC_DONE,WAITING_FOR_MOTION, etc.
    enum EMC_TASK_INTERP_ENUM interpState;	// EMC_IDLE,READING,PAUSED,WAITING
    int motionLine;		// line motion is executing-- may lag
    int currentLine;		// line currently executing
    int readLine;		// line interpreter has read to
    bool optional_stop_state;	// state of optional stop (== ON means we stop on M1)
    bool block_delete_state;	// state of block delete (== ON means we ignore lines starting with "/")
    bool input_timeout;		// has a timeout happened on digital input
    char file[LINELEN];
    char command[LINELEN];
    EmcPose g5x_offset;		// in user units, currently active
    int g5x_index;              // index of active g5x system
    EmcPose g92_offset;		// in user units, currently active
    double rotation_xy;
    EmcPose toolOffset;		// tool offset, in general pose form
    int activeGCodes[ACTIVE_G_CODES];
    int activeMCodes[ACTIVE_M_CODES];
    double activeSettings[ACTIVE_SETTINGS];
    CANON_UNITS programUnits;	// CANON_UNITS_INCHES,MM,CM

    int interpreter_errcode;	// return value from rs274ngc function 
    // (only useful for new interpreter.)
    int task_paused;		// non-zero means task is paused
    double delayLeft;           // delay time left of G4, M66..
    int queuedMDIcommands;      // current length of MDI input queue
};
// types for EMC_TASK interpState
enum EMC_TASK_INTERP_ENUM {
    EMC_TASK_INTERP_IDLE = 1,
    EMC_TASK_INTERP_READING = 2,
    EMC_TASK_INTERP_PAUSED = 3,
    EMC_TASK_INTERP_WAITING = 4
};
* 
* struct CANON_TOOL_TABLE {
    int toolno;
    EmcPose offset;
    double diameter;
    double frontangle;
    double backangle;
    int orientation;
};
* 
*/

struct machinestatus status;

extern CANON_TOOL_TABLE _tools[CANON_POCKETS_MAX];

int serialport_init()
{
    int fd;
    struct termios toptions;

    fd = open(DEVICE, O_RDWR | O_NOCTTY );
    if (fd == -1)
    {
        fd = open(DEVICE2, O_RDWR | O_NOCTTY );
        if (fd == -1)
        {
            perror("init_serialport: Unable to open port ");
            return -1;
        }  
    }

    if (tcgetattr(fd, &toptions) < 0)
    {
        perror("init_serialport: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = BAUDRATE;
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);
    /* 8 bits, no parity, no stop bits */

    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    
    toptions.c_iflag |= IGNCR;
    
    fcntl(fd, F_SETFL, FNDELAY);
    /* Canonical mode */
    toptions.c_lflag |= ICANON;

    if( tcsetattr(fd, TCSANOW, &toptions) < 0)
    {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }
    return fd;
}

void initstatus()
{
    status.screenmode = SCREENMANUAL;
    status.mode = EMC_TASK_MODE_MANUAL;
    status.axis = AXISX;
    status.jogfeedrate = 50;
    status.incr = 1.0;
    status.feedoverride = 100;
    status.singleblock = false;
    status.skipblock = false;
    status.optionalstop = false;
    status.slowrapid = false;
}

const char* isprefix( const char*  prefix, int* n)
{
    char *start = strstr(buf, prefix);
    if (start == buf)
    {
        const char *vstr = strlen( prefix ) + start;
        if(n != NULL){
            *n = atoi( vstr );
        }
        return vstr;
    }
    return NULL;
}


void calcval( int* v, int n, int min, int max ,int lower,int upper)
{
    if( n >= min && n <= max )
    {
        *v =  (float)(n-min) / (float)( min - max ) * (float)( lower - upper)  + lower;
    }
}

bool readserial()
{
    int n = 0;
    
    if( serport == -1 && ser_emul.size() > 0 )
    {
        strcpy( buf, ser_emul.back().c_str() );
        n = strlen( buf ); 
        ser_emul.pop_back();
    }
    else if( serport != -1 )
    {
        n = read(serport, buf, BUFFSIZE);
    }
    
    if( n > 0 )
    {
        buf[n] = 0;
        printf("%i bytes read, buffer contains: %s\n", n, buf);
        return true;
    }
    return false;
}


void  parse_serialdata()
{
    int n;
    int old_screenmode = status.screenmode; 
    
    parseagain: // handle potentiometers & increment here:
    
    if( isprefix( "MO=" ,&n ) )
    {
        status.screenmode = n;
        if( readserial() )
        {
            goto parseagain;
        } 
    }
    
    if( isprefix( "P2=" ,&n ) )
    {
        
        n =  (int)((float)n / (254.0f/200.0f));
        
        if(n > 200) n=200;
        if(n < 0) n=0;
        
        status.feedoverride_new = n;

        if( readserial() )
        {
            goto parseagain;
        } 
        
    }
    
    if( isprefix( "P1=" ,&n ) )
    {
        int v = 0;
        calcval( &v, n, 0  , 166, 0,  20);
        calcval( &v, n, 166, 500, 20, 50);
        calcval( &v, n, 500, 613, 50, 70);
        calcval( &v, n, 613, 731, 70, 100);
        calcval( &v, n, 731, 967, 100,500);
        calcval( &v, n, 967, 1022,500,2000);
        
        status.jogfeedrate = v;
        if( readserial() )
        {
            goto parseagain;
        } 
    }
    
    if( isprefix( "IC=" ,&n ) )
    {
        if (n == 1)      status.incr = 0.001f;
        else if (n == 2) status.incr = 0.01f;
        else if (n == 3) status.incr = 0.1f;
        else if (n == 4) status.incr = 1.0f;
    }
    else if( isprefix( "AX=" ,&n ) )
    {
        status.axis = n;
    }
    
    
    if( status.screenmode != old_screenmode )
    {
        
        switch(status.screenmode)
        {
            case SCREENMANUAL:
                manual_init();
            break;
                
            case SCREENHOME:
                manual_init();
            break;
            
            case SCREENSET:
            set_init();
            break;

            case SCREENAUTO:
                auto_init();
            break;

            case SCREENMDI:
                sendMdi();
            break;
            
            case SCREENFILE:
                file_init();
            break;   
            
            case SCREENWIZARDS:
                wizards_init();
            break;  
            
        }
        
        return;
    }
    
    switch( status.screenmode )
    {
        
        case SCREENMANUAL:
            manual_parse_serialdata();
        break;
            
        case SCREENHOME:
             home_parse_serialdata();
        break;
        
        case SCREENSET:
            set_parse_serialdata();
        break;

        case SCREENAUTO:
            auto_parse_serialdata();
        break;

        case SCREENMDI:
           mdi_parse_serialdata();
        break;
        
        case SCREENFILE:
            file_parse_serialdata();
        break;
        
        case SCREENEDIT:
            edit_parse_serialdata();
        break;
        
        case SCREENWIZARDS:
            wizards_parse_serialdata();
        break;  
              
    }
    
    // clean error message 
    if( isprefix( "START" ,NULL ) )
    {
        show_last_msg = false;
    }
    
}








void loadinifile( char *f )
{
    
    IniFile inifile;
    const char *inistring;
    printf("open inifile %s\n", f);
    if (inifile.Open( f )) {
        
        if (NULL != (inistring = inifile.Find("PROGRAM_PREFIX", "DISPLAY"))) {
            if (1 != sscanf(inistring, "%s", programPrefix)) {
                programPrefix[0] = 0;
            }
        }
        else if (NULL != (inistring = inifile.Find("PROGRAM_PREFIX", "TASK")))
        {
           if (1 != sscanf(inistring, "%s", programPrefix)) {
              programPrefix[0] = 0;
           }
        }
        else
        {
            programPrefix[0] = 0;
        }
        
        if (NULL != (inistring = inifile.Find("TOOL_TABLE","EMCIO"))) {
            if (1 != sscanf(inistring, "%s", ttfile)) {
                ttfile[0] = 0;
            }
        }
       
        printf("PROGRAM_PREFIX = %s\n", programPrefix );
        printf("TOOL_TABLE = %s\n", ttfile );
        
        inifile.Close();
        iniLoad( f );
    
    }

}

static void initemc()
{
    emcWaitType = EMC_WAIT_RECEIVED;
    emcCommandSerialNumber = 0;
    emcTimeout = 0.0;
    emcUpdateType = EMC_UPDATE_AUTO;
    linearUnitConversion = LINEAR_UNITS_MM;
    angularUnitConversion = ANGULAR_UNITS_AUTO;
    emcCommandBuffer = 0;
    emcStatusBuffer = 0;
    emcStatus = 0;

    emcErrorBuffer = 0;
    error_string[LINELEN-1] = 0;
    operator_text_string[LINELEN-1] = 0;
    operator_display_string[LINELEN-1] = 0;
    programStartLine = 0;
    
    print("starting",100,100,30);
    glutPostRedisplay();
    usleep(500);
    
    int i=0;
    while( tryNml(2,1) != 0)
    {
       if( i++ > 5 )
       {
           printf("can't connect to emc\n");
           exit(1);
       }
       printf("tryNml...\n");
    }

    for(int i=0; i<CANON_POCKETS_MAX; i++) ttcomments[i] = (char*)malloc(BUFFSIZE);
    
    print("Load tooltable",100,100+30*1,30);
    glutPostRedisplay();glutSwapBuffers();
    usleep(500);
    
    print("send EstopReset",100,100+30*2,30);
    glutPostRedisplay();glutSwapBuffers();
    usleep(500);
    
    sendEstopReset();
    
    print("send MachineOn",100,100+30*3,30);
    glutPostRedisplay();glutSwapBuffers();
    usleep(500);
    
    sendMachineOn();
    print("init serialport",100,100+30*4,30);usleep(500);
    serport = serialport_init();
    
}



int main ( int argc, char **argv )
{
    
    for (int i=1; i < argc; i++) {
         printf("arg%d=%s\n", i, argv[i]);
    }
    
    initstatus();
    init_opengl(argc, argv);
    initemc();
    loadinifile( argv[2] );
    sendLoadToolTable(ttfile);
    loadToolTable(ttfile, _tools, 0, ttcomments, 0);
    
    
    while ( 1 )
    {
        
        if( readserial() ) parse_serialdata();
        
        updateStatus();
        
        if( emcStatus->task.state == EMC_TASK_STATE_ESTOP_RESET )
        {
            show_last_msg = false;
            sendMachineOn();
            emcCommandWaitDone();
            updateStatus();
        }

        operator_display_string[0] = 0;
        operator_text_string[0] = 0;
        error_string[0] = 0;
        if( updateError() == 0 )
        {
            bool m = false;
            time_t secs = time(0);
            struct tm *local = localtime(&secs);
            if( operator_display_string[0] ) 
            {
                sprintf( strbuf, "%02d:%02d:%02d d:%s", local->tm_hour, local->tm_min, local->tm_sec, operator_display_string);
                m = true;
            }
            if( operator_text_string[0] )
            {
                sprintf( strbuf, "%02d:%02d:%02d t:%s", local->tm_hour, local->tm_min, local->tm_sec, operator_text_string);
                m = true;
            }
            if( error_string[0] )
            {
                sprintf( strbuf, "%02d:%02d:%02d ERROR:%s", local->tm_hour, local->tm_min, local->tm_sec, error_string);
                errors.push_back( strbuf );
                if( errors.size() > 20 ) errors.pop_front();
                m = true;
            }
            if(m)
            {
                show_last_msg = true;
                printf("%s\n",strbuf);
            }
        }
        
        glutMainLoopEvent();
        glutPostRedisplay();
        
        usleep(1000000 / REFRESHRATE);
        
        if( timer++ > REFRESHRATE/4 ) 
        {
            timer = 0,
            flasher = ! flasher;
        }
    }

    close(serport);
    return 0;
}

