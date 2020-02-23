#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <WebServer.h>

#define LEN 10

const char* ssid     = "Heslo za pivo";
const char* password = "neviem12";

WebServer server(80);

BLEScan* pBLEScan; //Name the scanning device as pBLEScan
BLEScanResults foundDevices;

int address = 0;

void handleScanBLEDevices(){
	foundDevices = pBLEScan->start(3);
	int length = foundDevices.getCount();
	JsonObject nestedObj;
	int pages = length/(LEN + 1) +1;
	if(length > LEN){
		length = LEN;
	}

	const int capacity = JSON_ARRAY_SIZE(LEN) + (1+LEN)*JSON_OBJECT_SIZE(2);
	StaticJsonDocument<capacity> doc;
	doc["pages"] = pages;
	JsonArray devices = doc.createNestedArray("devices");
	for (int i = 0; i < length; i++){
		nestedObj = devices.createNestedObject();
		nestedObj["address"] = foundDevices.getDevice(i).getAddress().toString();
		nestedObj["name"] = foundDevices.getDevice(i).getName();
	}
	server.send(200, "json", doc.as<String>());
}

void handleSetNewBLE(){
	if (server.args() != 1){
		server.send(401, "text/plain", "Bad request");
		return;
	}
	//server.argName(0);
	String My_BLE_Address = server.arg(0);
	int cnt = foundDevices.getCount()-1;
	static BLEAddress *Server_BLE_Address;
	String Scaned_BLE_Address;
	while (cnt >= 0){
		Server_BLE_Address = new BLEAddress( foundDevices.getDevice(cnt).getAddress());
		Scaned_BLE_Address = Server_BLE_Address->toString().c_str();
		if (Scaned_BLE_Address == My_BLE_Address){
			Serial.println("Found Device :-)... connecting to Server as client");
			if (connectToserver(*Server_BLE_Address)){
				Serial.println("Connected");
				break;
			}else{
				Serial.println("Pairing failed");
				break;
			}
		}
	}
}

void handleNotFound() {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}

void setup(void) {	
	Serial.begin(115200);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.println("");
	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	server.on("/scan", handleScanBLEDevices);
	server.on("/setble", handleSetNewBLE);
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");

	BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setActiveScan(true);

	if (!EEPROM.begin(1000)) {
    	Serial.println("Failed to initialise EEPROM");
	}

  	EEPROM.writeByte(address, 0);// store number of devices
  	address += sizeof(byte);
}

void loop(void) {
	server.handleClient();
}