//----------------------------------------------------------------------------//
//  Sono da sistemare:
//  - la funzione SuonaCicalino, che ha un loop di 1 sec. che blocca lo sketch
//----------------------------------------------------------------------------//

#include <Flash.h>
#include <Fishino.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>

#define REQ_BUF_SZ  100                // Dimensione del buffer usato per catturare le richieste HTTP
#define TERM_BUF_SZ   3                // Dimensione del buffer usato per il termostato
#define FILE_SZ      13                // Dimensione del buffer usato per il nome del file (8.3)

// Definizione variabili locali
LiquidCrystal lcd(33, 35, 39, 41, 43, 45);

// Definizione costanti: i pin di Arduino
//   ... digitali
const uint8_t ledCamera=22;
const uint8_t ledCucina=24;
const uint8_t ledSoggiorno=26;
const uint8_t ledBagno=28;
const uint8_t ledDisimpegno=32;
const uint8_t ledGarage=34;
const uint8_t ledCrepuscolari=36;
const uint8_t ledRiscaldamento=38;
const uint8_t ledCondizionamento=40;
const uint8_t ledAllarmeVerde=44;
const uint8_t ledAllarmeRosso=46;
const uint8_t Cicalino=31;
const uint8_t Ventola=23;
const uint8_t pinGasDig=27;
const uint8_t SensoreMovimento=29;
//   ... analogici
const byte pinFotores=A2;
const byte pinGasAnalog=A3;
const byte pinTemp=A4;
//   ... costanti per motore dc
const uint8_t controlPin1=2;
const uint8_t controlPin2=3;
const uint8_t accendiMotore=9;
const uint8_t velocitaMotore=200;

// Definizione variabili
char HTTP_req[REQ_BUF_SZ];                                // Buffer contenente le richieste HTTP
uint8_t req_index = 0;                                    // Indice posizione nel buffer HTTP_req
char *val;
uint32_t timer=0;
uint32_t timerTemp=0;
uint32_t timerSerr=0;
float Temp=0;
char term_string[TERM_BUF_SZ] = {0};                       // Buffer contenente il valore del termostato
uint8_t Termostato=23;
int readAnalog=0;
int readDigital=0;
// variabili booleane per xml
boolean LED_Camera=0;
boolean LED_Cucina=0;
boolean LED_Soggiorno=0;
boolean LED_Bagno=0;
boolean LED_Disimpegno=0;
boolean LED_Garage=0;
boolean LED_Crepuscolari=0;
boolean Intrusione=0;
boolean Movimento=0;
boolean Riscaldamento=0;
boolean Condizionamento=0;
boolean Gas=0;
boolean Serranda=0;
boolean Buzzer=1;                                         // Buzzer è di default attivo
boolean VentolaRisc=0;
boolean VentolaCond=0;
boolean AllarmeGas=0;
boolean AllarmeIntr=0;

File webFile;                                             // Nome logico della pagina web sulla scheda micro-SD

//----------------------------------------------------------------------------//
//                         Configurazione rete
//----------------------------------------------------------------------------//

#ifndef __MY_NETWORK_H

// Rete WiFi
// Modo di operazione:
// - NORMAL (STATION) -- serve una rete WiFi esistente a cui connettersi
// - STANDALONE (AP)  -- realizza una rete WiFi su Fishino
//#define STANDALONE_MODE

// Definizioni parametri per STATION mode
#define MY_SSID "Gramsci_12"              // SSID della rete WiFi
#define MY_PASS "al1912-fe2304"           // Password rete WiFi
#define IPADDR  192, 168, 1, 162       // Indirizzo IP 
#define GATEWAY 192, 168, 1,   1       // Gateway
#define NETMASK 255, 255, 255, 0       // Network mask

// Definizioni parametri per STANDALONE mode
//#define MY_SSID "MyFishino"               // SSID della rete WiFi
//#define MY_PASS ""                        // Password rete WiFi
//#define IPADDR  10, 0, 0, 1       // Indirizzo IP 

#endif

//----------------------------------------------------------------------------//

#ifdef IPADDR
  IPAddress ip(IPADDR);
  #ifdef GATEWAY
    IPAddress gw(GATEWAY);
  #else
    IPAddress gw(ip[0], ip[1], ip[2], 1);
  #endif
  #ifdef NETMASK
    IPAddress nm(NETMASK);
  #else
    IPAddress nm(255, 255, 255, 0);
  #endif
#endif

FishinoServer server(80);      // Porta di ascolto del web server

//----------------------------------------------------------------------------//
//            Stampa su monitor seriale i parametri di connessione
//----------------------------------------------------------------------------//
void printWiFiStatus()
{
  Serial.print("SSID: ");
  #ifdef STANDALONE_MODE
    Serial.println(MY_SSID);
  #else
    Serial.println(Fishino.SSID());
  #endif
  uint8_t mode = Fishino.getPhyMode();
  Serial.print("PHY MODE: (");
  Serial.print(mode);
  Serial.print(") ");
  switch (mode)
  {
    case PHY_MODE_11B:
      Serial.println("11B");
      break;
    case PHY_MODE_11G:
      Serial.println("11G");
      break;
    case PHY_MODE_11N:
      Serial.println("11N");
      break;
    default:
      Serial.println("SCONOSCIUTO");
  }
  
  #ifdef STANDALONE_MODE
    // prelevo informazioni IP dell'Access Point
    IPAddress ip, gw, nm;
    if (Fishino.getApIPInfo(ip, gw, nm))
    {
      Serial << F("Fishino IP      :") << ip << "\r\n";
      Serial << F("Fishino GATEWAY :") << gw << "\r\n";
      Serial << F("Fishino NETMASK :") << nm << "\r\n";
    }
    else
      Serial << F("(E) Errore rilevamento informazioni IP Fishino") << ip << "\r\n";
  #else
    // stampa l'indirizzo IP del Fishino
    IPAddress ip = Fishino.localIP();
    Serial.print("Indirizzo IP ");
    Serial.println(ip);

    // stampa la qualità del segnale WiFi
    Serial.print("Potenza del segnale wi-fi (RSSI): ");
    Serial.print(Fishino.RSSI());
    Serial.println(" dBm");
  #endif

}
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//                     Inizializzazione dello sketch
//----------------------------------------------------------------------------//
void setup()
{
  Serial.begin(115200);      // Apre il monitor seriale
    
  // Initializza la scheda micro-SD
  Serial << F("Initializzo la scheda micro-SD...\r\n");
  if (!SD.begin(4)) {
      Serial << F("ERRORE - Inizializzatione della scheda micro-SD fallita!\r\n");
      return;    // esco dallo sketch
  }
  Serial << F("OK - Scheda micro-SD inizializzata.\r\n");
  // Controlla se esiste il file index_X.htm
  if (!SD.exists("index_X.htm"))
  {
      Serial << F("ERRORE - File index_X.htm non trovato!\r\n");
      return;                          // esco dallo sketch
  }
  Serial << F("OK - File index_X.htm trovato.\r\n");
    
  // Inizializzo i pin di Arduino
  pinMode(ledCamera, OUTPUT);
  pinMode(ledCucina, OUTPUT);
  pinMode(ledSoggiorno, OUTPUT);
  pinMode(ledBagno, OUTPUT);
  pinMode(ledDisimpegno, OUTPUT);
  pinMode(ledGarage, OUTPUT);
  pinMode(ledCrepuscolari, OUTPUT);
  pinMode(ledRiscaldamento, OUTPUT);
  pinMode(ledCondizionamento, OUTPUT);
  pinMode(ledAllarmeRosso, OUTPUT);
  pinMode(ledAllarmeVerde, OUTPUT);
  pinMode(Cicalino, OUTPUT);
  pinMode(Ventola, OUTPUT);
  pinMode(pinGasDig,INPUT);
  pinMode(SensoreMovimento,INPUT);
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  pinMode(accendiMotore, OUTPUT);
  digitalWrite(accendiMotore, LOW);                 // Stato iniziale motore: spento

  // Inizializzo lcd
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Temp:       oC");
  lcd.setCursor(0, 1);
  lcd.print("Home sweet home");
  // Inizializza il modulo SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // Re-inizializza e controlla il modulo WiFi
  Serial << F("Inizializzo Fishino...");
  while (!Fishino.reset())
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");
  Serial << F("Fishino WiFi Web Server\r\n");

  // Imposta la modalità fisica a 11G
  Fishino.setPhyMode(PHY_MODE_11G);

  #ifdef STANDALONE_MODE
    // Imposta i parametri dell'Access Point
    // ... modalità SOFTAP
    Serial << F("Imposto la modalità SOFTAP_MODE\r\n");
    Fishino.setMode(SOFTAP_MODE);
    // ... arresta il server DHCP
    Serial << F("Fermo il server DHCP\r\n");
    Fishino.softApStopDHCPServer();
    // ... imposta i parametri dell'access point
    Serial << F("Imposto i parametri IP\r\n");
    Fishino.setApIPInfo(ip, ip, IPAddress(255, 255, 255, 0));
    Serial << F("Imposto i parametri WiFi\r\n");
    Fishino.softApConfig(MY_SSID, MY_PASS, 1, false);
    // ... riavvia il server DHCP
    Serial << F("Avvio il server DHCP\r\n");
    Fishino.softApStartDHCPServer();
  
  #else
    // imposta la modalità STATION
    Serial << F("Imposto la modalità STATION_MODE\r\n");
    Fishino.setMode(STATION_MODE);

    // si connette all'access point
    Serial << F("Mi connetto alla rete Wi-Fi...");
    while(!Fishino.begin(MY_SSID, MY_PASS))
    {
      Serial << ".";
      delay(500);
    }
    Serial << F("OK\r\n");

    // imposta l'IP statico o avvia il client DHCP
    #ifdef IPADDR
      Fishino.config(ip, gw, nm);
    #else
      Fishino.staStartDHCP();
    #endif

    // attende il completamento della connessione
    Serial << F("Attesa dell'indirizzo IP...");
    while(Fishino.status() != STATION_GOT_IP)
    {
      Serial << ".";
      delay(500);
    }
    Serial << F("OK\r\n");

  #endif

  // Visualizza o stato della connessione
  printWiFiStatus();

  // Il server rimane in attesa delle connessioni da parte dei client
  server.begin();
}

//----------------------------------------------------------------------------//
//                            Ciclo infinito
//----------------------------------------------------------------------------//
void loop(void)
{
    // Legge i sensori e innesca eventuali eventi
    ReadSensor();

    //  Attende nuovi client
    FishinoClient client = server.available();

    if (client)
    {
	    //  Si è connesso un nuovo client
	    boolean currentLineIsBlank = true;   // ogni richiesta http termina con una linea vuota
	    boolean gotGetLine = false;          // flag che indica che la prima riga (GET) è stata letta
	  
      while (client.connected())
	    {
        if (client.available())            // i dati inviati dal client sono disponibili
		    {
           char c = client.read();         // legge 1 byte inviato dal client
		       // Se sta leggendo la prima riga, aggiunge nel buffer il carattere letto.
           // Limita la dimensione della richiesta HTTP (memorizzata in HTTP_req) alla dimensione del buffer (REQ_BUF_SZ - 1)
           if (!gotGetLine && c != '\r' && c != '\n' && req_index < (REQ_BUF_SZ - 1))
               HTTP_req[req_index++] = c;    // aggiunge nel buffer il carattere della richiesta HTTP

		       // Sempre se sta leggendo la prima linea e trova un carattere di fine linea, la linea è terminata
           if ((c == '\r' || c == '\n') && !gotGetLine)
           {
              HTTP_req[req_index] = 0;     // aggiunge il terminatore di stringa alla linea appena letta
                                           // (tutte le linee in C terminano con un carattere nullo, 0)
              gotGetLine = true;           // indica che la prima linea è stata letta
           }

     	     // L'ultima linea della richiesta HTTP del client è vuota e finisce con \n
           // Risponde al client solo dopo aver ricevuto l'ultima linea
           if (c == '\n' && currentLineIsBlank)
		       {
		   
               // Segue la parte rimanente:
               //     ... se è una richiesta AJAX, esegue le azioni richieste e restituisce il file XML
               if (strstr(HTTP_req, "ajax_inputs"))
               {
                   Serial.println(HTTP_req);
                   client << F("HTTP/1.1 200 OK\r\n");             // risposta standard HTTP: OK!
                   client << F("Content-Type: text/xml\r\n");      // Invia la parte rimanente di header
                   client << F("Connection: keep-alive\r\n");
                   client << F("\r\n");
                   // In base alla richiesta AJAX esegue l'azione su Arduino
                   SetArduino();
                   // Invia al client il file XML file con i dati per la pagina web
                   XML_response(client);
               }
               //    ... se è una richiesta della home page, invia la pagina web (file index_X.htm)
               else if (strstr(HTTP_req, "GET / "))
			         {
                   client << F("HTTP/1.1 200 OK\r\n");             // risposta standard HTTP: OK!
                   client << F("Content-Type: text/html\r\n");     // invia la parte rimanente di header
                   client << F("Connection: keep-alive\r\n");
                   client << F("\r\n");
                   webFile = SD.open("index_X.htm");                 // apre il file contenente la pagina web
                   if (webFile) {
                      Serial.println(HTTP_req);
                      while(webFile.available()) {
                           client.write(webFile.read());          // invia la pagina web al client
                      }
                   }
                   webFile.close();                               // chiude il file
               }
               // Invia sul monitor seriale la richiesta HTTP ricevuta
               req_index = 0;                                      // reinizializza l'indice del buffer HTTP
               break;
           }
           // Ogni linea di testo ricevuta dal client termina con \r\n
           if (c == '\n')
               currentLineIsBlank = true;   // Ultimo carattere ricevuto, inizio una nuova linea con il prossimo carattere letto
           else if (c != '\r')
               currentLineIsBlank = false;  // Un carattere di testo è stato ricevuto dal client
        }
      }
      delay(1);         // Dà tempo al browser di ricevere i dati
      client.stop();    // Chiude la connessione
	}
}

// In base alla richiesta AJAX esegue l'azione su Arduino
// Inoltre salva lo stato dei vari sensori e attuatori per comporre il file XML da inviare al/ai client
void SetArduino(void)
{
    // LED Camera (pin 22)
    if (strstr(HTTP_req, "Cam=1")) {
        digitalWrite(ledCamera, HIGH);
        LED_Camera = 1;
    }
    else if (strstr(HTTP_req, "Cam=0")) {
        digitalWrite(ledCamera, LOW);
        LED_Camera = 0;
    }
    // LED Cucina (pin 24)
    if (strstr(HTTP_req, "Cuc=1")) {
        digitalWrite(ledCucina, HIGH);
        LED_Cucina = 1;
    }
    else if (strstr(HTTP_req, "Cuc=0") > 0) {
        digitalWrite(ledCucina, LOW);
        LED_Cucina = 0;
    }
    // LED Soggiorno (pin 26)
    if (strstr(HTTP_req, "Sog=1")) {
          digitalWrite(ledSoggiorno, HIGH);
        LED_Soggiorno = 1;
    }
    else if (strstr(HTTP_req, "Sog=0")) {
        digitalWrite(ledSoggiorno, LOW);
        LED_Soggiorno = 0;
    }
    // LED Bagno (pin 28)
    if (strstr(HTTP_req, "Bag=1")) {
        digitalWrite(ledBagno, HIGH);
        LED_Bagno = 1;
    }
    else if (strstr(HTTP_req, "Bag=0")) {
        digitalWrite(ledBagno, LOW);
        LED_Bagno = 0;
    }
    // LED Disimpegno (pin 32)
    if (strstr(HTTP_req, "Dis=1")) {
        digitalWrite(ledDisimpegno, HIGH);
        LED_Disimpegno = 1;
    }
    else if (strstr(HTTP_req, "Dis=0")) {
        digitalWrite(ledDisimpegno, LOW);
        LED_Disimpegno = 0;
    }
    // LED Garage (pin 34)
    if (strstr(HTTP_req, "Gar=1")) {
        digitalWrite(ledGarage, HIGH);
        LED_Garage = 1;
    }
    else if (strstr(HTTP_req, "Gar=0")) {
        digitalWrite(ledGarage, LOW);
        LED_Garage = 0;
    }

    // Intrusione (pin 29)
    if (strstr(HTTP_req, "Intr=1"))
        Intrusione = 1;
    else if (strstr(HTTP_req, "Intr=0"))
        Intrusione = 0; 
    // Riscaldamento
    if (strstr(HTTP_req, "Risc=1"))
        Riscaldamento = 1;
    else if (strstr(HTTP_req, "Risc=0"))
        Riscaldamento = 0;
    // Condizionamento
    if (strstr(HTTP_req, "Cond=1"))
        Condizionamento = 1;
    else if (strstr(HTTP_req, "Cond=0"))
        Condizionamento = 0;

    // Termostato
    val = strstr(HTTP_req, "Term=");
    if (val) {
        val = val+5;      // Mi posiziono dopo Term=
        //StrClear(term_string, TERM_BUF_SZ);
        //strncpy(term_string, val, 2);
        strncpy(term_string, val, TERM_BUF_SZ);
        term_string[TERM_BUF_SZ] = 0;         // terminatore di stringa = null
        Termostato=atoi(term_string);
    }

    // Serranda
    if (strstr(HTTP_req, "Serr=1")) {
       if (timerSerr == 0)      // se è in corso apertura o chiusura serranda non faccio nulla
       {
         timerSerr = millis() + 10000;
         digitalWrite(controlPin1, LOW);
         digitalWrite(controlPin2, HIGH);
         analogWrite(accendiMotore, velocitaMotore);
       }
    }
    else if (strstr(HTTP_req, "Serr=0")) {
       if (timerSerr == 0)      // se è in corso apertura o chiusura serranda non faccio nulla
       {
          timerSerr = millis() + 7500;
          digitalWrite(controlPin1, HIGH);
          digitalWrite(controlPin2, LOW);
          analogWrite(accendiMotore, velocitaMotore);
       }
    }
    // Se la serranda è in azione ed è passato il tempo per la apertura/chiusura, spengo il motore e inverto il flag
    if (timerSerr > 0)
    {
      if (millis() > timerSerr)
         {
            digitalWrite(accendiMotore, LOW);   // spengo il motore
            timerSerr = 0;                      // riazzero il timer della Serranda
            if (Serranda == 0)                  // inverto il flag (per xml)
              Serranda = 1;
            else
              Serranda = 0;
        }
    }
    
    // Buzzer
    if (strstr(HTTP_req, "Buzz=1"))
        Buzzer = 1;
    else if (strstr(HTTP_req, "Buzz=0"))
        Buzzer = 0;
}

// Legge i vari sensori ed esegue le opportune operazioni
void ReadSensor(void)
{
    // Rilevamento temperatura
    readAnalog = (analogRead(pinTemp));
    Temp = readAnalog * 0.48875; 

    // Visualizzo temperatura su lcd ogni 10 secondi
    if ( millis() > timerTemp )
    {
        lcd.setCursor(6,0);
        lcd.print(Temp);
        timerTemp = millis() + 10000;
    }
    
    // Rilevamento fotoresistenza per crepuscolari
    readAnalog = (analogRead(pinFotores));
    if(readAnalog > 500)
    {
        digitalWrite(ledCrepuscolari, HIGH);
        LED_Crepuscolari=1;
    }
    else
    {
        digitalWrite(ledCrepuscolari, LOW);
        LED_Crepuscolari=0;
    }
    
    // Sensore intrusione
    if (Intrusione) {
        //Accende led verde
        digitalWrite(ledAllarmeVerde, HIGH);
        int pirSegnal = digitalRead( SensoreMovimento );
        switch (pirSegnal)
        {
          case 0:
            // Accende led verde, se non c'è allarme gas in corso spegne led rosso e toglie suono cicalino
            digitalWrite( ledAllarmeVerde, HIGH );
            if (!AllarmeGas)
            {
                digitalWrite( ledAllarmeRosso, LOW );
                digitalWrite(Cicalino, LOW);
            }
            AllarmeIntr=0;
            Movimento=0;
            break;
          case 1:
            // Spegne led verde, accende led rosso e suona cicalino
            digitalWrite( ledAllarmeVerde, LOW );
            digitalWrite( ledAllarmeRosso, HIGH );
            SuonaCicalino();
            AllarmeIntr=1;
            Movimento=1;
            break;
        }
    }
    else {
        // Spegni led verde e se non c'è allarme gas in corso spegne led rosso e toglie suono cicalino
        digitalWrite(ledAllarmeVerde, LOW);
        if (!AllarmeGas)
        {
            digitalWrite(ledAllarmeRosso, LOW);
            digitalWrite(Cicalino, LOW);
        }
        AllarmeIntr=0;
        Movimento=0;
    }

    // Sensore gas
    readDigital = digitalRead(pinGasDig);
    readAnalog = analogRead (pinGasAnalog);
    if (readAnalog>200)
    {
        digitalWrite( ledAllarmeRosso, HIGH );
        SuonaCicalino();
        AllarmeGas=1;
        Gas=1;
    }
    else {
        // Se non c'è allarme intrusione in corso spegne led rosso e toglie suono cicalino
        if (!AllarmeIntr)
        {
            digitalWrite( ledAllarmeRosso, LOW );
            digitalWrite(Cicalino, LOW);
        }
        AllarmeGas=0;
        Gas=0;
    }
    // Riscaldamento e condizionamento
    if (!Riscaldamento && !Condizionamento) {
             digitalWrite(Ventola, LOW);
             digitalWrite(ledRiscaldamento, LOW);
             digitalWrite(ledCondizionamento, LOW);
        }
	
    if (Riscaldamento) {
        if (Temp < Termostato){
             VentolaRisc=1;
             digitalWrite(Ventola, HIGH);
             digitalWrite(ledRiscaldamento, HIGH);
        }
        else {
             VentolaRisc=0;
             digitalWrite(Ventola, LOW);
             digitalWrite(ledRiscaldamento, LOW);
        }
    }
    else {
             if (!VentolaCond)
                 digitalWrite(Ventola, LOW);
                 digitalWrite(ledRiscaldamento, LOW);
    }

    if (Condizionamento) {
        if (Temp > Termostato){
             VentolaCond=1;
             digitalWrite(Ventola, HIGH);
             digitalWrite(ledCondizionamento, HIGH);
        }
        else {
             VentolaCond=0;
             digitalWrite(Ventola, LOW);
             digitalWrite(ledCondizionamento, LOW);
        }
    }
    else {
             if (!VentolaRisc)
                 digitalWrite(Ventola, LOW);
                 digitalWrite(ledCondizionamento, LOW);
        }
}

void SuonaCicalino(void)
{
    // Se il buzzer è silenziato esce dalla funzione
    if (!Buzzer)
      return;
    //******************************************************************//
    return;
    //******************************************************************//
    unsigned char i;
    timer = millis() + 1000;
    while ( millis() < timer )
    {
      for (i = 0; i <100; i++){
        digitalWrite (Cicalino, HIGH);
        delay (1) ;
        digitalWrite (Cicalino, LOW);
        delay (1) ;
      }
      for (i = 0; i <100; i++){
        digitalWrite (Cicalino, HIGH);
        delay (2) ;
        digitalWrite (Cicalino, LOW);
        delay (2) ;
      }
    }
}

// Invia il file XML con i dati per comporre la pagina web
void XML_response(FishinoClient cl)
{   
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    
    // LED Camera
    cl.print("<camera>");
    if (LED_Camera)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</camera>");

    // LED Cucina
    cl.print("<cucina>");
    if (LED_Cucina)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</cucina>");

    // LED Soggiorno
    cl.print("<soggiorno>");
    if (LED_Soggiorno)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</soggiorno>");

    // LED Bagno
    cl.print("<bagno>");
    if (LED_Bagno)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</bagno>");

    // LED Disimpegno
    cl.print("<disimpegno>");
    if (LED_Disimpegno)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</disimpegno>");

    // LED Garage
    cl.print("<garage>");
    if (LED_Garage)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</garage>");

    // LED Crepuscolari
    cl.print("<crepuscolari>");
    if (LED_Crepuscolari)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</crepuscolari>");

    // LED Temperatura
    cl.print("<temperatura>");
        cl.print(Temp);
    cl.println("</temperatura>");

    // Intrusione
    cl.print("<intrusione>");
    if (Intrusione)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</intrusione>");

    // Movimento
    cl.print("<movimento>");
    if (Movimento)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</movimento>");

    // Riscaldamento
    cl.print("<riscaldamento>");
    if (Riscaldamento)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</riscaldamento>");
    
    // Condizionamento
    cl.print("<condizionamento>");
    if (Condizionamento)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</condizionamento>");

    // Termostato
    cl.print("<termostato>");
        cl.print(Termostato);
    cl.println("</termostato>");

    // Gas
    cl.print("<gas>");
    if (Gas)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</gas>");

    // Serranda
    cl.print("<serranda>");
    if (Serranda)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</serranda>");

    // Buzzer
    cl.print("<buzzer>");
    if (Buzzer)
        cl.print("on");
    else
        cl.print("off");
    cl.println("</buzzer>");
    
    cl.print("</inputs>");
}

