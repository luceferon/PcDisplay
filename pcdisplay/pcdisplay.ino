#include <FS.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <GyverTimer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <TaskScheduler.h> // Добавляем TaskScheduler

File fsUploadFile;
TFT_eSPI tft = TFT_eSPI();

#define BTN_PIN 4 // Пин для кнопки

StaticJsonDocument<1600> owm;
const char* ssid = "sysadmin";
const char* password = "213213213";
const String api_1 = "http://api.openweathermap.org/data/2.5/weather?q=";
const String qLocation = "Zelenogorsk,RU";
const String api_2 = "&lang=ru&units=metric";
const String api_3 = "&APPID=";
const String api_key = "7f8758891d00d3848d8a524a922ad057";
int timezone = 7;

byte syncerror = 0;
volatile byte screen = 0;
byte cpu_temp[100];
byte cpu_index = 0;
byte gpu_temp[100];
byte gpu_index = 0;
int sync_interval = 1000;
int i = 0;
String responce;

String dataServer = "http://192.168.28.64:8086/data.json";
String degree = degree.substring(degree.length()) + "°C";
String percentage = percentage.substring(percentage.length()) + (char)37;
float total;
float vals[32];

ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * timezone);

GTimer refsens(MS);
GTimer refscreen(MS);
GTimer refweather(MS);

// Флаг для защиты от дребезга
volatile bool buttonPressed = false;

IRAM_ATTR void btn_read() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();

    // Защита от дребезга: игнорируем повторные срабатывания в течение 200 мс
    if (interruptTime - lastInterruptTime > 200) {
        buttonPressed = true; // Устанавливаем флаг, что кнопка была нажата
    }

    lastInterruptTime = interruptTime;
}

// Создаем планировщик задач
Scheduler runner;

// Задачи
void getViaBTCDataTask();
void hardwareMonitorTask();
Task tViaBTC(300000, TASK_FOREVER, &getViaBTCDataTask); // Задача для ViaBTC API (каждые 5 минут)
Task tLHM(1500, TASK_FOREVER, &hardwareMonitorTask); // Задача для LHM (каждые 1,5 секунды)

void setup(void) {
    tft.init();
    tft.setRotation(1);
    pinMode(BTN_PIN, INPUT_PULLUP); // Настройка пина кнопки с подтягивающим резистором
    attachInterrupt(digitalPinToInterrupt(BTN_PIN), btn_read, FALLING); // Прерывание на нажатие кнопки
    Serial.println(F("Инициализировано"));
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    Serial.println("Экран инициализирован");

    refsens.setInterval(sync_interval);
    refscreen.setInterval(1000);
    refweather.setInterval(300000);

    Serial.begin(115200);
    Serial.println("Серийный запущен 115200");
    WiFi.begin(ssid, password);
    Serial.println("Подключение");
    tft.println("Подключение...");

    // Ждем подключения к Wi-Fi
    int count = 0;
    while (WiFi.status() != WL_CONNECTED && count < 20) { // Увеличиваем таймаут до 20 попыток
        delay(500);
        Serial.print(".");
        count++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        // Если не удалось подключиться, запускаем WiFiManager
        WiFiManager wifiManager;
        tft.setCursor(0, 20);
        tft.println("Не удалось подключиться");
        tft.setCursor(0, 40);
        tft.println("Подключитесь к PCdisplay");
        tft.setCursor(0, 60);
        tft.println("IP адрес: 192.168.4.1");
        wifiManager.autoConnect("PCDisplay");
    }

    // Если подключились к Wi-Fi
    if (WiFi.status() == WL_CONNECTED) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        Serial.print("Подключен к ");
        Serial.println(WiFi.SSID());
        tft.print("Подключен к ");
        tft.println(WiFi.SSID());
        tft.setCursor(0, 20);
        Serial.print("IP адрес: ");
        Serial.println(WiFi.localIP());
        tft.print("IP адрес: ");
        tft.println(WiFi.localIP());

        // Инициализация NTP и сервера
        timeClient.begin();
        Serial.println("Синхронизация времени началась");

        server.on("/", handle_index_page);
        server.on("/change_screen", handle_change_screen);
        server.on("/changeserver", handle_changeserver);
        server.on("/reboot", handle_reboot_page);
        Serial.print("Веб-интерфейс запущен");
        FS_init();
        Serial.println("Файловая система запущена");
        ElegantOTA.begin(&server);
        server.begin();
        Serial.println("OTA сервер запущен");
        Serial.println("HTTP сервер запущен");

        // Добавляем задачи в планировщик только после подключения к Wi-Fi
        runner.addTask(tViaBTC);
        runner.addTask(tLHM);
        tViaBTC.enable();
        tLHM.enable();

        delay(2000);
        tft.fillScreen(TFT_BLACK);
    } else {
        tft.fillScreen(TFT_BLACK);
        tft.setTextWrap(true);
        tft.println("Ошибка подключения к Wi-Fi");
    }
}

void loop() {
    // Выполняем задачи только если Wi-Fi подключен
    if (WiFi.status() == WL_CONNECTED) {
        runner.execute();
    }

    if (buttonPressed) {
        buttonPressed = false;
        tft.fillScreen(TFT_BLACK);
        screen++;
        if (screen > 4) screen = 0;
    }

    if (WiFi.getMode() == WIFI_STA) {
        timeClient.update();
        server.handleClient();
    }
}

// Задача для обновления данных ViaBTC
void getViaBTCDataTask() {
    if (WiFi.status() == WL_CONNECTED) {
        getViaBTCData();
        sendRqs();// Вызов функции из functions.ino
    }
}

// Задача для обновления данных о компьютере
void hardwareMonitorTask() {
    if (WiFi.status() == WL_CONNECTED) {
        hardwareMonitor();
        ScreenDraw();// Вызов функции из functions.ino
    }
}
