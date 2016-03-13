
#include <util/atomic.h>

volatile long rotaryhalfsteps = 0;
long rotarypos_old = 0;
long rotarypos = 0;
long rotarypos_seek = 0;

#define POT1PIN  A0
#define POT2PIN  A1
unsigned int pot1 = 0;
unsigned int pot2 = 0;
unsigned int pot1_sum = 0;
unsigned int pot2_sum = 0;
unsigned int pot1_old = 0;
unsigned int pot2_old = 0;
unsigned char pot_counter = 0;
#define POT_READS 8

#define MODEPIN1 4
unsigned char mode = 1;
unsigned char oldmode = 0;

#define INCRPIN1 22
unsigned char incr = 1;
unsigned char oldincr = 0;

#define MATXPIN 26
#define MATXSIZE 11
#define MATYPIN 27
#define MATYSIZE 5

#define ALTPIN   37

#define UPPIN 39
#define XPIN 16
#define ZPIN 15
#define CPIN 14
#define STARTPIN 12
#define HOLDPIN 13
#define SINGLEPIN 48
#define SKIPPIN 49
#define OSTOPPIN 50
#define RAPIDIN 51

#define XAXIS 0
#define ZAXIS 2
#define CAXIS 5

struct button
{
  byte pin;
  char *text;
  bool bsend;
  bool pressed;
};

#define BUTTONS 10

struct button buttons[BUTTONS] =
{
  { XPIN, "AX=0", false, false },
  { ZPIN, "AX=2", false, false },
  { CPIN, "AX=5", false, false },
  { HOLDPIN, "HOLD", false, false },
  { STARTPIN, "START", false, false },
  { SINGLEPIN, "SGLBLK", false, false },
  { SKIPPIN, "BLKSKP", false, false },
  { OSTOPPIN, "OPSTP", false, false },
  { RAPIDIN, "RPDO", false, false },
  { UPPIN, "UP", false, false }
};


/*
  0 (null, NUL, \0, ^@), originally intended to be an ignored character, but now used by many programming languages to mark the end of a string.
  7 (bell, BEL, \a, ^G), which may cause the device receiving it to emit a warning of some kind (usually audible).
  8 (backspace, BS, \b, ^H), used either to erase the last character printed or to overprint it.
  9 (horizontal tab, HT, \t, ^I), moves the printing position some spaces to the right.
  10 (line feed, LF, \n, ^J), used as the end of line marker in most UNIX systems and variants.
  11 (vertical tab, VT, \v, ^K), vertical tabulation.
  12 (form feed, FF, \f, ^L), to cause a printer to eject paper to the top of the next page, or a video terminal to clear the screen.
  13 (carriage return, CR, \r, ^M), used as the end of line marker in Mac OS, OS-9, FLEX (and variants).
  A carriage return/line feed pair is used by CP/M-80 and its derivatives including DOS and Windows, and by Application Layer protocols such as HTTP.
  27 (escape, ESC, \e (GCC only), ^[).
  127 (delete, DEL, ^?), originally intended to be an ignored character,
  but now used in some systems to erase a character. Also used by some Plan9 console programs to send an interrupt note to the current process.
*/


#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define BACK 5
#define DEL 6

char matchar[MATYSIZE][MATXSIZE] = {
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', DEL,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',  LEFT , RIGHT,
  'z', 'x', 'c', 'v', 'b', 'n', 'm',  0 ,  0 ,  0 ,  0 ,
  '1', '2', '3', '4', '5', '-', ' ',  DOWN ,  0 ,  0 ,  0 ,
  '6', '7', '8', '9', '0', '.',  0 , '\n',  0 ,  0 ,  0
};

char matchar2[MATYSIZE][MATXSIZE] = {
  '<', '>', '[', ']', '{', '}', '(', ')', '"', 0, BACK,
  '#', '%', '@', ';', ':', '_', 'ö', 'ä', 'Å',  0 , 0,
  '/', '*', '&', '|', '!', '^', '=',  0 ,  0 ,  0 ,  0 ,
  0  ,  0,   0,   0,   0,  '+', ' ',  0 ,  0 ,  0 ,  0 ,
  0  ,  0,   0,   0,   0,  ',',  0 , 0,  0 ,  0 ,  0
};

char key = 0;
char trykey = 0;
char oldkey = 0;
int keycount = 0;

void jog_int0();
void jog_reset();

void setup()
{
  Serial.begin(57600);
  attachInterrupt(0, jog_int0, CHANGE);
  jog_reset();

  for ( int i = 0; i < BUTTONS; i++)
  {
    pinMode( buttons[i].pin, INPUT_PULLUP);
  }

  for ( int y = 0; y < MATYSIZE; y++)
  {
    pinMode(MATYPIN + y * 2, INPUT_PULLUP);
  }

  for ( int x = 0; x < MATXSIZE; x++)
  {
    pinMode(MATXPIN + x * 2, OUTPUT);
    digitalWrite(MATXPIN + x * 2, HIGH);
  }


  pinMode(POT1PIN, INPUT);
  pinMode(POT2PIN, INPUT);

  pinMode(MODEPIN1, INPUT_PULLUP);
  pinMode(MODEPIN1 + 1, INPUT_PULLUP);
  pinMode(MODEPIN1 + 2, INPUT_PULLUP);
  pinMode(MODEPIN1 + 3, INPUT_PULLUP);
  pinMode(MODEPIN1 + 4, INPUT_PULLUP);
  pinMode(MODEPIN1 + 5, INPUT_PULLUP);
  pinMode(MODEPIN1 + 6, INPUT_PULLUP);
  pinMode(MODEPIN1 + 7, INPUT_PULLUP);

  pinMode(INCRPIN1, INPUT_PULLUP);
  pinMode(INCRPIN1 + 1, INPUT_PULLUP);
  pinMode(INCRPIN1 + 2, INPUT_PULLUP);
  pinMode(INCRPIN1 + 3, INPUT_PULLUP);

  pinMode(ALTPIN, INPUT_PULLUP);

}

void readkeyboard()
{
  char c = 0;

  for ( int x = 0; x < MATXSIZE; x++)
  {
    digitalWrite(MATXPIN + x * 2, LOW);

    for ( int y = 0; y < MATYSIZE; y++)
    {
      if ( ! digitalRead(MATYPIN + y * 2 ) )
      {

        if ( ! digitalRead( ALTPIN ) )
        {
          c = matchar2[y][(MATXSIZE - 1) - x];
        }
        else
        {
          c = matchar[y][(MATXSIZE - 1) - x];
        }


      }
    }

    digitalWrite(MATXPIN + x * 2, HIGH);
  }

  if (c == trykey)
  {
    if ( ++keycount > 10)
    {
      keycount = 0;
      key = c;
    }
  }
  else
  {
    trykey = c;
    keycount = 0;
  }


}

void readincr()
{

  if ( ! digitalRead(INCRPIN1) ) {
    incr = 1;
    return;
  }
  if ( ! digitalRead(INCRPIN1 + 1 ) ) {
    incr = 2;
    return;
  }
  if ( ! digitalRead(INCRPIN1 + 2 ) ) {
    incr = 3;
    return;
  }
  if ( ! digitalRead(INCRPIN1 + 3 ) ) {
    incr = 4;
    return;
  }
  if ( ! digitalRead(INCRPIN1 + 4 ) ) {
    incr = 5;
  }

}

void readmode()
{
  if ( ! digitalRead(MODEPIN1) ) {
    mode = 1;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 1) ) {
    mode = 2;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 2) ) {
    mode = 3;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 3) ) {
    mode = 4;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 4) ) {
    mode = 5;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 5) ) {
    mode = 6;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 6) ) {
    mode = 7;
    return;
  }
  if ( ! digitalRead(MODEPIN1 + 7) ) {
    mode = 8;
  }

}

void readbuttons()
{

  for ( int i = 0; i < BUTTONS; i++)
  {
    buttons[i].pressed = ! digitalRead( buttons[i].pin );
  }

  /*
    start =   ! digitalRead( STARTPIN );
    hold =   ! digitalRead( HOLDPIN );
    x = ! digitalRead( XPIN );
    z = ! digitalRead( ZPIN );
    c = ! digitalRead( CPIN );
  */

}



void read_pots()
{
  pot1_sum += analogRead(POT1PIN);
  pot2_sum += analogRead(POT2PIN);

  if ( ++pot_counter >= POT_READS )
  {
    pot1 = pot1_sum / POT_READS;
    pot2 = pot2_sum / ( POT_READS * 4 );
    pot1_sum = 0;
    pot2_sum = 0;
    pot_counter = 0;
  }

}


void jog_int0()
{
  if ( digitalRead(2) == digitalRead(3) )
  {
    rotaryhalfsteps++;
  }
  else
  {
    rotaryhalfsteps--;
  }
}

void jog_reset()
{

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    rotarypos = 0;
    rotarypos_seek = 0;
    rotarypos_old = 2;
    rotaryhalfsteps = 0;
  }

}

void jog_read()
{

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    rotarypos = rotaryhalfsteps ;
  }
  rotarypos /= 2;
  if (abs(rotarypos - rotarypos_old) > 10 )
  {
    Serial.println("Rotary encoder error");
    rotarypos = rotarypos_old;
  }

}

void loop()
{
  readbuttons();
  read_pots();
  jog_read();
  readmode();
  readincr();

  readkeyboard();


  String string;
  if ( key != oldkey  )
  {
    oldkey = key;
    if ( key != 0)
    {
      if (key == '\n')
      {
        Serial.println("RET");
      }
      else if (key == LEFT)
      {
        Serial.println("LEFT");
      }
      else if (key == RIGHT)
      {
        Serial.println("RIGHT");
      }
      else if (key == DOWN)
      {
        Serial.println("DOWN");
      }
      else if (key == BACK)
      {
        Serial.println("BACK");
      }
      else if (key == DEL)
      {
        Serial.println("DEL");
      }
      else {
        string = String( key );
        Serial.print("CH=");
        Serial.println(string);
      }
    } else Serial.println("RLKB");
  }


  for ( int i = 0; i < BUTTONS; i++)
  {
    if ( buttons[i].pressed )
    {
      if ( ! buttons[i].bsend || mode != oldmode )
      {
        Serial.println( buttons[i].text );
        buttons[i].bsend = true;
      }
    }
    else
    {
      if ( buttons[i].bsend || mode != oldmode )
      {
        Serial.print( "RL" );
        Serial.println( buttons[i].text );
      }
      buttons[i].bsend = false;
    }
  }

  if (mode != oldmode )
  {
    Serial.print("MO=");
    Serial.println(mode);
    oldmode = mode;
    jog_reset();

    // send all pots & switches
    /*
      Serial.print("IC=");
      Serial.println(incr);

      Serial.print("P1=");
      Serial.println(pot1);

      Serial.print("P2=");
      Serial.println(pot2);*/

  }


  if (incr != oldincr )
  {
    Serial.print("IC=");
    Serial.println(incr);
    oldincr = incr;
    jog_reset();
  }

  if ( abs( pot1 - pot1_old) > 1 )
  {
    Serial.print("P1=");
    Serial.println(pot1);
    pot1_old = pot1;
  }

  if ( abs( pot2 - pot2_old) > 1 )
  {
    Serial.print("P2=");
    Serial.println(pot2);
    pot2_old = pot2;
  }


  if (rotarypos != rotarypos_old)
  {
    rotarypos_old = rotarypos;
    //Serial.print("RO="); Serial.println(rotarypos);
  }

  if ( rotarypos > rotarypos_seek )
  {
    rotarypos_seek++;
    Serial.println("JG+");
  }
  else if ( rotarypos < rotarypos_seek )
  {
    rotarypos_seek--;
    Serial.println("JG-");
  }

}


