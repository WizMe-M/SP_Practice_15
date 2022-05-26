#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include <OneButton.h>

#define LATCH_PIN 4
#define CLOCK_PIN 7
#define DATA_PIN 8
#define BUZZER_PIN 3

#define BUZZER_ON 0
#define BUZZER_OFF 255

SoftwareSerial bluetooth(0, 1); // rx = 0, tx = 1 !!!
OneButton btn(A2);
Timer displayTimer;

//коды для цифр от 0 до 9 под соответствующими индексами
const int number_codes[] = {~B11111100, ~B01100000, ~B11011010, ~B11110010, ~B01100110,
                            ~B10110110, ~B10111110, ~B11100000, ~B11111110, ~B11110110};
// коды для разрядов числа, начиная с разряда единиц и заканчивая разрядом тысяч
const int digit_codes[] = {0xF1, 0xF2, 0xF4, 0xF8 };
//цифры для вывода
uint8_t digits[4];

int maxSeconds;
int currentSeconds;

bool isCorrect(int time)
{
  return time >= 0 && time < 60;
}

void separateTime()
{
  //из 142 секунд
  uint8_t minutes = currentSeconds / 60;
  uint8_t seconds = currentSeconds % 60;
  //в 02:22
  digits[0] = minutes / 10;
  digits[1] = minutes % 10;
  digits[2] = seconds / 10;
  digits[3] = seconds % 10;
}

void setTime(int minutes, int seconds)
{
  maxSeconds = minutes * 60 + seconds;
  bluetooth.println((String)"Timer set on " + maxSeconds + (String)" seconds");
  currentSeconds = maxSeconds;
}

void countdown()
{
  currentSeconds--;
  bluetooth.println((String)"Timer updated: " + currentSeconds);
  separateTime();
}

void handleInput(String s)
{
  if (s.startsWith("SLEEP") && s.length() == 10)
  {
    int mins = s.substring(5, 7).toInt();
    int secs = s.substring(8, 10).toInt();
    if (!isCorrect(mins))
    {
      bluetooth.println("Minutes should be in range from 00 to 59");
    }

    if (!isCorrect(secs))
    {
      bluetooth.println("Seconds should be in range from 00 to 59");
    }

    if (!isCorrect(mins) || !isCorrect(secs))
      return;

    bluetooth.println("i sleep... ");
    setTime(mins, secs);
    separateTime();
    displayTimer.init(1000, countdown, maxSeconds);
  }
  else if (s == "START")
  {
    bluetooth.println("timer tries to start");
    displayTimer.start();
  }
  else if (s == "PAUSE")
  {
    bluetooth.println("timer tries to pause");
    displayTimer.pause();
  }
  else if (s == "STOP")
  {
    bluetooth.println("timer tries to stop");
    displayTimer.stop();
    currentSeconds = maxSeconds;
  }
  else
  {
    bluetooth.println("Some unhandled input...");
  }
}

void refreshIndicator()
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, number_codes[digits[i]]);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digit_codes[i]);
    digitalWrite(LATCH_PIN, HIGH);
  }
}

void setup()
{
  bluetooth.begin(9600);
  bluetooth.flush();
  //ожидаем подтверждения подключения от терминала
  while (!bluetooth.available()) ;
  //считываем данные, чтобы следующий input не был битым
  bluetooth.readString();
  bluetooth.println("Hi, I am Arduino Timkin");

  //настраиваем семисегментник на вывод
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
}

void loop()
{
  if (bluetooth.available())
  {
    String input = bluetooth.readString();
    input.trim();
    bluetooth.println((String) "# Got input: `" + input + "`");
    handleInput(input);
  }

  displayTimer.update();
  refreshIndicator();
}