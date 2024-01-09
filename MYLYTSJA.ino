//
// Modifies coordinates in NMEA messages GGA, RMC, GLL
// Arduino source code for ATMEGA328P or other MCUs
// Linux version: compile mylytsja.cpp
// 2024 DN
//

#include <stdio.h>

// Module: ATGM332D, 9600 baud
// $GNGGA,hhmmss.000,ddmm.mmmmm,N,dddmm.mmmmm,E,1,06,2.6,a.a,M,0.0,M,,*cs
// $GNRMC,hhmmss.000,A,ddmm.mmmmm,N,dddmm.mmmmm,E,0.24,163.30,030124,,,A*cs

// Module: QUECTEL L26-T, 9600 baud
// $GPGGA,hhmmss.000,ddmm.mmmmm,N,dddmm.mmmmm,E,1,05,2.0,a.aa,M,44.7,M,,*cs
// $GPRMC,hhmmss.000,A,ddmm.mmmmm,N,dddmm.mmmmm,E,0.7,352.7,030124,,,A*cs

// Module: U-BLOX NEO-6M, 9600 baud
// $GPGGA,hhmmss.00,ddmm.mmmmm,N,dddmm.mmmmm,E,1,06,1.98,a.a,M,43.1,M,,*cs
// $GPRMC,hhmmss.00,A,ddmm.mmmmm,N,dddmm.mmmmm,E,0.067,,030124,,,A*cs
// $GPGLL,ddmm.mmmmm,N,dddmm.mmmmm,E,hhmmss.00,A,A*cs

// Arduino settings

#ifdef ARDUINO
#define SERIAL_BAUDRATE 9600 
#define SERIAL Serial
#endif // ARDUINO

// Data modification settings

// Use these:

// #define REPLACE_LAT "1234.5678" // Uncomment to replace latitude with this value
// #define REPLACE_LAT_DIR "S" // Uncomment to replace N/S direction

// #define REPLACE_LON "12345.6789" // Uncomment to replace longitude with this value
// #define REPLACE_LON_DIR "E" // Uncomment to replace E/W direction

// #define REPLACE_ALT "123.4" // Uncomment to replace altitude with this value

// Or these:

#define ADD_LAT_DEG -10 // Uncomment to add integer number of degrees to the latitude, negative value to subtract (Warining: do not cross 0deg/90deg borders!)
#define ADD_LON_DEG 20 // Uncomment to add integer number of degrees to the longitude, negative value to subtract (Warining: do not cross 0deg/180deg borders!)

#define MUL_ALT 0.5 // Uncomment to multiply altitude to this coefficient
#define ALT_PRECISION 1 // Number of decimal points for encoding the altitude (TODO: auto-detect)


// The program starts here

enum
{
  STATE_WAITFOR_CR_OR_LF,
  STATE_CR_OR_LF_RECEIVED,
  STATE_PERCENT_RECEIVED,
  STATE_PERCENT_G_RECEIVED,
  STATE_RECEIVE_FIELDS,
  STATE_RECEIVE_CHECKSUM_FIRST,
  STATE_RECEIVE_CHECKSUM_SECOND,
};

int parser_state = STATE_WAITFOR_CR_OR_LF;


enum
{
  MESSAGE_TYPE_OTHER,
  MESSAGE_TYPE_GGA,
  MESSAGE_TYPE_RMC,
  MESSAGE_TYPE_GLL,
};

int message_type = MESSAGE_TYPE_OTHER; // Current message type

// Send single character to the main device
void send_char(char c)
{
#ifdef ARDUINO
  SERIAL.write(c);
#else
  putchar(c);
#endif  
}

int receive_field_number; // 0=message type string, etc.

#define MAX_FIELD_LEN 100 
char field_buffer[MAX_FIELD_LEN+1]; // Temporary buffer for the fiels we are currently receiving

uint8_t checksum;

// Modify the field, if needed
void modify_field()
{
// See: https://w3.cs.jmu.edu/bernstdh/web/common/help/nmea-sentences.php
// See: https://www.rfwireless-world.com/Terminology/GPS-sentences-or-NMEA-sentences.html

  // Latitude
  if ( ((message_type==MESSAGE_TYPE_GGA)&&(receive_field_number==2)) ||
   ((message_type==MESSAGE_TYPE_RMC)&&(receive_field_number==3)) ||
   ((message_type==MESSAGE_TYPE_GLL)&&(receive_field_number==1)) 
   )
   {
#ifdef REPLACE_LAT
      strcpy(field_buffer, REPLACE_LAT);
#endif    
    
#ifdef ADD_LAT_DEG
    if (strlen(field_buffer)>=4) // Expecting DDMM(.m)
    {
      int lat_deg = (field_buffer[0]-'0')*10 + (field_buffer[1]-'0');
      lat_deg += ADD_LAT_DEG;
      field_buffer[0] = lat_deg/10 + '0';
      field_buffer[1] = lat_deg%10 + '0';
    }
#endif

   }
  else
  // Latitude direction
  if ( ((message_type==MESSAGE_TYPE_GGA)&&(receive_field_number==3)) ||
   ((message_type==MESSAGE_TYPE_RMC)&&(receive_field_number==4)) ||
   ((message_type==MESSAGE_TYPE_GLL)&&(receive_field_number==2)) )
   {
#ifdef REPLACE_LAT_DIR
      strcpy(field_buffer, REPLACE_LAT_DIR);
#endif      
   }
  else

  // Longitude
  if ( ((message_type==MESSAGE_TYPE_GGA)&&(receive_field_number==4)) ||
   ((message_type==MESSAGE_TYPE_RMC)&&(receive_field_number==5)) ||
   ((message_type==MESSAGE_TYPE_GLL)&&(receive_field_number==3)) )
   {
#ifdef REPLACE_LON
      strcpy(field_buffer, REPLACE_LON);
#endif    

#ifdef ADD_LAT_DEG
    if (strlen(field_buffer)>=5) // Expecting DDDMM(.m)
    {
       int lon_deg = (field_buffer[0]-'0')*100 + (field_buffer[1]-'0')*10 + (field_buffer[2]-'0');
       lon_deg += ADD_LON_DEG;
       field_buffer[0] = lon_deg/100 + '0';
       field_buffer[1] = (lon_deg%100)/10 + '0';
       field_buffer[2] = lon_deg%10 + '0';
    }
#endif

   }
  else

  // Longitude direction

  if ( ((message_type==MESSAGE_TYPE_GGA)&&(receive_field_number==5)) ||
   ((message_type==MESSAGE_TYPE_RMC)&&(receive_field_number==6)) ||
   ((message_type==MESSAGE_TYPE_GLL)&&(receive_field_number==4)) )
   {
#ifdef REPLACE_LON_DIR
      strcpy(field_buffer, REPLACE_LON_DIR);
#endif    
   }
  else

  // Altitude
  if ( (message_type==MESSAGE_TYPE_GGA)&&(receive_field_number==9) )
   {
#ifdef REPLACE_ALT
      strcpy(field_buffer, REPLACE_ALT);
#endif    

#ifdef MUL_ALT
      double alt = atof(field_buffer);
      alt *= MUL_ALT;

  #ifdef ARDUINO
      dtostrf(alt, 0, ALT_PRECISION, field_buffer);
  #else
     sprintf(field_buffer, "%.*f", ALT_PRECISION, alt);
  #endif

#endif

   }

   // Add more handlers here
}


// Single character is received from the GPS module
void char_received(char c)
{
  switch(parser_state)
  {
    case STATE_WAITFOR_CR_OR_LF:
      {
          if ( (c=='\r') || (c=='\n') )
            parser_state = STATE_CR_OR_LF_RECEIVED;
          send_char(c);
      }
      break;

    case STATE_CR_OR_LF_RECEIVED:
      {
          if ( c=='$' )
          {
            parser_state = STATE_PERCENT_RECEIVED;
          }
          else
          if ( (c!='\r') && (c!='\n') )
            parser_state = STATE_WAITFOR_CR_OR_LF;
          send_char(c);
      }
      break;

   case STATE_PERCENT_RECEIVED:
      {
          if ( c=='G' )
          {
            parser_state = STATE_PERCENT_G_RECEIVED;
          }
          else
            parser_state = STATE_WAITFOR_CR_OR_LF;
          send_char(c);
      }
      break;

   case STATE_PERCENT_G_RECEIVED:
      {
          if ( (c=='N') || (c=='P') )
          {
            parser_state = STATE_RECEIVE_FIELDS;
            checksum = 'G'^c; // Init checksum to "GN" or "GP"
            receive_field_number = 0;
            field_buffer[0] = 0;
            message_type = MESSAGE_TYPE_OTHER;
          }
          else
            parser_state = STATE_WAITFOR_CR_OR_LF;
          send_char(c);
      }
      break;
   
   case STATE_RECEIVE_FIELDS:
      {
          switch(c)
          {

            case '*': // End of line
              parser_state = STATE_RECEIVE_CHECKSUM_FIRST;
              // fall through, no break 
              
            case ',': // End of field, other field follows
              {              
                if (receive_field_number==0) // Message type field
                {
                  if (!strcmp(field_buffer, "GGA"))
                    message_type = MESSAGE_TYPE_GGA;
                  else
                  if (!strcmp(field_buffer, "RMC"))
                    message_type = MESSAGE_TYPE_RMC;
                  else
                  if (!strcmp(field_buffer, "GLL"))
                    message_type = MESSAGE_TYPE_GLL;
                }

                modify_field(); // Change data, if needed

                // Send the current field to the receiving device and update checksum
                const char *s = field_buffer;
#ifdef ARDUINO
                digitalWrite(LED_BUILTIN, HIGH);
#endif                
                while(*s)
                {
                  send_char(*s); 
                  checksum ^= *s;
                  s++;
                }
#ifdef ARDUINO
                digitalWrite(LED_BUILTIN, LOW);
#endif                

                send_char(c); // End with ',' or '*'
                if (c!='*')
                  checksum ^= c; // Update checksum with ','

                field_buffer[0] = 0;
                receive_field_number++;
              }
              break;

            default:
              int l = strlen(field_buffer);
              if (l<MAX_FIELD_LEN)
              {
                  field_buffer[l] = c;
                  field_buffer[l+1] = 0;  
              }
          }
      }
      break;

   case STATE_RECEIVE_CHECKSUM_FIRST:
      {
          c = (checksum>>4) & 0x0F;          
          send_char( (c<10) ? (c+'0') : (c+'A'-10));
          parser_state = STATE_RECEIVE_CHECKSUM_SECOND;
      }
      break;  

   case STATE_RECEIVE_CHECKSUM_SECOND:
      {
          c = checksum & 0x0F;          
          send_char( (c<10) ? (c+'0') : (c+'A'-10));
          parser_state = STATE_WAITFOR_CR_OR_LF;
     }
      break;  
  }
}


#ifdef ARDUINO  

void setup() 
{
#ifdef ARDUINO  
  SERIAL.begin(SERIAL_BAUDRATE);
  pinMode(LED_BUILTIN, OUTPUT);
#endif
}

void loop() 
{
  while (SERIAL.available())
  {
    char c = SERIAL.read();
    char_received(c);
  }
}

#endif // ARDUINO
