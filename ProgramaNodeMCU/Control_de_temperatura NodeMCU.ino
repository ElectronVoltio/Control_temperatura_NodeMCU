// Including the ESP8266 WiFi library
#include <ESP8266WiFi.h>
//#include <DHT.h>
#include <EEPROM.h>

// Connexió a xarxa privada
const char* ssid = "*****************"; // Nombre de la red WIFI
const char* password = "*************"; // Contraseña

WiFiServer server(80);

// Variables temporals
static char celsiusTemp[7];
static char consignaString[7];
int cons;
IPAddress ip_xarxa;

bool error = false;
float t;
int Pin_cale = 10;                   //Pin activa la calefacció
int Pin_act = 5;                    //Pin que indica que la calefacció está en mode automàtic
int Pin_man = 4;                    //Pin per posar la calefacció en automàtic o manual
int Pin_activar = 16;               //Pin per activar el control automatic de la calefacció
float consigna = 20.0;             //Valor de consigna de la temperatura en graus Celsius
bool cale;
bool operacio = false;
bool man = false;
//bool histeresis = false;
int entero;
int Ft;
int Rt;
float tt;
int lecturaSensor;

unsigned long temper = millis();
unsigned long temper1 = millis();
unsigned long temp_cale_on = millis();
unsigned long temp_cale_off = millis();

// secuència d'arrancada
void setup() {
  // Inicialitzem port sèrie
  Serial.begin(115200);

  EEPROM.begin(512);                                            //Inicialitzem Eeprom interna
  consigna = EEPROM.read(256);                                  //Lectura part entera de consigna
  consigna = consigna + (EEPROM.read(258)/100.0);               //Lectura de la part decimal i suma el resultat
  operacio = EEPROM.read(200);                                  //Lectura del estat de la variable operació per saber si el control està connectat
  delay(10);
 
  pinMode(Pin_cale, OUTPUT);        //Configura Pin_cale com sortida
  pinMode(Pin_act, OUTPUT);         //Configura Pin_act com sortida
  pinMode(Pin_man,INPUT);           //Configura Pin_man com entrada
  pinMode(Pin_activar,INPUT);       //Configura Pin_activar com entrada
  digitalWrite(Pin_cale,0);         //Apagar calefacció al inici del cicle
  
  // Connectant a la xarxa WiFi
  Serial.println();
  Serial.print("Connectant a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connectat");
  
  // Inicialitzant servidor WEB
  server.begin();
  Serial.println("Servidor Web en marxa. Espereu per IP del ESP...");
  delay(500);
  
  // Visualitza adreça IP del ESP
  ip_xarxa = WiFi.localIP();
  Serial.println(ip_xarxa);

}

void lectura(){
 
    lecturaSensor = analogRead(A0);
    t = (lecturaSensor * 330) / 997.0;                //Lectura de temperatura en ºC
    if (lecturaSensor  <= 15){
      error = true;
    }else{
      error = false;
    }
  
  Ft = t*10;
  Rt = Ft%10;
  if (Rt>=5){
    tt=int(t)+0.5;
  }else{
    tt=int(t)+0.0;
  }
}

// Secuència principal
void loop() {
while(true){

  if (temper > millis()) temper = millis();
  if (millis()-temper >= 100){
    temper = millis();
    lectura();
  }
    
  // comprova nou client
  WiFiClient client = server.available();

  String req = client.readStringUntil('H');                       //Lectura de la primera linea de la resposta 
  //Serial.println(req);
  if (req.indexOf("cons=") != -1){
    String getV = req.substring(req.indexOf("=")+1,req.length());
    //Serial.println(getV);
    consigna = 1.0*(getV.toFloat());
   }
  
  if (error){                             //Comprova que no hi ha fallo de lectura i si n'hi ha, torni a fer una lectura nova
    Serial.println("Fallo lectura sensor LM35!");
    strcpy(celsiusTemp,"Fallo");
    digitalWrite(Pin_cale,0); 
    digitalWrite(Pin_act,1);
    delay(100);
    digitalWrite(Pin_act,0);
    delay(100);       
    }else{
      /*if (Serial.available()>0){                        //lectura de consigna des de terminal serie
          cons = 100*(Serial.read()-48);                //Lectura del primer número
          cons = cons+(10*(Serial.read()-48));          //Lectura del segon número
          int coma = Serial.read();                     //Lectura de la coma o el punt
          //cons = cons+(10*(Serial.read()-48));        //Lectura del tercer número
          cons = cons+(1*(Serial.read()-48));           //Lectura de quart número
          consigna = cons/10.0;                         //asigna el valor a la variable consigna   
        
       }*/
       
      entero = consigna*100.0; 
      EEPROM.write(256, (entero/100));
      EEPROM.write(258, (entero%100));
      EEPROM.commit();
      
      if (req.indexOf("incr=1") != -1) {           //Incrementa consigna en 0.5 graus
          consigna = consigna + 0.5;
      }
      if (req.indexOf("incr=0") != -1) {           //Disminueix la consigna en 0.5
          consigna = consigna - 0.5;
      }
      
      if (req.indexOf("mato=0") != -1) {
          consigna = 15.0;
      }
      if (req.indexOf("mato=1") != -1) {
          consigna = 20.0;
      }
      if (req.indexOf("mato=2") != -1) {
          consigna = 21.0;
      }
      if (req.indexOf("mato=3") != -1) {
          consigna = 22.0;
      }
       
      if (consigna < 5){                                //Rang de temperatura entre 5ºC i 30ºC
        consigna = 5;
      }
      if (consigna >30){
        consigna = 30;
      }
      
              
//Activar/desactivar calefació des d'el circuit
//********************************************************************
      if (digitalRead(Pin_activar) && !operacio){
         operacio = true;
         while (digitalRead(Pin_activar)){
            delay(20);
         }
      }else if (digitalRead(Pin_activar) && operacio){
         operacio = false;
      }
      
//Encendre/apagar calefacció des d'el circuit
      if (digitalRead(Pin_man) && !man){
         man = true;
         while (digitalRead(Pin_man)){
            delay(20); 
         }
      }else if (digitalRead(Pin_man) && man){
         man = false;
      }
//******************************************************************************          
      if (req.indexOf("auto=1") != -1) {           //Lectura per activar la calefacció
          operacio = true;
      }
      if (req.indexOf("auto=0") != -1) {           //lectura per desactivar la calefacció
          operacio = false;
      }
      
      if (req.indexOf("leds=1") != -1){           //Amb la calefacció desactivada, connectar-la 
          man = true;
      }
      if (req.indexOf("leds=0") != -1){                         //Amb la calefacció desactivada, desconnectar-la
          man = false;
      }

      EEPROM.write(200, operacio);
      
      if (operacio)                                              //si operacio = true, automàtic. Si operació = false, manual
        {
        if (t <= consigna){                                       //Comprova si la consigna de temperatura és més gran que la temperatura
          if (temp_cale_on > millis()) temp_cale_on = millis();
          if (millis() - temp_cale_on >= 15000){
            temp_cale_off = millis();
            digitalWrite(Pin_cale,1);                             // si és així connecta la salida per començar a escalfar
            cale = true;
          }
        }
        else
        {
          if (temp_cale_off > millis()) temp_cale_off = millis();
          if (millis() - temp_cale_off >= 15000){
            temp_cale_on = millis();
            digitalWrite(Pin_cale,0);                           //si no és així, desconnecta la salida
            cale = false;
          }
        }
        
         /*if ((tt < consigna + 0.5) && !histeresis){                //Comprova si la consigna de temperatura és més gran que la temperatura
            digitalWrite(Pin_cale,1);                             // si és així connecta la salida per començar a escalfar
            cale = true;
         }
         else if (tt >= consigna +0.5 || t > consigna) {
              digitalWrite(Pin_cale,0);                           //si no és així, desconnecta la salida
              cale = false;
              histeresis = true;
              }
              else if (tt <= consigna){
                   histeresis = false;
              }*/ 
        digitalWrite(Pin_act,1);
        man = false;
      }
      else{                                                       //Manual
            if (man){
               digitalWrite(Pin_cale,1);
            }
            else{
               digitalWrite(Pin_cale,0);
            }
            cale = false;
            digitalWrite(Pin_act,0);
            
            if (tt <= 15.0){                                          //Activa la calefacció si la temperatura ambient és menor de 15ºC
                operacio = true;
            }
      }
      client.flush();
      
      dtostrf(tt, 4, 1, celsiusTemp);
      dtostrf(consigna,4,1,consignaString);
      
      if (temper1 > millis()) temper1 = millis();
      if (millis()-temper1 >= 100){
        temper1 = millis();
        Serial.print("Temperatura: ");
        Serial.print(t);
        Serial.print("ºC");
        Serial.print("\t Segons: ");
        Serial.print(millis()/1000);
        Serial.print("s");
        Serial.print("\t Lectura LM35: ");
        Serial.print(lecturaSensor);
        Serial.println("mV");
      }
      
 }


//PÀGINA WEB

 client.println("HTTP/1.1 200 OK");
 client.println("Content-Type: text/html");
 client.println("Connection: close");
 client.println();
 
 client.println("<!DOCTYPE HTML>");
 client.println("<html>");
 client.println("<head> <style type='text/css'>body{width:100%;height:100%}</style><meta http-equiv='refresh' ");
 client.println("content='30;URL=/' charset='utf-8'>");
 client.println("</head><body bgcolor='#3daee9'><font color='white'><h1 align='center'>Control de temperatura calefacció</h1></font>");

 client.println("<table width='80%' align='center' border='2px'><tr>");
 client.println("<td width='50%' align='center' bgcolor='#9fa89d'><a style='font-size:25pt' href='/?auto=1'>Activar<br>Control</a></td>");
 client.println("<td>   </td>");
 client.println("<td width='50%' align='center' bgcolor='#a89d9e'><a style='font-size:25pt' href='/?auto=0'>Desactivar<br>Control</a></td>");
 client.println("</tr></table><br>");


 if (operacio){
    client.println("<table width='80%' align='center' border='2px'><tr><td bgcolor='green' align='center' colspan='4'><h1>Control Activat</h1></td</tr>");
    client.println("<tr><td width='25%' align='center'><a style='font-size:25pt' href='/?incr=1'>Augmentar(+)</a><br></td>");
    client.println("<td width='25%' align='center' rowspan='2'><font color='white'><h2>Consigna:<br>(graus ºC)</h2></font></td> ");
    client.println("<form action='/' method='get'>");
    client.println("<td width='25%' align='center' rowspan='2'><input style='text-align: center;width: 120px;height:50px;font-size:20pt' type='text' size=3 name='cons' value='");
    client.println(consignaString);
    client.println("'>");

    if (cale){
        client.println("<td width='25%' bgcolor='#00ff00' align='center' rowspan='2'><font color='black'><h2>Encesa</h2></td></tr>");
    }
    else{
        client.println("<td width='25%' bgcolor='red' align='center' rowspan='2'><font color='white'><h2>Apagada</h2></font></td></tr>");
    }
    client.println("<tr><td align='center'><a style='font-size:25pt' href='/?incr=0'>Disminuir(-)</a></td></tr>");
    client.println("<tr><td align='center' colspan='4'><input style='width:140px;height:50px;font-size:20pt' type='submit' value='Actualitzar'>");
    client.println("</td></form></tr>");
    client.println("<tr><td align='center'><a style='font-size:25pt' href='/?mato=0'>15ºC</a></td>");
    client.println("<td align='center'><a style='font-size:25pt' href='/?mato=1'>20ºC</a></td>");
    client.println("<td align='center'><a style='font-size:25pt' href='/?mato=2'>21ºC</a></td>");
    client.println("<td align='center'><a style='font-size:25pt' href='/?mato=3'>22ºC</a></td></tr>");
    client.println("</table><br><br>");
 }
 else{
    client.println("<table width='80%' align='center' border='2px'><tr>");
    client.println("<td bgcolor='red' align='center' colspan='2'><h1>Control Desactivat</h1></td>");
    client.println("</tr><tr>");
    client.println("<td align='center' colspan='2'><font color='white'><h2>Control Manual de la calefacció</h2></font></td>");
    client.println("</tr><tr>");
    client.println("<td width='50%' align='center'><a style='font-size:25pt' href='/?leds=1'>Encendre</a></td>");
    client.println("<td width='50%' align='center'><a style='font-size:25pt' href='/?leds=0'>Apagar</a></td>");
    client.println("</tr><tr>");
    if (man){
        client.println("<td colspan='2' bgcolor='#00ff00' style='height: 10px'></td>");
    }
    else{
        client.println("<td colspan='2' bgcolor='red' style='height: 10px'></td>");
    }
    client.println("</tr></table><br><br>");
 }
 
 client.println("<table align='center' border='2px'><caption style='font-size:18pt'>Paràmetres actuals</caption><tr>");
 client.println("<td><input style='text-align: center;width: 160px;height:60px;font-size:40pt' type='text' size=4 name='t' value='");
 client.println(celsiusTemp);
 client.println("'></td><td><h2> ºC de Temperatura</h2></td>");
 client.println("</tr><tr>");

 client.println("<td align='center'><input style='text-align: center;width: 80px;height:30px;font-size:20pt' type='text' size=4 name='s' value='");
 //client.println(sensacio);
 client.println(t);
 client.println("'></td><td><h2> ºC Reals</h2></td>");
 client.println("</tr><br><tr>");
 
 client.println("<td align='center'><input style='text-align: center;width: 80px;height:30px;font-size:20pt' type='text' size=4 name='h' value='");
 //client.println(humidityTemp);
 client.println("'></td><td><h2>---</h2></td>");
 client.println("</tr></table>");
 client.println("</body></html>");
 
 client.stop();
}
}   
