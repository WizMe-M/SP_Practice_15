#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include <OneButton.h>

#define LATCH_PIN 4
#define CLOCK_PIN 7
#define DATA_PIN 8
#define BUZZER_PIN 3

#define RX_PIN 0
#define TX_PIN 1

#define SLEEP "SLEEP"
#define START "START"
#define PAUSE "PAUSE"
#define STOP "STOP"

String input;
OneButton btn(A2);

Event timer;
bool alarming = false;

//общее количество тиков таймера (количество секунд)
unsigned int sleepTime;

unsigned int currentSleepTime;

//коды для цифр от 0 до 9 под соответствующими индексами
const int number_codes[] = {~B11111100, ~B01100000, ~B11011010, ~B11110010, ~B01100110,
                            ~B10110110, ~B10111110, ~B11100000, ~B11111110, ~B11110110};

// коды для разрядов числа, начиная с разряда единиц и заканчивая разрядом тысяч
const int digit_codes[] = {0xF8, 0xF4, 0xF2, 0xF1};

//цифры для вывода
uint8_t digits[4];

void stopAlarm()
{
  alarming = false;
  analogWrite(BUZZER_PIN, 255);
}

void separateNumber(int num)
{
  for (uint8_t i = 3; i >= 0; i--)
  {
    digits[i] = num % 10;
    num /= 10;
  }

  for (int i = 3; i > 0; i--)
  {
    if (digits[i] == 0)
    {
      digits[i] = 10;
    }
    else
    {
      break;
    }
  }
}

void convertToInt(int mins, int secs)
{
  sleepTime = mins * 60 + secs;
  currentSleepTime = sleepTime;
  separateNumber(currentSleepTime);
}

void countdown()
{
  separateNumber(currentSleepTime);
  currentSleepTime--;
}

void setup()
{
  Serial.begin(9600);

  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Hi, I am Arduino Timkin");

  convertToInt(01, 00);
  timer.initEvery(1000, countdown, sleepTime);

  btn.attachClick(stopAlarm);
}

bool isCorrect(int time)
{
  return time >= 0 && time < 60;
}

void handleInput(String s)
{
  if (s.startsWith(SLEEP) && s.length() == 10)
  {
    int mins = s.substring(5, 7).toInt();
    int secs = s.substring(8, 10).toInt();
    if (!isCorrect(mins))
    {
      Serial.println("Minutes should be in range from 00 to 59");
    }

    if (!isCorrect(secs))
    {
      Serial.println("Seconds should be in range from 00 to 59");
    }

    if (!isCorrect(mins) || !isCorrect(secs))
      return;

    convertToInt(mins, secs);
    Serial.println((String) "i sleep... " + mins + (String) ":" + secs);
    timer.initEvery(1000, countdown, sleepTime);
  }
  else if (s == START)
  {
    Serial.println("timer tries to start");
    timer.resume();
  }
  else if (s == PAUSE)
  {
    Serial.println("timer tries to pause");
    timer.pause();
  }
  else if (s == STOP)
  {
    Serial.println("timer tries to stop");
    timer.stop();
  }
  else
  {
    Serial.println("Some unhandled input...");
  }
  Serial.println();
}

void alarm()
{
  if (alarming)
  {
    analogWrite(BUZZER_PIN, 0);
  }
  else
  {
    analogWrite(BUZZER_PIN, 255);
  }
}

void displayTime()
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, number_codes[digits[i]]);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digit_codes[i]);
    digitalWrite(LATCH_PIN, HIGH);
  }
}

void loop()
{
  timer.update();
  btn.tick();
  if (Serial.available())
  {
    input = Serial.readString();
    input.trim();
    Serial.println((String) "Got input: `" + input + "`");
    handleInput(input);
  }
  displayTime();
  alarm();
}