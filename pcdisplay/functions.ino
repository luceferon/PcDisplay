#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Добавляем переменные для хранения данных от ViaBTC API
String viaBTCAPIKey = "Замените на ваш API ключ"; // Замените на ваш API ключ
String viaBTCSecretKey = "Замените на ваш секретный ключ"; // Замените на ваш секретный ключ
String viaBTCAccountInfoURL = "https://www.viabtc.com/res/openapi/v1/account";
String viaBTCAccountHashrateURL = "https://www.viabtc.com/res/openapi/hashrate";
String viaBTCProfitSummaryURL = "https://www.viabtc.com/res/openapi/profit";
// URL для получения курса BTC к рублю (CoinGecko API)
String coinGeckoURL = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=rub";

WiFiClientSecure client; // Используем WiFiClientSecure для HTTPS

// Структуры для хранения данных от ViaBTC API
struct AccountInfo {
  String balance;         // Баланс BTC
  String balanceInRubles; // Баланс в рублях
  String pending_balance;
};

struct AccountHashrate {
  String hashrate;
};

struct ProfitSummary {
  String profit;
};

AccountInfo accountInfo;
AccountHashrate accountHashrate;
ProfitSummary profitSummary;

// Функция для получения курса BTC к рублю
void getBTCRubRate() {
    WiFiClientSecure client; // Используем WiFiClientSecure для HTTPS
    client.setInsecure(); // Игнорируем проверку сертификата (для упрощения)

    HTTPClient http;
    http.begin(client, coinGeckoURL); // Используем HTTPS
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("CoinGecko API Response: " + payload);

        // Парсим JSON-ответ
        DynamicJsonDocument doc(200);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Получаем курс BTC к рублю
        float btcRubRate = doc["bitcoin"]["rub"];
        Serial.println("BTC to RUB rate: " + String(btcRubRate));

        // Вычисляем баланс в рублях
        if (accountInfo.balance != "") {
            float btcBalance = accountInfo.balance.toFloat();
            float rubBalance = btcBalance * btcRubRate;
            accountInfo.balanceInRubles = String(rubBalance, 2); // Округляем до 2 знаков после запятой
            Serial.println("Balance in RUB: " + accountInfo.balanceInRubles);
        }
    } else {
        Serial.println("Connection to CoinGecko API failed");
    }

    http.end();
}

// Функция для получения данных от ViaBTC API
void getViaBTCData() {
    client.setInsecure(); // Игнорируем проверку сертификата (для упрощения)
    String payload = "";

    // Запрос баланса
    Serial.println("Connecting to ViaBTC API for balance...");
    if (client.connect("www.viabtc.com", 443)) {
        Serial.println("Connected to ViaBTC API");

        // Формируем HTTP-запрос для получения баланса
        client.println("GET /res/openapi/v1/account HTTP/1.1");
        client.println("Host: www.viabtc.com");
        client.println("X-API-KEY: " + viaBTCAPIKey);
        client.println("Connection: close");
        client.println();

        // Читаем заголовки ответа
        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                break; // Конец заголовков
            }
        }

        // Читаем тело ответа (JSON)
        while (client.available()) {
            payload += client.readString();
        }

        // Удаляем HTTP-заголовки из ответа
        int jsonStart = payload.indexOf("{");
        if (jsonStart >= 0) {
            payload = payload.substring(jsonStart);
        }

        Serial.println("ViaBTC API Response (Balance): " + payload);

        // Парсим JSON-ответ
        DynamicJsonDocument doc(1024); // Увеличиваем размер документа
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Обрабатываем данные о балансе
        if (doc.containsKey("data") && doc["data"].containsKey("balance")) {
            JsonArray balances = doc["data"]["balance"];
            for (JsonObject balance : balances) {
                String coin = balance["coin"].as<String>();
                String amount = balance["amount"].as<String>();
                Serial.println("Coin: " + coin + ", Amount: " + amount);

                // Сохраняем баланс BTC
                if (coin == "BTC") {
                    accountInfo.balance = amount;
                    Serial.println("BTC Balance saved: " + accountInfo.balance);
                }
            }
        } else {
            Serial.println("Invalid JSON structure: 'data' or 'balance' key not found");
        }
    } else {
        Serial.println("Connection to ViaBTC API for balance failed");
    }

    client.stop();

    // Получаем курс BTC к рублю и вычисляем баланс в рублях
    getBTCRubRate();

    // Запрос хэшрейта
    payload = "";
    Serial.println("Connecting to ViaBTC API for hashrate...");
    if (client.connect("www.viabtc.com", 443)) {
        Serial.println("Connected to ViaBTC API");

        // Формируем HTTP-запрос для получения хэшрейта
        client.println("GET /res/openapi/v1/hashrate?coin=BTC HTTP/1.1");
        client.println("Host: www.viabtc.com");
        client.println("X-API-KEY: " + viaBTCAPIKey);
        client.println("Connection: close");
        client.println();

        // Читаем заголовки ответа
        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                break; // Конец заголовков
            }
        }

        // Читаем тело ответа (JSON)
        while (client.available()) {
            payload += client.readString();
        }

        // Удаляем HTTP-заголовки из ответа
        int jsonStart = payload.indexOf("{");
        if (jsonStart >= 0) {
            payload = payload.substring(jsonStart);
        }

        Serial.println("ViaBTC API Response (Hashrate): " + payload);

        // Парсим JSON-ответ
        DynamicJsonDocument doc(1024); // Увеличиваем размер документа
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Обрабатываем данные о хэшрейте
        if (doc.containsKey("data") && doc["data"].containsKey("hashrate_10min")) {
            String fullHashrate = doc["data"]["hashrate_10min"].as<String>();
            // Обрезаем хешрейт до первых двух цифр и добавляем "TH/s"
            String shortHashrate = fullHashrate.substring(0, 2) + " TH/s";
            accountHashrate.hashrate = shortHashrate;
            Serial.println("Hashrate (10min): " + accountHashrate.hashrate);
        } else {
            Serial.println("Invalid JSON structure: 'hashrate_10min' key not found");
        }
    } else {
        Serial.println("Connection to ViaBTC API for hashrate failed");
    }

    Serial.println("Free heap: " + String(ESP.getFreeHeap()));
    client.stop();
}

//здесь содержатся основные функции прошивки
//генерация web интерфейса для настройки параметров подключения
void handle_index_page(){
   String webpage;
  webpage="<html lang='ru'>";
  webpage+="<head>";
  webpage+="<meta http-equiv='Content-type' content='text/html; charset=utf-8'>";
  webpage+="<link rel='stylesheet' href='/bootstrap.min.css'>";
  webpage+="<link rel='stylesheet' type='text/css' href='/style.css'>";
  webpage+="<script type='text/javascript' src='/function.js'></script>";
  webpage+="<link rel='icon' href='/favicon.ico'>"; // Добавленная строка для отображения иконки в web адресе
  webpage+="<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  webpage+="<title>Настройки</title>";
  webpage+="<script type='text/javascript'>";
  webpage+="function data_server(submit){";
  webpage+="server = '/changeserver?adress='+val('dataserver');";
  webpage+="send_request(submit,server);";
  webpage+="}";
  webpage+="function reboot(submit){";
  webpage+="send_request(submit,'/reboot');";
  webpage+="}";
  webpage+="function update_firmware(submit){";
  webpage+="window.location.href = '/update.htm';";
  webpage+="}";
  webpage+="function open_edit(submit){";
  webpage+="window.location.href = '/edit';";
  webpage+="}";
  webpage += "function change_screen(submit){";
  webpage += "send_request(submit,'/change_screen');";
  webpage += "}";
  webpage+="</script>";
  webpage+="</head>";
  webpage+="<body>";
  webpage+="<div class='container'>";
  webpage+="<div class='row' style='text-align:center;'>";
  webpage+="<h1 style='margin:50px;'>Настройки</h1>";
  webpage+="<div class='col-sm-offset-2 col-sm-8 col-md-offset-3 col-md-6'>";
  webpage+="<h2>Адрес WEB интерфейса Libre Hardware Monitor:</h2>";
  webpage+="<div class='alert alert-dismissible alert-info'>";
  webpage+="<input id='dataserver' value='"+dataServer+"' class='form-control'>";
  webpage+="<input class='btn btn-block btn-success' value='Сохранить' onclick='data_server(this);' type='submit'></div>";
  webpage+="<input class='btn btn-block btn-success' value='Сменить экран' onclick='change_screen(this);' type='submit'>"; // Добавленная кнопка для смены экрана
  webpage+="<input class='btn btn-block btn-primary' value='Редактор' onclick='open_edit(this);' type='submit'>"; // Добавленная кнопка для перехода в редактор
  webpage+="<input class='btn btn-block btn-danger' value='Перезагрузка' onclick='reboot(this);' type='submit'>"; // Добавленная кнопка для перезагрузки
  webpage+="<input class='btn btn-block btn-primary' value='Обновить прошивку' onclick='update_firmware(this);' type='submit'>"; // Добавленная кнопка для обновления прошивки
  webpage+="</div></div></div></div>";
  webpage+="</body>";
  webpage+="</html>";
  server.send(200, "text/html", webpage);
}

//фунция сохранения настроек подключения к LHM через WEB интерфейс
void handle_changeserver() {               
  File myFile;
  dataServer = server.arg("adress");
  server.send(200, "text/plain", "OK"); 
   myFile = SPIFFS.open("/server.txt", "w");
  myFile.print(dataServer);
  myFile.close();
}

 //функция вывода времени полученного по NTP в формате HH:MM
 String printTime(){
 byte h,m;
 String result;
 h=timeClient.getHours();
 if (h<10) result+="0";
 result+=String(h)+":";
 m=timeClient.getMinutes();
  if (m<10) result+="0";
 result+=String(m);
 return (result); 
 }
 
 //Самая жирная (и самая тормозная по работе) функция парсинга JSON Libre Hardware Monitor
 void hardwareMonitor()
{
  
  WiFiClient client;
  HTTPClient http;
  // Send request
  http.useHTTP10(true);
  http.begin(client, dataServer);
//  Serial.println("http connecting...");
//  Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      //Здесь проверяем доступен ли воообще JSON по указанному нами адресу
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
         // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
          //если доступен после длительного перерыва снижаем интервал опроса датчиков, сбрасываем счетчик и переходим на экран отображения датчиков
          if (syncerror>6 && screen==4){
            sync_interval=1000;
            refsens.setInterval(sync_interval);
            tft.fillScreen(TFT_BLACK);
            screen=0;
          }
          syncerror=0;
        }
      } else {
        Serial.printf("sensors http sync failed, error: %s\n", http.errorToString(httpCode).c_str());
        //Здесь если после 40 запросов сервер Libre Hardware Monitor не дал ответ, то уменьшаем интервал между запросами, и переключаемся на экран отображения погоды
        if (syncerror<6) {
          syncerror++;
          delay(200);
        }
        if (syncerror==6){
          sync_interval=30000;
          refsens.setInterval(sync_interval);
          syncerror++;
          if (screen!=4){
            tft.fillScreen(TFT_BLACK);
          screen=4;
          }
        }
        return;
      }

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  
  http.GET();
 StaticJsonDocument<224> filter;

JsonObject filter_Children_0_Children_0 = filter["Children"][0]["Children"].createNestedObject();
filter_Children_0_Children_0["Text"] = true;

JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"][0]["Children"].createNestedObject();
filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;
filter_Children_0_Children_0_Children_0_Children_0["Children"][0]["Value"] = true;
  // непосредственно парсинг JSON. Файл очень большой, памяти занимает много, парсинг занимает немало времени
  //const size_t capacity = 89 * JSON_ARRAY_SIZE(0) + 11 * JSON_ARRAY_SIZE(1) + 6 * JSON_ARRAY_SIZE(2) + 4 * JSON_ARRAY_SIZE(3) + 3 * JSON_ARRAY_SIZE(4) + 2 * JSON_ARRAY_SIZE(5) + JSON_ARRAY_SIZE(6) + JSON_ARRAY_SIZE(7) + 2 * JSON_ARRAY_SIZE(8) + 4 * JSON_ARRAY_SIZE(9) + 4 * JSON_ARRAY_SIZE(10)+ 4 * JSON_ARRAY_SIZE(11)+ 4 * JSON_ARRAY_SIZE(12)+ 4 * JSON_ARRAY_SIZE(13) + 123 * JSON_OBJECT_SIZE(7) + 8530;
  DynamicJsonDocument doc(17000);
  deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));
   /*структура файла довольно таки большая и сложная, со множеством вложенных списков и ветвлений, но в целом разобраться можно
    ["Children"][0]["Children"][1] - разлчиные параметры процессора
    ["Children"][0]["Children"][3] - видеокарта
    ["Children"][0]["Children"][2] - память
    ["Children"][0]["Children"][0] - материнская плата
   */
   String cpuName = doc["Children"][0]["Children"][1]["Text"]; // название процессора
   String cpuTempPackage = doc["Children"][0]["Children"][1]["Children"][3]["Children"][6]["Value"];  // температура процессора
   String cpuLoad = doc["Children"][0]["Children"][1]["Children"][4]["Children"][0]["Value"];  // загрузка процессора
   String cpuFAN= doc["Children"][0]["Children"][0]["Children"][0]["Children"][3]["Children"][1]["Value"];  // скорость вент процессора
   String gpuName = doc["Children"][0]["Children"][3]["Text"];  //название видеокарты
   String gpuTemp = doc["Children"][0]["Children"][3]["Children"][2]["Children"][0]["Value"];  // температура видеокарты
   String gpuLoad = doc["Children"][0]["Children"][3]["Children"][3]["Children"][0]["Value"];  // загрузка видеокарты
   String gpuFAN = doc["Children"][0]["Children"][3]["Children"][5]["Children"][0]["Value"];  // скорость вент видеокарты
  String loadRAM = doc["Children"][0]["Children"][2]["Children"][0]["Children"][0]["Value"];  // загрузка пямяти
  String usedRAM = doc["Children"][0]["Children"][2]["Children"][1]["Children"][0]["Value"];  // использовано памяти
  String gpuRAM = doc["Children"][0]["Children"][3]["Children"][3]["Children"][3]["Value"];  // загрузка памяти видеокарты
  String gpuRAMused = doc["Children"][0]["Children"][3]["Children"][6]["Children"][1]["Value"];  // использовано памяти видеокарты 
  String m2Temp = doc["Children"][0]["Children"][5]["Children"][0]["Children"][0]["Value"];  // температура m2
  String m2Load = doc["Children"][0]["Children"][5]["Children"][1]["Children"][0]["Value"];  // загрузка m2
  String hddTemp = doc["Children"][0]["Children"][4]["Children"][1]["Children"][3]["Value"];  // активность hdd
  String hddLoad = doc["Children"][0]["Children"][4]["Children"][1]["Children"][0]["Value"];  // место hdd
  String ethUpload = doc["Children"][0]["Children"][9]["Children"][2]["Children"][0]["Value"];  // загрузка eth
  String ethDown = doc["Children"][0]["Children"][9]["Children"][2]["Children"][1]["Value"];  // загрузка eth
  String hdd1Temp = doc["Children"][0]["Children"][7]["Children"][1]["Children"][3]["Value"];  // активность hdd
  String hdd1Load = doc["Children"][0]["Children"][7]["Children"][1]["Children"][0]["Value"];  // место hdd

  
  //String degree = degree.substring(degree.length()) + "°C";
  //String percentage = percentage.substring(percentage.length()) + (char)37;
  //здесь строим графики
  if (cpu_index<99){
    cpu_temp[cpu_index]=cpuTempPackage.toInt();
    cpu_index++;
  }
  if (cpu_index==99){
    cpu_temp[cpu_index]=cpuTempPackage.toInt();
    for (byte i=1; i<100; i++){
      cpu_temp[i-1]=cpu_temp[i];
  }
  }
   
   if (gpu_index<99){
    gpu_temp[gpu_index]=gpuTemp.toInt();
    gpu_index++;
  }
  if (gpu_index==99){
    gpu_temp[gpu_index]=gpuTemp.toInt();
    for (byte i=1; i<100; i++){
      gpu_temp[i-1]=gpu_temp[i];
  }
  }
  if (screen==0){
   tft.setTextColor(0x0E3F, TFT_BLACK);
   tft.setCursor(10, 65);
      tft.setTextSize(3);
        tft.print("Temp:");
      tft.print(cpuTempPackage.substring(0, cpuTempPackage.length() - 6));
  
      tft.print(degree+"  ");

       // CPU - Load
      tft.setTextSize(3);
      tft.setCursor(10, 95);
      tft.print("Load:");
      tft.print(cpuLoad.substring(0, cpuLoad.length() - 4));
      tft.setTextSize(3);
      tft.print(percentage+"  ");
      
         // CPU fan
      tft.setTextSize(3);
      tft.setCursor(10, 125);
      tft.print("Fan:");
      tft.print(cpuFAN);
      tft.setTextSize(3);
      
      //GPU
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(250, 65);
      tft.setTextSize(3);
      tft.print("Temp:");
      tft.print(gpuTemp.substring(0, gpuTemp.length() - 6));
      tft.setTextSize(3);
      tft.print(degree+"  ");

      // GPU - Load
      tft.setTextSize(3);
      tft.setCursor(250, 95);
      tft.print("Load:");
      tft.print(gpuLoad.substring(0, gpuLoad.length() - 4));
      tft.setTextSize(3);
      tft.print(percentage+"  ");
      
      //gpuFAN
      tft.setTextSize(3);
      tft.setCursor(250, 125);
      tft.print("Fan:");
      tft.print(gpuFAN);

         //ram
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.setTextSize(3);
      tft.setCursor(10, 222);
      tft.println("Load:" + loadRAM);

      // RAM
      tft.setTextSize(3);
      tft.setCursor(10, 267);
      tft.println("U:" + usedRAM);

      //gpu ram
      tft.setTextSize(3);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(250, 222);
      tft.setTextSize(3);
      tft.print("RAM:");
      tft.print(gpuRAM);
      tft.setTextSize(3);

      // GPU - Load
      tft.setTextSize(3);
      tft.setCursor(250, 267);
      tft.print("U:");
      tft.print(gpuRAMused);

        }
  if (screen==1){
     //m2
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 65);
      tft.setTextSize(3);
      tft.print("Temp:" + m2Temp);

      // m2 - Load
      tft.setTextSize(3);
      tft.setCursor(10, 105);
      tft.print("Used:" + m2Load);

     //hdd
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setCursor(250, 65);
      tft.setTextSize(3);
      tft.print("Act:" + hddTemp);

      // hdd - Load
      tft.setTextSize(3);
      tft.setCursor(250, 105);
      tft.print("Load:" + hddLoad);

      // eth - Upload
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      tft.setTextSize(3);
      tft.setCursor(10, 222);
      tft.print("Up:" + ethUpload);
     
      // eth - Down  
      tft.setTextSize(3);
      tft.setCursor(10, 267);
      tft.print("D:" + ethDown);  
      
     //hdd1
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setCursor(250, 222);
      tft.setTextSize(3);
      tft.print("Act:" + hdd1Temp);

      // hdd1 - Load
      tft.setTextSize(3);
      tft.setCursor(250, 267);
      tft.print("Load:" + hdd1Load);
  }

  if (screen==3){
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
     tft.setCursor(100,23);
      tft.print(gpuName);
  }
  }
  http.end();
  if (screen>4) {sync_interval=30000;} else { if (syncerror<6) sync_interval=1000;} 
  refsens.setInterval(sync_interval);
  Serial.println(String(ESP.getFreeHeap()));
}
//функция рисования содержимого экрана
void ScreenDraw(){
  
  uint16_t color;
  byte min_temp,max_temp,cur_temp; 
  //Time
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(210,0);
  tft.setTextSize(2);
  if (screen!=4) tft.print(printTime());
  tft.setTextColor(0x0E3F, TFT_BLACK);
  switch (screen){
    
 case 0:      
       //CPU 
      tft.drawRoundRect (0, 16, 238, 148, 7, 0x0E3F);
      tft.setCursor(20,25);
      tft.setTextColor(0x0E3F, TFT_BLACK);
      tft.setTextSize(3);
      tft.print("CPU");
      
      //GPU - Temperature
      tft.drawRoundRect (240, 16, 238, 148, 7, TFT_GREEN);
      tft.setTextSize(3);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(260, 25);
      tft.print("GPU");
      
      // RAM
      tft.drawRoundRect (0, 167, 238, 148, 7, TFT_ORANGE);
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.setTextSize(3);
      tft.setCursor(20, 177);
      tft.print("RAM");
      
      //GPU - RAM
      tft.drawRoundRect (240, 167, 238, 148, 7, TFT_YELLOW);
      tft.setTextSize(3);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(260, 177);
      tft.print("GPU RAM");
      
 break;
 case 1:      
      //m2 
      tft.drawRoundRect (0, 16, 238, 148, 7, TFT_CYAN);
      tft.setCursor(20,25);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setTextSize(3);
      tft.print("M2");
      
      //HDD
      tft.drawRoundRect (240, 16, 238, 148, 7, TFT_GREENYELLOW);
      tft.setTextSize(3);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setCursor(260, 25);
      tft.print("HDD");
      
      // Ethernet
      tft.drawRoundRect (0, 167, 238, 148, 7, TFT_GOLD);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      tft.setTextSize(3);
      tft.setCursor(20, 177);
      tft.print("Ethernet");
      
      //
      tft.drawRoundRect (240, 167, 238, 148, 7, TFT_SILVER);
      tft.setTextSize(3);
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
      tft.setCursor(260, 177);
      tft.print("SSD");
      
 break;
 case 2:
      // Отображение информации о балансе и хэшрейте
            tft.drawRoundRect(0, 16, 238, 148, 7, TFT_BLUE);
            tft.setCursor(20, 25);
            tft.setTextColor(TFT_BLUE, TFT_BLACK);
            tft.setTextSize(3);
            tft.print("Balance");

            tft.setCursor(10, 50);
            tft.setTextSize(2);
            tft.print("BTC: " + accountInfo.balance);

            tft.setCursor(10, 70);
            tft.print("RUB: " + accountInfo.balanceInRubles);

            tft.drawRoundRect(240, 16, 238, 148, 7, TFT_RED);
            tft.setCursor(260, 25);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.setTextSize(3);
            tft.print("Hashrate");

            tft.setCursor(250, 50);
            tft.setTextSize(3);
            tft.print(accountHashrate.hashrate);
            break;
 case 3:
      tft.setTextWrap(false);
      tft.setCursor(7,55);
      tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.print("100");
      tft.setCursor(20,280);
      tft.print("0");
      tft.startWrite();
      tft.drawFastVLine(50,45,265,TFT_WHITE);
      tft.drawFastHLine(50,310,360,TFT_WHITE);
      tft.drawFastHLine(0,45,480,TFT_WHITE);
      tft.drawFastVLine(410,45,265,TFT_WHITE);
      min_temp=gpu_temp[gpu_index-1];
      max_temp=gpu_temp[0];
      cur_temp=gpu_temp[gpu_index-1];
      for (byte i=0;i<gpu_index;i++){
      tft.drawFastVLine(i+55,50,295-gpu_temp[i]*2.5,TFT_BLACK);
      if (gpu_temp[i]<50) color=TFT_GREEN;
      if (gpu_temp[i]>49 and gpu_temp[i]<65) color=TFT_YELLOW;
      if (gpu_temp[i]>64)  color=TFT_RED;
      tft.drawFastVLine(i+55,305-gpu_temp[i]*2.5,gpu_temp[i]*2.5,color);
      if (gpu_temp[i]<=min_temp) min_temp=gpu_temp[i];
      if (gpu_temp[i]>max_temp) max_temp=gpu_temp[i];  
      }
      tft.endWrite();
      tft.setCursor(430,55);
       tft.setTextColor(TFT_RED,TFT_BLACK);
      tft.print("MAX");
      tft.setCursor(435,85);
      tft.print(max_temp);
      tft.setCursor(430,135);
       tft.setTextColor(TFT_GREEN,TFT_BLACK);
      tft.print("MIN");
      tft.setCursor(435,165);
      tft.print(min_temp);
      tft.setCursor(430,215);
       tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.print("CUR");
      tft.setCursor(435,245);
      tft.print(cur_temp);
 break;
 case 4:
      tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.setTextSize(7);
      tft.setCursor(150,5);
      tft.print(printTime());
      draw_weather();
 break;
 }  
}

//обновление погоды
void sendRqs(){
   WiFiClient client;
  HTTPClient http;
  http.begin(client,api_1 + qLocation + api_2 + api_3 + api_key);
  int httpCode = http.GET();

  if (httpCode > 0) { 

    String response = http.getString();
    
    Serial.println(response);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(owm, response);

    // Test if parsing succeeds.
    if (error) {
      String errorStr = error.c_str();
      Serial.print ("weather parse: ");
      Serial.println(errorStr);
    }else{
      Serial.println("weathersync no error.");
    }
    }else{
    Serial.println("http.GET() == 0");
  }
  
  http.end();   //Close connection
}
//вывод погоды на экран
void draw_weather(){
      //--- Copy from ArduinoJson Assistant
      float coord_lon = owm["coord"]["lon"];
      float coord_lat = owm["coord"]["lat"];
      uint16_t color;
      JsonObject weather_0 = owm["weather"][0];
      int weather_0_id = weather_0["id"];
      String weather_0_main = weather_0["main"];
      const char* weather_0_description = weather_0["description"];
      const char* weather_0_icon = weather_0["icon"];

      const char* base = owm["base"];

      JsonObject main = owm["main"];
      int main_temp = main["temp"];
      int main_temp_min = main["temp_min"]; // -8.9
      int main_temp_max = main["temp_max"]; // -8.9
      int main_feels_like = main["feels_like"];
      int main_pressure = main["pressure"];
      int main_pressure_mm = main_pressure/1.333;
      int main_humidity = main["humidity"];

      int visibility = owm["visibility"];

      float wind_speed = owm["wind"]["speed"];
      int wind_deg = owm["wind"]["deg"];

      int clouds_all = owm["clouds"]["all"];

      long dt = owm["dt"];

      JsonObject sys = owm["sys"];
      int sys_type = sys["type"];
      int sys_id = sys["id"];
      const char* sys_country = sys["country"];
      long sys_sunrise = sys["sunrise"];
      long sys_sunset = sys["sunset"];

      int timezone = owm["timezone"];
      long id = owm["id"];
      const char* name = owm["name"];
      int cod = owm["cod"];

      //--- End of ArduinoJson Assistant ---

      // Print values.
       tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(160,70);
      tft.print(String(name)+", "+String(sys_country));
      if(weather_0_main=="Clouds")  // 'Облачно', 112x112px
        {
          tft.drawBitmap(20, 55, clouds, 112, 112,0xFFE0,TFT_BLACK);
          }
          
          else if(weather_0_main=="Rain") // 'Дождь', 112x112px
        {
         tft.drawBitmap(20, 55, rain, 112, 112,0x07FF,TFT_BLACK);
          }
          
          else if(weather_0_main=="Thunderstorm")// 'Гроза', 112x90px
        {
          tft.drawBitmap(20, 55, thunder, 112, 90,TFT_DARKGREY,TFT_BLACK);
          }
          
          else if(weather_0_main=="Clear")  // 'Ясно', 112x112px
        {
          tft.drawBitmap(20, 55, clearS1, 112, 112,0xFFE0,TFT_BLACK);
          }
          
          else if(weather_0_main=="Drizzle")// 'Морось', 112x112px
        {
          tft.drawBitmap(20, 55, drizzle, 112, 112,0x07FF,TFT_BLACK);
          }
          
          else if(weather_0_main=="Snow")// 'Снег', 112x112px
        {
          tft.drawBitmap(20, 55, snow, 112, 112,TFT_WHITE,TFT_BLACK);
          }
          
          else
        {
          tft.drawBitmap(20, 55, mist, 112, 112,TFT_LIGHTGREY,TFT_BLACK);// 'Туман', 112x112px
          }
           if (main_temp>=5) color=0xF705;
           if (main_temp>20) color=0xF005;
           if (main_temp<5) color=0x0E3F;
           if (main_temp<=-10) color=TFT_RED;

           tft.setCursor(220,160);
           tft.setTextSize(2);
           tft.setTextColor(color,TFT_BLACK);
       tft.print ("Ощущается:"+String(main_feels_like)+degree);
           
           tft.drawBitmap(210, 102, temp, 48, 48, color);// 'Термо', 48x48px
           tft.setCursor(270,105);
           tft.setTextSize(6);
           tft.setTextColor(color,TFT_BLACK);
       tft.print (String(main_temp)+degree);
       tft.setCursor(30,200); 
           tft.setTextSize(2);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);
        tft.print (weather_0_description);
        tft.print("        ");
        tft.setCursor(205,180);
        tft.setTextColor(color,TFT_BLACK);
        tft.println("Мин:"+String(main_temp_min)+degree+" Макс:"+String(main_temp_max)+degree);
        tft.setTextSize(2);
        tft.drawBitmap(30, 220, pressure, 68, 68,0x6515);    // давление, 68x68px
       tft.setCursor(35,300);
       tft.setTextColor(TFT_WHITE,TFT_BLACK);
        tft.println (String(main_pressure_mm)+"мм");
       tft.drawBitmap(140, 220, humidity, 68, 68,0xAF1F);  // влажность, 68x68px
       tft.setCursor(160,300);
       tft.println(String(main_humidity)+"%");
       tft.drawBitmap(260, 220, wind, 68, 68,TFT_WHITE);  // ветер, 68x68px
       tft.setCursor(255,300);
      tft.println (String(wind_speed)+"м/с");
       int x2=(420+(sin(wind_deg/57.29577951)*(30)));
       int y2=(250-(cos(wind_deg/57.29577951)*(30)));
      tft.drawCircle(420,250,30,TFT_WHITE);
      tft.fillCircle(420,250,28,TFT_BLACK);
      tft.drawLine(420,250,x2,y2,TFT_CYAN);
      tft.fillCircle(x2,y2,3,TFT_WHITE);
      
      tft.setCursor(412,300);
  if (wind_deg > 337.5 || wind_deg < 22.5) {
    tft.print("С");
  } else if (wind_deg < 67.5) {
    tft.print("С/В");
  } else if (wind_deg < 112.5) {
    tft.print("В");
  } else if (wind_deg < 157.5) {
    tft.print("Ю/В");
  } else if (wind_deg < 202.5) {
    tft.print("Ю");
  } else if (wind_deg < 247.5) {
    tft.print("Ю/З");
  } else if (wind_deg < 292.5) {
    tft.print("З");
  } else if (wind_deg < 337.5) {
    tft.print("С/З");
  }
      
    }
