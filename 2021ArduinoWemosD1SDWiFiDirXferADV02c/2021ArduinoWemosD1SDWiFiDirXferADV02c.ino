//Arduino C.  Advanced. Sends SD file to Web browser via WiFi directly.  Reinits file.
//WeMos D1R1 (ESP8266 Boards 2.5.2), upload 921600, 80MHz, COM:7,WeMos D1R1 Boards 2
//WeMos Micro SD Shield uses HSPI(12-15) not (5-8), 3V3, G:
//GPIO12 (D6) = MISO      master in, slave out (primary in, secondary out)
//GPIO13 (D7) = MOSI      master out, slave in (primary out, secondary in)
//GPIO14 (D5) = CLK       Clock        
//GPIO15 (D8) = CS        Chip select  
//SD library --> 8.3 filenames (not case-sensitive, ABCDEFGH.txt = abcdefgh.txt).
//RTC DS1307. I2C--> SCL(clk)=D1, SDA(data)=D2 
//20210501 - TSBrownie.  Non-commercial use approved.
#include <SD.h>                          //SD card library
#include <SPI.h>                         //Serial library
#include <ESP8266WiFi.h>                 //WiFi library
#include <ESP8266WebServer.h>            //Web Server library
const char* ssid = "WemosD1R1-002";      //Desired SSID
const char* password = "12345678";       //Desired password
File dataFile;                           //SD card file handle
String FName = "20211224.txt";           //SD card file name (ANSI encoding is best)
const int msgLen = 57;                   //Xfer client confirm message length
char msg[msgLen];                        //Final confirm msg to client.  2GB file limit

IPAddress apIP(10, 10, 10, 1);           //IP Address for client
IPAddress gateway(10,10,10,1);           //Device IP for local connect
IPAddress subnet(255,255,255,0);         //Subnet mask
ESP8266WebServer server(80);             //Port 80, standard internet

//SD Routines ==============================
void openSD(){                           //Open SD card
  Serial.println("Opening SD card");     //User message
  if (!SD.begin(4)) {                    //If not open, print error msg
    Serial.println("Open SD card failed"); 
    return;
  }
  Serial.println("SD Card open");        //User msg
}

char openFile(char RW) {                 //Routine open SD file.  Only 1 open at a time.
  dataFile.close();                      //Ensure file status, before re-opening
  dataFile = SD.open(FName, RW);}        //Open Read at end.  Open at EOF for write/append

void initFile(){                         //Initialize SD card file
  dataFile.close();                      //Ensure file is closed         
  SD.remove(FName);                      //Delete file
  openFile(FILE_WRITE);                  //Open user SD file for write
  dataFile.close();                      //Close file
}

//Web Routines =============================
void handleRoot(){                       //Handle initial client request
  server.send(200, "text/html", "IP Linked. Add '/getdata' to URL to start transfer");}

void handle_NotFound(){                  //File not found
  server.send(404, "text/plain", "Not found");} //(code, content_type, content[text])

void getdata() {
  openFile(FILE_READ);                                           //Open SD file for read
  unsigned int fsizeSent = 0;                                    //Size of data sent
  int SDfileSz = dataFile.size();                                //Get size of file
  int SDfileSz0 = SDfileSz;                                      //Keep file size
  Serial.print("SD File Size: "); Serial.println(SDfileSz);      //Data file size
  server.sendHeader("Content-Length", (String)(SDfileSz));       //Header info
  server.sendHeader("Cache-Control", "max-age=2628000, public"); //Cache 30 days
  char buf[1024];                                                //File read/send buffer
  while(SDfileSz > 0) {                                          //While there's data
    size_t len = std::min((int)(sizeof(buf) - 1), SDfileSz);     //If short file, don't buffer
    dataFile.read((uint8_t *)buf, len);                          //Read SD card data
    server.client().write((const char*)buf, len);                //Send data chunk to client
    fsizeSent += len;                                            //Sent data length
    SDfileSz -= len;                                             //File data-amount read chunk
  }
  sprintf(msg,"%s%09i%s%09i%s","*** EOF ***  File Size: ", SDfileSz0, "   Data Sent: ", fsizeSent,"\n");
  server.client().write((const char*)msg, msgLen);               //Send chunk to client
  Serial.print("Data Sent: "); Serial.println(SDfileSz0);        //Data file size
  dataFile.close();                                              //Close SD file
  delay(100);
}

//Setup / Loop =============================
void setup(void) {
  Serial.begin(115200);                              //Open COM
  Serial.println(' ');                               //Blank line
  Serial.println("Connecting to network ");          //User msg
  WiFi.softAP(ssid, password);                       //Connect w/ SSID/PW
  WiFi.softAPConfig(apIP, gateway, subnet);          //Config
  delay(150);
  Serial.print("SSID Connected: ");Serial.println(ssid);         //User msg SSID
  Serial.print("Local IP      : ");Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);                        //First web input
  server.on("/getdata", getdata);                    //Data request
  server.on("/initfile", initFile);                  //Data request
  server.onNotFound(handle_NotFound);                //If error
  server.begin();                                    //Start server
  Serial.println("HTTP Server Started");             //User info
  openSD();                                          //Call open SD card
}

void loop(void){
  server.handleClient();                             //Handle client calls
}
