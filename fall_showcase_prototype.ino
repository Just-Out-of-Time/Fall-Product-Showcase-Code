#include <heltec.h>
#if defined(ESP32)
  #include <WiFi.h>
#endif
#include <ESP_Mail_Client.h>
#include <Adafruit_MLX90614.h>
#include <Wire.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 //or 587
#define AUTHOR_EMAIL "temperaturereadingdevice@gmail.com" //email address that is the middle man for communication
#define AUTHOR_PASSWORD "" 
#define RECIPIENT_EMAIL "@vtext.com" //you can enter a phone number and then @ some address of the persons carrier to send them a text with this method

const char ssid[]     = "WSU Guest"; 
const char password[] = ""; 

SMTPSession smtp;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void initWiFi(){ 
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  
  Serial.println("\nConnecting to: ");
  Serial.println(ssid);  

  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("\nStill connecting...");
  }
  /*
  The heltec section is for the prototype at the showcase. There is a display on the device display this information
  */
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0, "Connected to: ");
  Heltec.display->drawString(0, 40, WiFi.SSID());
  Heltec.display->display();
  
  Serial.println("\nDevice IP address: ");
  Serial.println(WiFi.localIP());
}

void sendNotification() {
  smtp.debug(1);
  //Set the callback function to get the sending results 
  smtp.callback(smtpCallback);

  //Declare the session config data
  ESP_Mail_Session session;

  // Set the session config
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  //Declare the message class
  SMTP_Message message;

  message.sender.name = "temperatureReadingDevice";
  message.sender.email = AUTHOR_EMAIL;
  String textMsg = "Temperature Out of Range";
  message.text.content = textMsg.c_str();
  //message.subject = "ESP automatic test";
  message.addRecipient("Test", RECIPIENT_EMAIL);

  if (!smtp.connect(&session))
    return;

  //Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void smtpCallback(SMTP_Status status){
  //Print the current status
  Serial.println(status.info());

  //Print the sending result
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      //Get the result item
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}

void setup() {
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 470E6 /**/);
  Wire1.begin(21, 22, 1000);
  if(!mlx.begin()) {
    Serial.println("Error Reaching MLX");
  } else {
    Serial.println("MLX setup Done");
  }
}

void loop() {
  double temp = mlx.readObjectTempF();
  Serial.println(temp);
  Serial.println("\n");
  if(temp > 100.00){
    initWiFi();
    sendNotification();
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 0, "Temp out of range: ");
    Heltec.display->drawString(0, 40, String(temp));
    Heltec.display->display();
    WiFi.disconnect();
    delay(60000);
  } else {
    Serial.println("\nTemperature within range");
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Temp: ");
    Heltec.display->drawString(0, 40, String(temp));
    Heltec.display->display();
  }
  delay(1000);
}
