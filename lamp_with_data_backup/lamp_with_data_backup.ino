/*
  OpenSource_RGB_Lamp

  created 2018
  by Gonzalo Cervantes <https://github.com/cervgon/OpenSource_RGB_Lamp>

  This example code is in the public domain.
*/
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

const char* SSID = "the_name_of_your_network_goes_here";
const char* PASS = "its_password_goes_here";
 
WiFiServer server(80);
WiFiClient client;

// D6 is not mandatory, I chose it without any reason.
#define PIN         D6

// Number of leds
#define NUMPIXELS   16

// Set up NeoPixel parameters
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// EEPROM
int colors = NUMPIXELS;
// Create array to save NUMPIXELS * 3 (channels)
int lastColors[NUMPIXELS*3]; // 16 x 3

void setup() {
  Serial.begin(115200);
  // Set pinout
  pinMode(PIN, OUTPUT);
  pixels.begin();
  delay(10);
  // Set up brightness to 100%
  pixels.setBrightness(100);
  // Init EEPROM
  EEPROM.begin(4096);
  delay(50);
  // Read quantity of colors
  colors =  EEPROM.read(0), DEC;
  delay(50);
  // Check if 'colors' is appropriate
  if (colors <= 0 || colors > NUMPIXELS){
    colors = NUMPIXELS;
    // Set colors = 16 in this case
    EEPROM.write(0, colors);
    EEPROM.commit();
    delay(20);
    int spacesInMemoryTemp = (colors*3) + 1;
    // Set every channel as 255
    for ( int i = 1; i < spacesInMemoryTemp; i++ ) {
      EEPROM.write(i, 0);
      EEPROM.commit();
      delay(20);
    }
  }
  
  Serial.println("\n\nOpenSource_RGB_Lamp | Change every LED as you please | by Gonzalo Cervantes.\n\n ¯|_(ツ)_|¯ It's something.. \n\n");
 
  int spacesInMemory = NUMPIXELS * 3;
  Serial.print("Saved values in memory\n"); Serial.print(colors); Serial.print("|");
  for ( int i = 0; i < spacesInMemory; i++ ) {
      lastColors[ i ] = EEPROM.read(i+1), DEC;
      delay(10);
      Serial.print(lastColors[ i ]);
      Serial.print("|");
   }
  Serial.println();
 
  // Connect to WiFi network
  Serial.print("\nConnecting to ");   Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  WiFi.begin(SSID, PASS);
  IPAddress ip(192,168,0,200);   
  IPAddress gateway(192,168,0,254);   
  IPAddress subnet(255,255,255,0);   
  WiFi.config(ip, gateway, subnet);
  
  // Wait until it is connected..
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Wait for it..\n");
  }
  Serial.println("WiFi connected (y)"); // WEEEE!
  // Start the server
  server.begin();
  Serial.println("Server started (y)");
  // Print the IP address
  Serial.print("Use this URL to connect: ");  Serial.print("http://");  Serial.print(WiFi.localIP()); Serial.println("/\n");
  Serial.println("\n===============\nShall we begin?\n===============");
  Serial.println("\nWaiting for colors..\n");
  // Show the colors which were saved in the EEPROM.
  showColors(lastColors);
}

void loop() {
  // Check if a client has connected
  client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.print("New Request = ");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  client.flush();

  // Use the request wisely
  process(request);

  delay(50); // Wait a bit
}

// Parse the request and process the request
void process(String request) {
  int requestLenght = request.length(); // Get the length of the request
  String requestValue = request.substring(5, requestLenght - 9); // removes "GET /"
  Serial.println(requestValue); // Show the request value
  int requestValueLenght = requestValue.length(); // Gets the lenght of the value
  String response = ""; // Init response

  // Displays 1,2,4,8 or 16 colors according to the request.
  switch (requestValueLenght) {
     case 6:
        response = "Color changed"; // 1 color, e.g. ff0000
        changeColorsTo(requestValue,1);
        break;
     case 13:
        response = "Colors changed"; // 2 colors e.g. ff0000-00ff66
        changeColorsTo(requestValue,2);
        break;
     case 27:
        response = "Colors changed"; // 4 colors e.g. ff0000-66ff00-00ff66-0000ff
        changeColorsTo(requestValue,4);
        break;
     case 55:
        response = "Colors changed"; // 8 colors e.g. ff0000-ff9900-66ff00-00ff00-00ff66-0066ff-0000ff-ff00ff
        changeColorsTo(requestValue,8);
        break;
     case 111:
        response = "Colors changed"; // 16 colors e.g. ff0000-ff6600-ff9900-ffff00-66ff00-99ff00-00ff00-00ff99-00ff66-00ffff-0066ff-0099ff-0000ff-9900ff-ff00ff-ff0099
        changeColorsTo(requestValue,16);
        break;
     default:
        response = "Wrong query"; // 
        Serial.println(response);
  }
  // Returns the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text");
  client.println(""); //  Do not forget this one
  client.print(response);
  delay(1);
  Serial.print("Server Response: ");
  Serial.println(response);
  Serial.println("Client disonnected --> Goodbye Moonmen\n");
}

// Parse the Request
void changeColorsTo(String requestValue, int quantityOfColors){
  // Get an array of hex colors
  String hexColors[quantityOfColors];
  int index = 0;
  for ( int i = 0; i < quantityOfColors; i++ ) {
    int secIndex = index + 6;
    hexColors[i] = requestValue.substring(index,secIndex);
    index = index + 7; // 'xxxxxx-'
  }
  // Build the array, show the colors and save them into the EEPROM.
  rgbColorsBuilder(hexColors, quantityOfColors); // See below
}

// Create an array populated with all the colors in RGB, show them & save all the data in the memory
void rgbColorsBuilder(String hexColors[], int quantityOfColors){
  // Convert an array of hex colors into an array of R,G,B colors. E.g [ff00ff,00ff00] => [255,0,255,0,255,0]
  int rgbColorsLength = NUMPIXELS * 3; // In this case 16 * 3 = 48
  int rgbColors[rgbColorsLength]; // Set array length
  int colorStep = NUMPIXELS / quantityOfColors;   // e.g. ff00ff-00ff00: 16 / 2 = 8
  int counter = 0;
  // Go over every color recursively
  for ( int i = 0; i < quantityOfColors; i++ ) {
    String currentHex = hexColors[i]; // Get the current strings
    String rr = currentHex.substring(0,2); // Extract the first 2 hex strings
    String gg = currentHex.substring(2,4); // Extract the second 2 hex strings
    String bb = currentHex.substring(4,6); // Extract the third 2 hex strings
    int thisR = hexToInt256(rr); // Convert the first 2 hex into decimal
    int thisG = hexToInt256(gg); // Convert the second 2 hex into decimal
    int thisB = hexToInt256(bb); // Convert the third 2 hex into decimal
    // Show the same color 'colorStep' times
    for ( int j = 0; j < colorStep; j++ ) {
      counter = i*colorStep + j;
      int rrIndex = counter * 3;
      int ggIndex = (counter * 3) + 1;
      int bbIndex = (counter * 3) + 2;
      rgbColors[rrIndex] =  thisR; // Save the red channel value into the array
      rgbColors[ggIndex] =  thisG; // Save the green channel value into the array
      rgbColors[bbIndex] =  thisB; // Save the blue channel value into the array
    }
  }
  // Show the colors in the ring
  showColors(rgbColors);
  // Save the colors into the EEPROM
  saveColorsEEPROM(rgbColors, quantityOfColors);
}

// Display the colors in the ring based on the rgbColors array.
void showColors(int rgbColors[]){
    for ( int i = 0; i < NUMPIXELS; i++ ) {
      int rIndex = i * 3;
      int gIndex = (i * 3) + 1;
      int bIndex = (i * 3) + 2;
      int thisR = rgbColors[rIndex];
      int thisG = rgbColors[gIndex];
      int thisB = rgbColors[bIndex];
      pixels.setPixelColor(i, pixels.Color(thisR,thisG,thisB));
    }
    pixels.show();
    delay(100);
}

// Save everything in the memory
void saveColorsEEPROM(int rgbColors[], int quantityOfColors){
  EEPROM.write(0, quantityOfColors);
  EEPROM.commit();
  delay(100);
  int spacesInMemory = NUMPIXELS * 3;
  for ( int i = 0; i < spacesInMemory; i++ ) {
    int index = i + 1;
    int tempColor = rgbColors[i];
    // Serial.print(tempColor); Serial.print("|");
    EEPROM.write(index, tempColor);
    EEPROM.commit();
    delay(20);
  }
  Serial.println("Colors were saved in EEPROM");
}

// Return the DEC value of and Hex String
int hexToInt (String hex){
    if(hex == "0"){return 0;}
    if(hex == "1"){return 1;}
    if(hex == "2"){return 2;}
    if(hex == "3"){return 3;}
    if(hex == "4"){return 4;}
    if(hex == "5"){return 5;}
    if(hex == "6"){return 6;}
    if(hex == "7"){return 7;}
    if(hex == "8"){return 8;}
    if(hex == "9"){return 9;}
    if(hex == "a"){return 10;}
    if(hex == "b"){return 11;}
    if(hex == "c"){return 12;}
    if(hex == "d"){return 13;}
    if(hex == "e"){return 14;}
    if(hex == "f"){return 15;}
}

// Return a 0 to 255 DEC value of 2 Hex Strings
int hexToInt256(String channelHex){
  String firstChar = channelHex.substring(0,1);
  String secondChar = channelHex.substring(1,2);
  int int256 = hexToInt(firstChar)*16 + hexToInt(secondChar);
  return int256;
}
