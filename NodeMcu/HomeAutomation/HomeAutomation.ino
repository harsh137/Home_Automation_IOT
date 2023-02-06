
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <WiFiManager.h>   


// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// 1. Define the WiFi credentials 
//#define WIFI_SSID "Omphh!!"
//#define WIFI_PASSWORD "harsh137"

// For the following credentials, see examples /Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

// 2. Define the API Key 
#define API_KEY "AIzaSyBlD01zoxb7Y81fRD1IU4HFBoAcuYr2gxE"

// 3. Define the RTDB URL 
#define DATABASE_URL "home-automation137-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

// 4. Define the user Email and password that alreadey registerd or added in your project 
#define USER_EMAIL "harsh@gmail.com"
#define USER_PASSWORD "harsh137"
String uid;
String change;

// Define Firebase Data object
FirebaseData stream;
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
WiFiManager wifiManager;
 
unsigned long sendDataPrevMillis = 0;

int L[4]={-1,-1,-1,-1};
String P[4]={};
String p1;

volatile bool dataChanged = false;

void FirstSetlightStatus()
{
  L[0]=Firebase.getInt(fbdo , P[0])? fbdo.to<int>() : 2;
  L[1]=Firebase.getInt(fbdo , P[1])? fbdo.to<int>() : 2;
  L[2]=Firebase.getInt(fbdo , P[2])? fbdo.to<int>() : 2;
  L[3]=Firebase.getInt(fbdo , P[3])? fbdo.to<int>() : 2;
}


void streamCallback(StreamData data)
{
  Serial.printf("\nstream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());

                change=data.dataPath().c_str();
//                printResult(data); // see addons/RTDBHelper.h
                  
  Serial.println();

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
        //Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here especially starting connect to server.
  // Just set this flag and check it status later.
  dataChanged = true;
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void setup()
{

  Serial.begin(115200);


//  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//
//  Serial.print("Connecting to Wi-Fi");
//  unsigned long ms = millis();
//  while (WiFi.status() != WL_CONNECTED)
//  {
//    Serial.print(".");
//    delay(300);
//
//  }
//  Serial.println();
//  Serial.print("Connected with IP: ");
//  Serial.println(WiFi.localIP());
//  Serial.println();

    
    wifiManager.autoConnect("Node MCU", "harsh137");

    while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);

  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
    

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.


  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino
  
  while(!Firebase.ready()){
  Firebase.begin(&config, &auth);
  Serial.print(".") ;
  delay(2000); 
  }
  Serial.println();

  Firebase.reconnectWiFi(true);

// Recommend for ESP8266 stream, adjust the buffer size to match your stream data size
  stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
   uid=auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);
  Serial.println();
   p1="/"+uid+"/LED/data/json";
   
  Serial.print(p1);
  Serial.println();
  P[0]=p1+"/LED-1";
  P[1]=p1+"/LED-2";
  P[2]=p1+"/LED-3";
  P[3]=p1+"/LED-4";

     //first light status set
  FirstSetlightStatus();


  
  
    
  if (!Firebase.beginStream(stream, p1))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.setStreamCallback(stream, streamCallback, streamTimeoutCallback);

}

void loop()
{

  // Firebase.ready() should be called repeatedly to handle authentication tasks.


  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    
    sendDataPrevMillis = millis();
                  
                  FirebaseJson json;
                  json.add("LED-1", L[0]);
                  json.add("LED-2", L[1]);
                  json.add("LED-3", L[2]);
                  json.add("LED-4", L[3]);
                  
                  Serial.printf("Set json... %s\n\n", Firebase.setJSON(fbdo,p1 , json) ? "ok" : fbdo.errorReason().c_str());
                  
  }

  if (dataChanged)
  {
    dataChanged = false;
    
    // When stream data is available, do anything here...
      if(change=="/")  {
         L[0]=Firebase.getInt(fbdo , P[0])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-1 STatus... %d\n",L[0]);
    
           L[1]=Firebase.getInt(fbdo , P[1])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-2 STatus... %d\n",L[1]);
      
           L[2]=Firebase.getInt(fbdo , P[2])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-3 STatus... %d\n",L[2]);
          
           L[3]=Firebase.getInt(fbdo , P[3])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-4 STatus... %d\n",L[3]);
          }
          
        else if(change=="/LED-1"){
          L[0]=Firebase.getInt(fbdo , P[0])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-1 STatus... %d\n",L[0]);
          }
          
        else if(change=="/LED-2"){
           L[1]=Firebase.getInt(fbdo , P[1])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-2 STatus... %d\n",L[1]);
          }
          
        else if(change=="/LED-3"){
           L[2]=Firebase.getInt(fbdo , P[2])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-3 STatus... %d\n",L[2]);
          }
          
        else if(change=="/LED-4"){
          L[3]=Firebase.getInt(fbdo , P[3])? fbdo.to<int>() : 2;
          Serial.printf("GET LED-4 STatus... %d\n",L[3]);
        }
    
  }
}
