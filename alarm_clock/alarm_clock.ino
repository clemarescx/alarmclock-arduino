#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <TFT.h>

#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    8
#define TFT_SCLK 13   
#define TFT_MOSI 11   

TFT TftScreen = TFT(TFT_CS,  TFT_DC, TFT_RST);
RTC_DS1307 rtc;

#define reset_button       2
#define set_alarm_button   3
#define speakerPin          4
#define hoursModulator     A0
#define minutesModulator   A1

int reset_button_state = 0;
int set_alarm_button_state = 0;

bool alarm_on = false;

const int print_now_value_y_offset = 20;
const int print_alarm_title_y_offset = 40;
const int print_alarm_value_y_offset = 60;

DateTime alarmTime;

void setup() {
  Serial.begin(9600);

  setup_RTC();

  pinMode(speakerPin, OUTPUT);
  pinMode(reset_button, INPUT);
  pinMode(set_alarm_button, INPUT);


  TftScreen.begin();
  TftScreen.setTextSize(2);
  resetScreen();
}

void loop() {
  
  reset_button_state = digitalRead(reset_button);

  if (reset_button_state == HIGH) {
    Serial.println("Reset button pressed");
    reset();
    resetScreen();
  }

  DateTime now = rtc.now();
  set_alarm_button_state = digitalRead(set_alarm_button);

  if (set_alarm_button_state == HIGH) {
    Serial.println("Set alarm button pressed");
    // enter the "Set alarm" state;
    // the function clears the screan once it's done
    alarm_on = setAlarm(now);
  }

  TftScreen.stroke(255, 255, 255);
  printTime(now, print_now_value_y_offset);
  delay(10);
  // erase
  TftScreen.stroke(0, 0, 0);
  printTime(now, print_now_value_y_offset);

  if (alarm_on) {
    if( (alarmTime - now).totalseconds() <= 0)
       play_alarm();
  }
}

String doubleDigitFormat(int timeunit) {
  String timeStr = String(timeunit);
  if (timeunit < 10)
    return "0" + timeStr;
  return timeStr;
}

void printTime(const DateTime& dateTime, const int& y_offset) {
  String hour = doubleDigitFormat(dateTime.hour());
  String minute = doubleDigitFormat(dateTime.minute());
  String second = doubleDigitFormat(dateTime.second());
  String stringPrintout = hour + ':' + minute + ':' + second;
  char printout[9];
  stringPrintout.toCharArray(printout, 9);
  TftScreen.text(printout, 0, y_offset);
}

void play_alarm() {
  for (int i = 0; i < 3; i++) {
    tone(4, 1100, 100);
    delay(200);
  }
  delay(800);
}

void setup_RTC() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}
void reset() {
  alarm_on = false;
}

void resetScreen() {
  TftScreen.background(0, 0, 0);
  TftScreen.stroke(255, 255, 255);
  TftScreen.text("Time\n ", 0, 0);
}

/// Starts the "set alarm" state
/// Contains a secondary main-loop
bool setAlarm(const DateTime& now) {
  // clear the screen as we enter the new state + initialise local variables
  TftScreen.background(0, 0, 0);
  TftScreen.stroke(255, 255, 255);
  TftScreen.text("Set alarm\n ", 0, 0);
  DateTime alarm;
  bool alarm_is_set = false;
  int alarmHour;
  int alarmMinutes;


  while (true) {
    if (digitalRead(reset_button) == HIGH) {
      break;
    }
    if (digitalRead(set_alarm_button) == HIGH) {
      // set the correct timespan according to the datetime now
      alarm_is_set = true;
      break;
    }

    int hours_reading = analogRead(hoursModulator);
    int minutes_reading = analogRead(minutesModulator);
    Serial.print("analoghours = ");
    Serial.println(hours_reading);
    Serial.print("analogminutes = ");
    Serial.println(minutes_reading);

    // Small adjustments of the "from" upperbound 
    // to get a better coverage of the whole range
    alarmHour = map(hours_reading, 0, 1000, 0, 23);   
    alarmMinutes = map(minutes_reading, 0, 1010, 0, 59);

    String hours = doubleDigitFormat(alarmHour);
    String minutes = doubleDigitFormat(alarmMinutes);
    char alarmPrintout[6];
    (hours + ':' + minutes).toCharArray(alarmPrintout, 6);
    TftScreen.stroke(255, 255, 255);
    TftScreen.text(alarmPrintout, 0, print_now_value_y_offset);
    delay(100);
    TftScreen.stroke(0, 0, 0);
    TftScreen.text(alarmPrintout, 0, print_now_value_y_offset);
  }

  // clear the screen to initial state before returning
  resetScreen();

  if (alarm_is_set) {
    int hour_offset = alarmHour - now.hour();
    int minutes_offset = alarmMinutes - now.minute();
    TimeSpan alarm_offset = TimeSpan(0, hour_offset, minutes_offset, -now.second());
    alarmTime = now + alarm_offset;
    // if the alarm was set to earlier than now, add a day.
    if ( (alarmTime - now).totalseconds() < 0) 
      alarmTime = alarmTime + TimeSpan(1, 0, 0, 0);
      
    TftScreen.text("Alarm set for:\n", 0, print_alarm_title_y_offset);
    printTime(alarmTime, print_alarm_value_y_offset);
  }
  return alarm_is_set;
}

