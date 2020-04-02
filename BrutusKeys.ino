#include "arduino2.h"

// inputs
byte rows[] = {10, 9, 8, 7, 6, 5, 4, 3};
const int rowCount = 8;


//outputs
byte cols[] = {A0, A1, A2, A3, A4, A5, 12, 11};

byte notes[32] = {
 67, //0
73, //1
83,  //2 
0, 
66, //4 
74, //5
70,  //6
0,
65,   //8
73,  //9
81,   //10 
0,  
64, //12 
72, //13
80, //14
0,
63,  //16
71, //17
79,  //18
0,
62, //20
70,  //21
78, // 22
0,
61, //24 
69, //25
77,   //26
0,
60,//28
68,//29
76,//30
84 //31
};
  
const int colCount = 8;

#define KEYS_NUMBER 25
byte keys[colCount][rowCount];
byte keybuffer[32][2] = {1,1};

byte          keys_state[32];
unsigned long keys_time[32];


#define KEY_OFF               2
#define KEY_START             1
#define KEY_ON                0
#define KEY_RELEASED          3


#define MIN_TIME_MS   3
#define MAX_TIME_MS   50
#define MAX_TIME_MS_N (MAX_TIME_MS - MIN_TIME_MS)

//uncoment the next line to get text midi message at output
//#define DEBUG_MIDI_MESSAGE

void setup() {
  #ifdef DEBUG_MIDI_MESSAGE
    Serial.begin(115200);
  #else
    Serial.begin(31250);
  #endif

  
  int i;
  for (i = 0; i < KEYS_NUMBER; i++)
  {
    keys_state[i] = KEY_OFF;
    keys_time[i] = 0;
  }
  for (int x = 0; x < rowCount; x++)
  {
    Serial.print(rows[x]); Serial.println(" as input");
    pinMode(rows[x], INPUT);
  }

  for (int x = 0; x < colCount; x++)
  {
    Serial.print(cols[x]); Serial.println(" as input-pullup");
    pinMode(cols[x], INPUT_PULLUP);
  }
}

void send_midi_event(byte status_byte, byte key_index, unsigned long time)
{
  unsigned long t = time;
  
  if (t > MAX_TIME_MS)
    t = MAX_TIME_MS;
  if (t < MIN_TIME_MS)
    t = MIN_TIME_MS;
  t -= MIN_TIME_MS;
  unsigned long velocity = 127 - (t * 127 / MAX_TIME_MS_N);
  byte vel = (((velocity * velocity) >> 7) * velocity) >> 7;
  byte key = 24 + key_index;
#ifdef DEBUG_MIDI_MESSAGE
  char out[32];
  sprintf(out, "%02X %d %d %03d %d", status_byte, key_index,notes[key_index], vel, time);
  Serial.println(out);
#else
  Serial.write(status_byte);
  Serial.write(notes[key_index]-24);
  Serial.write(127);
#endif
}

void readMatrix() {
  
  // iterate the columns
  for (int colIndex = 0; colIndex < colCount; colIndex++)
  {
    // col: set to output to low
    byte curCol = cols[colIndex];
    pinMode(curCol, OUTPUT);
    digitalWrite2(curCol, LOW);

    // row: iterate through the rows
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
      byte rowCol = rows[rowIndex];
      pinMode(rowCol, INPUT_PULLUP);
      keys[colIndex][rowIndex] = digitalRead2(rowCol);
      pinMode(rowCol, INPUT);
    }
    // disable the column
    pinMode(curCol, INPUT);
    
  }
}

void fillBuffer() {
    int counter = 0;
  for (int rowIndex = 0; rowIndex < rowCount; rowIndex ++)
  {
    for (int colIndex = 0; colIndex < colCount; colIndex+=2)
    {
      keybuffer[counter][0] = keys[colIndex][rowIndex];
      keybuffer[counter][1] = keys[colIndex + 1][rowIndex];
      counter++;
    }
  }
  //Serial.println( keybuffer[0][1], keybuffer[0][2]);
  translateBuffer();
}

void translateBuffer(){
  
  for (int key = 0; key < 32; key++)
  {
      switch (keys_state[key])
      {
      case KEY_OFF:
          if (keybuffer[key][0] == 0 && keybuffer[key][1] == 1)
          {
              keys_state[key] = KEY_START;
              keys_time[key] = millis();
          }
          if (keybuffer[key][0] == 1 && keybuffer[key][1] == 0)
          {
              keys_state[key] = KEY_START;
              keys_time[key] = millis();
          }
          if (keybuffer[key][0] == 0 && keybuffer[key][1] == 0)
          {
              keys_state[key] = KEY_ON;
              keys_time[key] = millis();
          }
          break;
      case KEY_START:
          if (keybuffer[key][0] == 1 && keybuffer[key][1] == 1)
          {
             keys_state[key] = KEY_OFF;
              break;
          }
          if (keybuffer[key][0] == 0 && keybuffer[key][1] == 0)
          {
              keys_state[key]= KEY_ON;
              send_midi_event(0x90, key, millis() - keys_time[key]);
          }
          break;
      case KEY_ON:
          if (keybuffer[key][0] == 1 && keybuffer[key][1] == 0)
          {
              keys_state[key] = KEY_RELEASED;
              keys_time[key] = millis();
          }
          if (keybuffer[key][0] == 0 && keybuffer[key][1] == 1)
          {
              keys_state[key] = KEY_RELEASED;
              keys_time[key] = millis();
          }
          if (keybuffer[key][0] == 1 && keybuffer[key][1] == 1)
          {
              keys_state[key] = KEY_OFF;
               send_midi_event(0x80, key, millis() - keys_time[key]);
          }
          break;
      case KEY_RELEASED:
          if (keybuffer[key][0] == 1 && keybuffer[key][1] == 1)
          { 
              keys_state[key] = KEY_OFF;
              send_midi_event(0x80, key, millis() - keys_time[key]);
          }
          break;
     }
  }
}

void loop() {
  readMatrix();
  fillBuffer();
}
