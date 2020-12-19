#include "U8glib.h"
#include <avr/wdt.h>
#include <EEPROM.h>
#define LINE_MAX 30
#define ROW_MAX 12
#define LINE_PIXEL_HEIGHT 7
U8GLIB_ST7920_128X64 u8g(13, 11, 10, U8G_PIN_NONE);
int PANIC_PIN;
int BUZZER_PIN =6 ;
byte D_BIT;
byte AUTO_REBBOT_ON_PANIC;
byte ECHO_TO_SERIAL = 0;
byte STARTUP_PP;
void(* resetFunc) (void) = 0;
uint8_t line_buf[LINE_MAX] = "Initializing BIOS...";
uint8_t line_pos = 0;
uint8_t screen[ROW_MAX][LINE_MAX];
uint8_t rows, cols;
void clear_screen(void) {
  uint8_t i, j;
  for ( i = 0; i < ROW_MAX; i++ )
    for ( j = 0; j < LINE_MAX; j++ )
      screen[i][j] = 0;
}
void add_line_to_screen(void) {
  uint8_t i, j;
  for ( j = 0; j < LINE_MAX; j++ )
    for ( i = 0; i < rows - 1; i++ )
      screen[i][j] = screen[i + 1][j];

  for ( j = 0; j < LINE_MAX; j++ )
    screen[rows - 1][j] = line_buf[j];
}
void draw(void) {
  uint8_t i, y;
  // graphic commands to redraw the complete screen are placed here
  y = 0;       // reference is the top left -1 position of the string
  y--;           // correct the -1 position of the drawStr
  for ( i = 0; i < rows; i++ )
  {
    u8g.drawStr( 0, y, (char *)(screen[i]));
    y += u8g.getFontLineSpacing();
  }
}
void exec_line(void) {
  // echo line to the serial monitor
  //Serial.println((const char *)line_buf);

  // add the line to the screen
  add_line_to_screen();

  // U8GLIB picture loop
  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );
}
void reset_line(void) {
  line_pos = 0;
  line_buf[line_pos] = '\0';
}
void char_to_line(uint8_t c) {
  line_buf[line_pos] = c;
  line_pos++;
  line_buf[line_pos] = '\0';
}
void read_line(void) {
  if ( Serial.available() )
  {
    uint8_t c;
    c = Serial.read();
    if ( line_pos >= cols - 1 ) {
      exec_line();
      reset_line();
      char_to_line(c);
    }
    else if ( c == '\n' ) {
      // ignore '\n'
    }
    else if ( c == '\r' ) {
      exec_line();
      reset_line();
    }
    else {
      char_to_line(c);
    }
  }
}
void DisplaySetup() {
  // set font for the console window
  u8g.setFont(u8g_font_5x7);
  //u8g.setFont(u8g_font_9x15);

  // set upper left position for the string draw procedure
  u8g.setFontPosTop();

  // calculate the number of rows for the display
  rows = u8g.getHeight() / u8g.getFontLineSpacing();
  if ( rows > ROW_MAX )
    rows = ROW_MAX;

  // estimate the number of columns for the display
  cols = u8g.getWidth() / u8g.getStrWidth("m");
  if ( cols > LINE_MAX - 1 )
    cols = LINE_MAX - 1;

  clear_screen();               // clear screen
  delay(150);                  // do some delay
  Serial.begin(9600);        // init serial
  exec_line();                    // place the input buffer into the screen
  reset_line();
}
void printscreen(String text, bool CRLF) {
  const char* string1 = text.c_str();
  strcat(line_buf, string1);
  exec_line(); // clear input buffer
  if (int(ECHO_TO_SERIAL) == 1) {
    Serial.println(text);
  }
  if (CRLF) {
    reset_line();
  }

}
void setup(void) {
  DisplaySetup();
  int exit_code = OS();
  printscreen("-------------------", true);
  printscreen("-------------------", true);
  if (exit_code == 0) {
    printscreen("OS EXITED !", true);
    tone(BUZZER_PIN, 6000);
    delay(500);
    noTone(BUZZER_PIN);
  }
  else {
    printscreen("OS PANIC !", true);
    printscreen("OS ERROR CODE: " + String(exit_code), true);
    digitalWrite(PANIC_PIN, HIGH);
    tone(BUZZER_PIN, 4000);
    if (int (AUTO_REBBOT_ON_PANIC) = 1) {
      resetFunc();
    }
  }
}
int availableMemory() {
  // Use 1024 with ATmega168
  int size = 2048;
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  return size;
}
int BIOS(void) {
  String string_buffer;
  byte mgk;
  mgk = EEPROM.read(4);
  ECHO_TO_SERIAL = mgk;
  string_buffer = "Echo_Serial byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  mgk = EEPROM.read(0);
  D_BIT = mgk;
  string_buffer = "Magic byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  if (String(mgk) != "255") {
    printscreen("D-BIT detected...", true);
    return 0;
  }
  mgk = EEPROM.read(1);
  string_buffer = "Panic byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  if (int((mgk)) > (13)) {
    return 101;
  }
  PANIC_PIN = int(mgk);
  pinMode(PANIC_PIN, OUTPUT);
  mgk = EEPROM.read(2);
  BUZZER_PIN = int (mgk);
  //pinMode(BUZZER_PIN,OUTPUT);
  string_buffer = "Buzzer byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  mgk = EEPROM.read(3);
  AUTO_REBBOT_ON_PANIC = mgk;
  string_buffer = "Reboot_Panic byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  mgk = EEPROM.read(5);
  STARTUP_PP = mgk;
  string_buffer = "Startup_PP byte: " + String("0x") + String(mgk, HEX) ;
  printscreen(string_buffer, true);
  tone(BUZZER_PIN, 10000);
  delay(1000);
  tone(BUZZER_PIN, 8000);
  delay(200);
  noTone(BUZZER_PIN);
  return 0;
}
int pinMode(uint8_t pin)
{
  if (pin >= NUM_DIGITAL_PINS) return (-1);

  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *reg = portModeRegister(port);
  if (*reg & bit) return (OUTPUT);

  volatile uint8_t *out = portOutputRegister(port);
  return ((*out & bit) ? INPUT_PULLUP : INPUT);
}
int Processcommand(String command) {
  String string_buffer;
  if (command.indexOf("reboot") != -1 ) {
    command.replace("reboot", " ");
    command = command.toInt();
    string_buffer = "Rebooting in " + command ;
    printscreen(string_buffer, true);
    int counter2 = 0;
    counter2 = command.toInt();
    for (counter2 ; counter2 > 0 ; counter2--) {
      string_buffer = counter2 ;
      printscreen(string_buffer, true);
      delay( 1000);
    }
    MCUSR = 0;
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
  }
  if (command.indexOf("show") != -1 ) {
    if (command.indexOf(" memory ") != -1 ) {
      string_buffer = "Availbvle RAM: " + String(availableMemory()) + " kb";
      printscreen(string_buffer, true);
      return 0 ;
    }
    else if (command.indexOf(" pin " != -1)) {
      command.replace("show ", "");
      command.replace("pin", "");
      string_buffer =  "MODE is: " + String(pinMode(command.toInt())) + "  VALUE is: " + digitalRead(command.toInt());
      printscreen(string_buffer, true);
      return 0 ;
    }
  }
  if (command.indexOf("set") != -1 ) {
    if (command.indexOf(" pin ") != -1 ) {
      if (command.indexOf(" mode ") != -1 ) {
        command.replace("set", "");
        command.replace("pin", "");
        command.replace("mode", "");
        if (command.indexOf("output") != -1 ) {
          command.replace("output", "");
          pinMode(command.toInt(), OUTPUT);
        digitalWrite(command.toInt(),HIGH);
          string_buffer =  "PIN " + String(command.toInt()) + " is now OUTPUT";
          printscreen(string_buffer, true);
          return 0;
        }
        else if (command.indexOf("input") != -1 ) {
          command.replace("input", "");
          pinMode(command.toInt(), INPUT);
          string_buffer =  "PIN " + String(command.toInt()) + " is now INPUT";
          printscreen(string_buffer, true);
          return 0 ;
        }
      }
      else if (command.indexOf(" value ") != -1 ) {
       command.replace("set", "");
        command.replace("pin", "");
        command.replace("value", "");
        if (command.indexOf("high") != -1 ) {
          command.replace("high", "");
         digitalWrite(command.toInt(),HIGH);
          string_buffer =  "PIN " + String(command.toInt()) + " is now HIGH";
          printscreen(string_buffer, true);
          return 0;
        }
        else if (command.indexOf("low") != -1 ) {
          command.replace("low", "");
                 string_buffer =  command.toInt();
          printscreen(string_buffer, true);
             digitalWrite(command.toInt(),LOW);
          string_buffer =  "PIN " + String(command.toInt()) + " is now LOW";
          printscreen(string_buffer, true);
          return 0 ;
        }
      }
    }
  }
}
 
int OS(void) {
//  String 
  //int BIOS_val ;
  // BIOS_val = BIOS;
  //if ( BIOS_val != 0) {
  //  return BIOS_val;
  //}
  int BIOS_CODE = BIOS();
  int tesla_start_relay = 3;
  int motor_relay = 4;
  int LED_relay = 6;
  int LED_butt = A3;
  int motor_butt =A2;
  int tesla_start_butt =A1;
  int tesla_stop_butt = A0;
  bool tesla_on = false;
  bool LED_on = false;
  bool motor_on = false;
  
  pinMode(tesla_start_relay,OUTPUT);
  pinMode(motor_relay,OUTPUT);
  pinMode(LED_relay,OUTPUT);
  pinMode(LED_butt,INPUT);
  pinMode(motor_butt,INPUT);
  pinMode(tesla_start_butt,INPUT);
  pinMode(tesla_stop_butt,INPUT);
  
  if (BIOS_CODE != 0) {
    return BIOS_CODE;
  }
  String string_buffer;
  string_buffer = ">>>>Neo OS version 1<<<<";
  printscreen(string_buffer, true);
  string_buffer = "Availbvle RAM: " + String(availableMemory()) + " kb";
  printscreen(string_buffer, true);
  string_buffer = "Shell>>";
  printscreen(string_buffer, true);
  while (true) {
    String command;
    if (Serial.available()) {
      command = Serial.readString();
      string_buffer = command;
      printscreen(string_buffer, true);
      Processcommand(command);
      string_buffer = "Shell>>";
      printscreen(string_buffer,true);
    }
    if (digitalRead(tesla_stop_butt) == HIGH){           //tesla off key
string_buffer = "Tesla Stop processing...";
printscreen(string_buffer,true);
if ( tesla_on == true){
  digitalWrite(tesla_start_relay,LOW);
  tesla_on = false;
  string_buffer = "Tesla is off";
  printscreen(string_buffer,true);
}
else{
    string_buffer = "Tesla is already off!";
    printscreen(string_buffer,true);
}

      delay(500);
    }
    
if (digitalRead(tesla_start_butt) == HIGH){           //tesla on key
string_buffer = "Tesla Start processing... ";
printscreen(string_buffer,true);
if ( tesla_on == false){
 string_buffer =  "ON TIME= 5SEC";
 printscreen(string_buffer,true); 
 //for (int x = 0 ; x < 5 ;x ++){
  // string_buffer =  5 - x;
   //printscreen(string_buffer,true);
 // delay(1000); 
 //}
  digitalWrite(tesla_start_relay,HIGH);
  tesla_on = true;
  string_buffer = "WARNINH!!  Tesla is on";
  printscreen(string_buffer,true);
}
else{
    string_buffer = "Tesla is already on!";
    printscreen(string_buffer,true);
}
  string_buffer = "Shell>>";
  printscreen(string_buffer, true);
      delay(100);
    }
    if (digitalRead(motor_butt) == HIGH){           //motor key 
string_buffer = "Sparkgap  processing... ";
printscreen(string_buffer,true);
if ( motor_on == false){
  digitalWrite(motor_relay,HIGH);
  motor_on = true;
  string_buffer = "Sparkgap is on";
  printscreen(string_buffer,true);
}
else{
  digitalWrite(motor_relay,LOW);
  motor_on =false;
    string_buffer = "Sparkgap is off";
    printscreen(string_buffer,true);
}
  string_buffer = "Shell>>";
  printscreen(string_buffer, true);
      delay(100);
    }
       if (digitalRead(LED_butt) == HIGH){           //LED key 
string_buffer = "Tesla LED  processing... ";
printscreen(string_buffer,true);
if ( LED_on == false){
  digitalWrite(LED_relay,HIGH);
  LED_on = true;
  string_buffer = "Tesla LED is on";
  printscreen(string_buffer,true);
}
else{
  digitalWrite(LED_relay,LOW);
  LED_on =false;
    string_buffer = "Tesla LED is off";
    printscreen(string_buffer,true);
}
  string_buffer = "Shell>>";
  printscreen(string_buffer, true);
      delay(100);
    }
      delay(20);
    //  digitalWrite(Tesla_start_relay,HIGH);
    //read_line();
  //  digitalWrite(3,HIGH);
  }
  return 0;
}

void loop(void) {

}
