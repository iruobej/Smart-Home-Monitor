#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

//unsigned long selectPressTime = 0;
//unsigned long selectReleaseTime = 0;
int emptyDeviceIndex = -1;
int displayIndex = 0;
bool down = false;
bool up = false;
class Device {
  public:
    bool isitnull = true;
    char id [4]; 
    char location [16];
    char type;
    bool state = false;
    int powerTemp = 0;
    Device(char id [], char location [], char type, bool state, int powerTemp) {
      strcpy(this->id , id);
      strcpy(this->location, location);
      this->type = type;
      this->state = state;
      this->powerTemp = powerTemp;
    }
    //IN setup: if type = S/O/L/T/C, then that call constructor on S/O/L/T/C class

    void setId(char id){
      strcpy(this->id , id);
    }
  

    char* getId() {
      return id;
    }

    void setLocation(char location){
      strcpy(this->location, location);
    }

    char* getLocation() {
      return location;
    }

    void setType(char type){
      strcpy(this->type, type);
    }

    char getType() {
      return type;
    }

    void setState(bool state){
      this->state = state;
    }

    bool getState() {
      return state;
    }

    void setPowerTemp(int powerTemp){
      this->powerTemp = powerTemp;
    }

    int getPowerTemp() {
      return powerTemp;
    }

};

Device *devices[10];

unsigned long selectButtonPressedTime = 0;


enum States {
  SYNCHRONISING,
  STANDARD_DISPLAY,
  STUDENT_ID
};

States state = SYNCHRONISING;

//Making custom arrows:
byte ArrowUp[] ={
  B00100,
  B01110,
  B11111,
  B11111,
  B11011,
  B10001,
  B00000,
  B00000
};
byte ArrowDown[] ={
  B00000,
  B00000,
  B10001,
  B11011,
  B11111,
  B11111,
  B01110,
  B00100
};

//To sort the devices in the array, putting all the null pointers at the end
void bubbleSort()
{
  //Serial.println("List is sorted");
  int n = 0;
  for (int i = 0; i < 10; i++){
    if (devices[i] == NULL){
      break;
    }
    n++;
  }  
   bool swapped = true;
   int j = 0;
   Device *tmp;

   while (swapped)
   {
      swapped = false;
      j++;
      for (int i = 0; i < n - j; i++)
      {
        if ((devices[i]==nullptr) && (devices[i+1]!=nullptr))
         {
            tmp = new Device(devices[i]->id, devices[i]->location, devices[i]->type, devices[i]->state, devices[i]->powerTemp);  
            devices[i] = devices[i + 1];
            devices[i + 1] = nullptr;
            swapped = true;
         }
         else if ((strcmp(devices[i]->id, devices[i + 1]->id) > 0))
         {
            tmp = new Device(devices[i]->id, devices[i]->location, devices[i]->type, devices[i]->state, devices[i]->powerTemp);  
            devices[i] = devices[i + 1];
            devices[i + 1] = tmp;
            swapped = true;
         }
      }
   }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.createChar(0, ArrowUp);
  lcd.createChar(1, ArrowDown);

  //Fill the array with objects with null values
for(int i = 0; i<10; i++){
    devices[i] = nullptr;    
}
}

void displayDevices(){
  //Serial.println("Devices displayed");
  lcd.clear();
  if((displayIndex >= 0) && (devices[displayIndex-1] != nullptr)) {     
    lcd.write(byte(0));
  }else{
    lcd.write(' ');
  }
  lcd.setCursor(1, 0);
  lcd.print(devices[displayIndex]->id);
  lcd.setCursor(5, 0);
  lcd.print(devices[displayIndex]->location);
  lcd.setCursor(0, 1);
  if((displayIndex < 9) && (devices[displayIndex+1] != nullptr)) {
    lcd.write(byte(1));
  }else{
    lcd.write(' ');
  }
  String isiton = devices[displayIndex]->getState() ? "ON" : "OFF";
  if(devices[displayIndex]->getState()){
    lcd.setBacklight(2);
  }else{
    lcd.setBacklight(3);
  }
  switch(devices[displayIndex]->getType()){
    case 'T':
      lcd.print("T " + isiton + " " + String(devices[displayIndex]->getPowerTemp()) + " " + char(223) + "C");
      break;
    case 'C':
      lcd.print(String(devices[displayIndex]->getType()) + " " + isiton);
      break;
    case 'O':
      lcd.print(String(devices[displayIndex]->getType()) + " " + isiton);
      break;
    case 'S':
      lcd.print(String(devices[displayIndex]->getType()) + " " + isiton + " " + String(devices[displayIndex]->getPowerTemp()) + "%");
      break;
    case 'L':
      lcd.print(String(devices[displayIndex]->getType()) + " " + isiton + " " + String(devices[displayIndex]->getPowerTemp()) + "%");
      break;
    default:
      lcd.print(String(devices[displayIndex]->getType()) + " " + isiton + " " + String(devices[displayIndex]->getPowerTemp()) + "%");
      break;
      
  }
}



void loop() {
 
  // put your main code here, to run repeatedly:
  bool present;
  Device* newDevice = nullptr;
  switch (state){
    case SYNCHRONISING:
    {
        lcd.setBacklight(5);
        Serial.print(F("Q"));
        delay(1000);
        if(Serial.available()>0){
          char str = Serial.read();
          /*if(str == "\\n" || str == "\\r"){
            Serial.print("Error: Carriage return and newline inputs are not allowed"); 
          } else */
          if (str == 'X'){
            lcd.setBacklight(7);
            Serial.print(F("UDCHARS,FREERAM\n"));
            state=STANDARD_DISPLAY;
          }      
        }
      //}
      
    }
      break;
    case STANDARD_DISPLAY:
    {
      String command;
      String commandid;
      String dtype;
      String location;
      String setterid;
      String setterstate; 
      char dtypechar;     
        if(Serial.available() > 0){
          char commandidarr [4];
          char locationarr [16];
          command = Serial.readString();
          command.trim();
          commandid = command.substring(2,5);
          dtype = command.substring(6,7); 
          dtypechar = dtype.charAt(0); 
          location = command.substring(8, command.length());
          location.toCharArray(locationarr, 15);
          commandid.toCharArray(commandidarr, 4);
        } 
          //write here
          if (command.indexOf("A") == 0) { 
            if(location.length() == 0 || commandid.length() != 3 || dtype.length() != 1){
              Serial.print(F("ERROR: "));
              Serial.println(command);
            }else{
              bubbleSort();
            
            char commandidarr [4];
            char locationarr [16];
            commandid.toCharArray(commandidarr, 4);
            location.toCharArray(locationarr, 16);
            if(dtype == "T"){
              newDevice = new Device(commandidarr, locationarr, dtypechar, false, 9);
            } else if (dtype == "S" || "O" || "L" || "C"){
              newDevice = new Device(commandidarr, locationarr, dtypechar, false, 0);
            } else {
              Serial.print(F("ERROR: "));
              Serial.println(command);
            }
            // Find an empty slot in the array and add the new device
            bubbleSort();
            for(int i = 0; i < 10; i++) {
              if(devices[i] == nullptr) {
                devices[i] = newDevice;
                break;
              }
            }    
            }       
              displayDevices();
            } else if (command.indexOf("S") == 0){           
              setterid = command.substring(2,5);           
              setterstate = command.substring(6);
               if(setterid.length() != 3 || (setterstate != "ON" && setterstate != "OFF")) {
                Serial.print(F("ERROR: "));
                Serial.println(command);
              } else {
                for (int i=0; i<10; i++){
                if(String(devices[i]->id) == setterid){
                  if(setterstate == "ON"){
                    devices[i]->setState(true);
                  } else if (setterstate == "OFF") {
                    devices[i]->setState(false);        
                  } else {
                    Serial.println(F("Invalid command"));
                  }
                } 
                displayDevices();
              }
            }
          } else if (command.indexOf("P") == 0) {
            String deviceId = command.substring(2, 5);
            int power = command.substring(6).toInt();
            if(deviceId.length() != 3 || power < 0 || power > 100) {
              Serial.print(F("ERROR: "));
              Serial.println(command);
            } else {
          
              for (int i = 0; i < 10; i++) {
              if (String(devices[i]->id) == deviceId) {
                if (devices[i]->type == 'S' || devices[i]->type == 'L') { // For speaker and light
                  if (power >= 0 && power <= 100) {
                    devices[i]->setPowerTemp(power);
                  } else {
                    Serial.println(F("Invalid command: power should be between 0 and 100 for speaker and light"));
                  }
                } else if (devices[i]->type == 'T') { // For thermostat
                  if (power >= 9 && power <= 32) {
                    devices[i]->setPowerTemp(power);
                  } else {
                    Serial.println(F("Invalid command: temperature should be between 9 and 32 for thermostat"));
                  }
                } else {
                  Serial.println(F("Invalid command: power command not supported for this device type"));
                }
              }
            }
            displayDevices();
            }
          } else if (command.indexOf("R") == 0) {
            bubbleSort();
              String deviceId = command.substring(2, 5);
              if(deviceId.length() != 3) {
                Serial.print(F("ERROR: "));
                Serial.println(command);
              } else {
    
                for(int i = 0; i < 10; i++) {
                if(devices[i] != nullptr && String(devices[i]->id) == deviceId) {
                  delete devices[i]; // Free the memory allocated for the device
                  devices[i] = nullptr; // Set the pointer to null
                  bubbleSort();
                  break;
                }
              }
              displayDevices(); 
              }    
          }

         uint8_t pressedButtons = lcd.readButtons();
          if ((selectButtonPressedTime) < (millis()-2000)) {
            if (BUTTON_SELECT & pressedButtons) {
              // The SELECT button was just pressed; record the current time
              selectButtonPressedTime = millis();

              uint8_t pressedButtons2 = lcd.readButtons();
              if (pressedButtons == pressedButtons2){
                state = STUDENT_ID;
              }
            }
          //} else if (selectButtonPressedTime > millis()) {
            // // The SELECT button was just released; check how long it was held
            // if (millis() - selectButtonPressedTime > 1000) {
            //   // The button was held for more than one second; switch to the STUDENT_ID state
            //   state = STUDENT_ID;
           // }
  // Reset the button press time
  //selectButtonPressedTime = millis();
}
    

        // Wait for half a second
        //delay(500);  
        
          if(pressedButtons & BUTTON_UP){
            //Serial.println("Up button pressed");
            if((displayIndex >= 0) && (devices[displayIndex-1] != nullptr)) {             
              up=true;
              displayIndex--;
              delay(500);
              displayDevices();
            }
          }

          if(pressedButtons & BUTTON_DOWN){
            //Serial.println("Down button pressed");
            if((displayIndex < 9) && (devices[displayIndex+1] != nullptr)) {
              down=true;
              displayIndex++;
              delay(500);
              displayDevices();
            }
          }
            
    }
      break;
   case STUDENT_ID:
   {    
     //Serial.println("Entering Student ID State");
      lcd.clear();
      lcd.setBacklight(5);
      lcd.print("F231061");
      lcd.setCursor(0, 1);
      lcd.print("Free SRAM: ");
      lcd.setCursor(11, 1);
      lcd.print(freeMemory());
      uint8_t pressedButtons = lcd.readButtons();
      //delay(1000);
      while(true){
        uint8_t pressedButtons = lcd.readButtons();
        
        if (!(pressedButtons & BUTTON_SELECT)) {
        state = STANDARD_DISPLAY;
        displayDevices();

        break;
        }
      }
   }
    break;
  }
}



