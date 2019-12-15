#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <LiquidCrystal.h>

#define REQ_BUF_SZ   80                // Dimensione del buffer usato per catturare le richieste HTTP
#define TERM_BUF_SZ  3                 // Dimensione del buffer usato per il termostato

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
char *val;
uint32_t timer=0;
uint32_t timerTemp=0;
float Temp=0;
char term_string[TERM_BUF_SZ] = {0};                       // Buffer contenente il valore del termostato
uint8_t Termostato=0;
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
boolean AllarmeGas=0;
boolean VentolaRisc=0;
boolean VentolaCond=0;
boolean AllarmeIntr=0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };      // MAC address della Ethernet shield
IPAddress ip(10, 0, 0, 2);                                // Indirizzo IP
EthernetServer server(80);                                // Crea un web server sulla porta 80
File webFile;                                             // Nome logico della pagina web sulla scheda micro-SD
char HTTP_req[REQ_BUF_SZ] = {0};                          // Buffer contenente le richieste HTTP
uint8_t req_index = 0;                                    // Indice posizione nel buffer HTTP_req

void setup()
{
    // Disabilita il pin utilizzato dalla Ethernet shield
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    Serial.begin(115200);                                 // Apre il monitor seriale
    
    // Initializza la scheda micro-SD
    Serial.println("Initializzo la scheda micro-SD...");
    if (!SD.begin(4)) {
        Serial.println("ERRORE - Inizializzatione della scheda micro-SD fallita!");
        return;    // esco dallo sketch
    }
    Serial.println("OK - Scheda micro-SD inizializzata.");
    // Controlla se esiste il file index.htm
    if (!SD.exists("index.htm")) {
        Serial.println("ERRORE - File index.htm non trovato!");
        return;  // esco dallo sketch
    }
    Serial.println("OK - File index.htm trovato.");
    
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
  
    Ethernet.begin(mac, ip);                          // Inizializzo la Ethernet shield
    server.begin();                                   // Mi metto in ascolto dei client
}

void loop()
{
    // Legge i sensori e innesca eventuali eventi
    ReadSensor();

    // Innesca il colloquio con eventuali client
    EthernetClient client = server.available();       // Controllo se ci sono client disponibili
    if (client) {                                     // Client trovato?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // I dati inviati dal client sono disponibili
                char c = client.read(); // legge 1 byte inviato dal client
                // Limita la dimensione della richiesta HTTP inviata che viene memorizzata nel buffer HTTP_req
                // Lascia l'ultimo elemento nel buffer a 0 (null) come terminatore della stringa (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // Memorizza il carattere della richiesta HTTP
                    req_index++;
                }
                // L'ultima linea della richiesta HTTP del client è vuota e finisce con \n
                // Risponde al client solo dopo aver ricevuto l'ultima linea
                if (c == '\n' && currentLineIsBlank) {
                    // Invia in risposta l'header standard HTTP
                    client.println("HTTP/1.1 200 OK");
                    // Segue la parte rimanente:
                    //     ... se è una richiesta AJAX, manda il file XML
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        client.println("Content-Type: text/xml");      // Invia la parte rimanente di header
                        client.println("Connection: keep-alive");
                        client.println();
                        // In base alla richiesta AJAX esegue l'azione su Arduino
                        SetArduino();
                        // Legge i sensori e innesca eventuali eventi
                        // ReadSensor();
                        // Invia al client il file XML file con i dati per la pagina web
                        XML_response(client);
                    }
                    //    ... se è una richiesta HTML, invia la pagina web (file index.htm)
                    else {
                        client.println("Content-Type: text/html");     // Invia la parte rimanente di header
                        client.println("Connection: keep-alive");
                        client.println();
                        webFile = SD.open("index.htm");                // Apre il file contenente la pagina web
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read());          // Invia la pagina web al client
                            }
                            webFile.close();                           // Chiude il file
                        }
                    }
                    // Invia sul monitor seriale la richiesta HTTP ricevuta
                    Serial.print(HTTP_req);
                    // Reinizializza il buffer HTTP_req e il relativo indice
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // Ogni linea di testo ricevuta dal client termina con \r\n
                if (c == '\n') {
                    currentLineIsBlank = true;   // Ultimo carattere ricevuto, inizio una nuova linea con il prossimo carattere letto
                } 
                else if (c != '\r') {
                    currentLineIsBlank = false;  // Un carattere di testo è stato ricevuto dal client
                }
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
    if (StrContains(HTTP_req, "Cam=1")) {
        digitalWrite(ledCamera, HIGH);
        LED_Camera = 1;
    }
    else if (StrContains(HTTP_req, "Cam=0")) {
        digitalWrite(ledCamera, LOW);
        LED_Camera = 0;
    }
    // LED Cucina (pin 24)
    if (StrContains(HTTP_req, "Cuc=1")) {
        digitalWrite(ledCucina, HIGH);
        LED_Cucina = 1;
    }
    else if (StrContains(HTTP_req, "Cuc=0") > 0) {
        digitalWrite(ledCucina, LOW);
        LED_Cucina = 0;
    }
    // LED Soggiorno (pin 26)
    if (StrContains(HTTP_req, "Sog=1")) {
          digitalWrite(ledSoggiorno, HIGH);
        LED_Soggiorno = 1;
    }
    else if (StrContains(HTTP_req, "Sog=0")) {
        digitalWrite(ledSoggiorno, LOW);
        LED_Soggiorno = 0;
    }
    // LED Bagno (pin 28)
    if (StrContains(HTTP_req, "Bag=1")) {
        digitalWrite(ledBagno, HIGH);
        LED_Bagno = 1;
    }
    else if (StrContains(HTTP_req, "Bag=0")) {
        digitalWrite(ledBagno, LOW);
        LED_Bagno = 0;
    }
    // LED Disimpegno (pin 32)
    if (StrContains(HTTP_req, "Dis=1")) {
        digitalWrite(ledDisimpegno, HIGH);
        LED_Disimpegno = 1;
    }
    else if (StrContains(HTTP_req, "Dis=0")) {
        digitalWrite(ledDisimpegno, LOW);
        LED_Disimpegno = 0;
    }
    // LED Garage (pin 34)
    if (StrContains(HTTP_req, "Gar=1")) {
        digitalWrite(ledGarage, HIGH);
        LED_Garage = 1;
    }
    else if (StrContains(HTTP_req, "Gar=0")) {
        digitalWrite(ledGarage, LOW);
        LED_Garage = 0;
    }
    // Intrusione (pin 29)
    if (StrContains(HTTP_req, "Intr=1"))
        Intrusione = 1;
    else if (StrContains(HTTP_req, "Intr=0"))
        Intrusione = 0; 
    // Riscaldamento
    if (StrContains(HTTP_req, "Risc=1"))
        Riscaldamento = 1;
    else if (StrContains(HTTP_req, "Risc=0"))
        Riscaldamento = 0;
    // Condizionamento
    if (StrContains(HTTP_req, "Cond=1"))
        Condizionamento = 1;
    else if (StrContains(HTTP_req, "Cond=0"))
        Condizionamento = 0;
    // Termostato
    if (StrContains(HTTP_req, "Term=")) {
        val = strstr(HTTP_req, "Term=");
        val = val+5;      // Mi posiziono dopo Term=
        StrClear(term_string, TERM_BUF_SZ);
        strncpy(term_string, val, 2);
        Termostato=atoi(term_string);
    }
    // Serranda
    if (StrContains(HTTP_req, "Serr=1")) {
       timerTemp = millis() + 10000;
       digitalWrite(controlPin1, LOW);
       digitalWrite(controlPin2, HIGH);
       analogWrite(accendiMotore, velocitaMotore);
       delay(10000);
       digitalWrite(accendiMotore, LOW);
       Serranda = 1;
    }
    else if (StrContains(HTTP_req, "Serr=0")) {
        digitalWrite(controlPin1, HIGH);
        digitalWrite(controlPin2, LOW);
        analogWrite(accendiMotore, velocitaMotore);
        delay(7500);
        analogWrite(accendiMotore, LOW);
        Serranda = 0;
    }
    // Buzzer
    if (StrContains(HTTP_req, "Buzz=1"))
        Buzzer = 1;
    else if (StrContains(HTTP_req, "Buzz=0"))
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
    unsigned char i, j ;
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
void XML_response(EthernetClient cl)
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

// Imposta ogni elemento della stringa a 0 (inizializza l'array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// Cerca la stringa sfind all'interno della stringa str
// return code: - 1 se ha trovato la stringa
//              - 0 se non ha trovato la stringa
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

