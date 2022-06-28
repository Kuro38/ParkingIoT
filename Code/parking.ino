#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Chrono.h>


#define TRIGGER 5
#define ECHO    4
//#define mqtt_user "guest"  //s'il a été configuré sur Mosquitto
//#define mqtt_password "guest" //idem
#define mqtt_user "myuser"  //s'il a été configuré sur Mosquitto
#define mqtt_password "toto" //idem
//#define mqtt_user ""  //s'il a été configuré sur Mosquitto
//#define mqtt_password "" //idem
#define temperature_topic "sensor/temperature"  //Topic température

unsigned long duration;
Chrono myChrono;
bool occupied;
bool confirmOccupation;

//WIFI
const char* ssid = "DavPhone";
const char* password = "testchiante";

//MQTT
const char* mqtt_server = "192.168.97.234";//Adresse IP du Broker Mqtt
const int mqttPort = 1883; //port utilisé par le Broker 
long tps=0;

//Buffer qui permet de décoder les messages MQTT reçus
char message_buff[100];

long lastMsg = 0;   //Horodatage du dernier message publié sur MQTT
long lastRecu = 0;
bool debug = true;  //Affiche sur la console si True

//Création des objets
     
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);     //Facultatif pour le debug
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  setup_wifi();           //On se connecte au réseau wifi

  client.setServer(mqtt_server, mqttPort);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message   
  
}

//Connexion au réseau WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.print(WiFi.localIP());
}

//Reconnexion
void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    //if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      if (client.connect("ESP8266Client")) {
      Serial.println("OK");
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  long distance, valueIN, valueOUT; 
  
  //Envoi d'un message par minute
  if (now - lastMsg > 1000 * 10) {
    lastMsg = now;
    
    digitalWrite(TRIGGER, LOW);  
    delayMicroseconds(90); 
    digitalWrite(TRIGGER, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIGGER, LOW);
    duration = pulseIn(ECHO, HIGH);
    distance = (duration/2) / 29.1;

    Serial.print(distance);
    Serial.println("Centimeter:");

    /* NOUVEL ALGO */
    if( myChrono.hasPassed(5000) )
    {
      confirmOccupation = distance < 20.f;
      if (confirmOccupation)
      {
        Serial.print("occupied, need confirm\n");
        if( myChrono.hasPassed( 10000 ) )
        {
          Serial.print("occupied\n");
          occupied = true;
          client.publish(temperature_topic, String(occupied).c_str(), true); 
          myChrono.restart();
        }
      }
      else if(!confirmOccupation)
      {
        if(myChrono.hasPassed( 10000 ) )
        {
          Serial.print("free\n");
          occupied = false;
          client.publish(temperature_topic, String(occupied).c_str(), true); 
          myChrono.restart();
        }
      }
    }
/*    
 *     ANCIEN ALGO
 */
    if (myChrono.elapsed() <= 20) // returns true if it passed 1000 ms since it was started
    {
      duration = myChrono.elapsed();
      distance = (duration/2) / 29.1;
      Serial.println("premier if");
      //Serial.println(myChrono.elapsed());
      Serial.println(duration);
      Serial.println(distance);
    }
    else if (myChrono.elapsed() >= 10000 && distance < 100)
    {
      //duration = 0;
      //duration = pulseIn(ECHO, LOW);
      duration = myChrono.elapsed();
      distance = (duration/2) / 29.1;
     occupied = true;
     client.publish(temperature_topic, String(occupied).c_str(), true);   //Publie la température sur le topic temperature_topic
      Serial.print("Occupé");
      myChrono.restart();
      //Serial.println("pulseIn LOW");
      Serial.println("premier elseif");
      Serial.println(duration);
      Serial.println(distance);
    }
    else
    {
        duration = myChrono.elapsed();
        distance = (duration/2) / 29.1;
        Serial.println(duration);
        Serial.println(distance);

     occupied = false;
     client.publish(temperature_topic, String(occupied).c_str(), false);   //Publie la température sur le topic temperature_topic
     Serial.print("Libre");
     Serial.println("dernier else :");
     Serial.println(myChrono.elapsed());
      Serial.println(duration);
    } 
    
    /*if (now - lastRecu > 100 ) {
      lastRecu = now;
      client.subscribe("homeassistant/switch1");
    }*/
  }
}

// Déclenche les actions à la réception d'un message
// D'après http://m2mio.tumblr.com/post/30048662088/a-simple-example-arduino-mqtt-m2mio
void callback(char* topic, byte* payload, unsigned int length) {

  int i = 0;
  if ( debug ) {
    Serial.println("Message recu =>  topic: " + String(topic));
    Serial.print(" | longueur: " + String(length,DEC));
  }
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  if ( debug ) {
    Serial.println("Payload: " + msgString);
  }
  
  if ( msgString == "ON" ) {
    digitalWrite(BUILTIN_LED,HIGH);  
  } else {
    digitalWrite(BUILTIN_LED,LOW);  
  }
}
