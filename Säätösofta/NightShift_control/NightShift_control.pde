import processing.serial.*;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.List;

int bgColor = 30;
// xpos, ypos, corner radiuses (tl, tr, br, bl)
int[] minusButton, plusButton, removeGearButton, addGearButton;
int buttonSize, buttonSize2, buttonPadding;
int leftPadding = 50;
int screenWidth = 500;
int screenHeight = 400;
int maxGearLimit = 11;
int minGearLimit = 5;
int maxGearHeight = 200;
int gearSizeDifference = 15;

int gearsInCassette = 7;
int selectedGear = 1;
int previousGear;

int connectionState = 0;
String connectingString, notConnectedString, connectedString;
float connectingStringW, notConnectedStringW, connectedStringW;

Serial NightShiftPort;
int serialEventTrigger = 10; // ascii line feed
String serialInString;  // input string from serial port
String previousSerialInString;
String hardCodedComPort = "COM33";

void setup() {
  
  size(screenWidth, screenHeight);
  background(bgColor);
  textAlign(CENTER);
  fill(255);
  
  textSize(24);  
  text("Night shift control software", width/2,24);
  
  setButtonPositions();
  setConnectionTextVariables();
}

/**
 * Automaattinen sarjaportin ja asetusten tunnistus
 * sarjaportin luomista varten: new Serial(this, portti, baudrate);
 */
public void autoDetectSerial() {
  
  String keyPath ="SYSTEM\\CurrentControlSet\\Enum\\USB\\";
  // todo: find correct usb / com device key / friendlyname / description
  // "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Ports" listaa COM -portit ja niiden viimeiset tunnetut asetukset, mm. baudrate
  // "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\BTHENUM" listaa bluetooth -laitteet (teksti COM FriendlyNamessa = laite on kytketty)
  
  try {
    List<String> subKeys = WinRegistry.readStringSubKeys(WinRegistry.HKEY_LOCAL_MACHINE, keyPath);
    List<String> subKeys2;
    for (String subKey : subKeys) 
    {      
      subKeys2 = null;
      if (subKey != null) {
        subKeys2 = WinRegistry.readStringSubKeys(WinRegistry.HKEY_LOCAL_MACHINE, keyPath+ subKey);
        if (subKeys2 != null) {
          for (String subSubKey : subKeys2) 
          {
    //        println(" "+subSubKey);
            String friendlyName = getFriendlyName(keyPath + subKey + "\\" + subSubKey);
            String deviceDesc = getDeviceDesc(keyPath + subKey + "\\" + subSubKey);
            if (friendlyName != null)// && friendlyName.contains("COM33")) 
            {
              println("Found key: "+subKey);
              println("    "+friendlyName);
              if (deviceDesc != null) 
              {
                println("    "+deviceDesc);
              }
              // todo: dig the actual values from received info
              setupSerial(hardCodedComPort);
              return;
            }
          }  
        }
      }
    }
  } catch (Exception ex) { // catch-all: 
        // readString() throws IllegalArg, IllegalAccess, InvocationTarget
        System.err.println(ex.getMessage()); 
  }
}

// Given a registry key, attempts to get the 'DeviceDesc' value
// Returns null on failure.
//
public static String getDeviceDesc(String registryKey) {
    if (registryKey == null || registryKey.isEmpty()) {
        throw new IllegalArgumentException("'registryKey' null or empty");
    }
    try {
        int hkey = WinRegistry.HKEY_LOCAL_MACHINE;
        return WinRegistry.readString(hkey, registryKey, "DeviceDesc");
    } catch (Exception ex) { // catch-all: 
        // readString() throws IllegalArg, IllegalAccess, InvocationTarget
        System.err.println(ex.getMessage());
        return null;
    }
}

// Given a registry key, attempts to get the 'FriendlyName' value
// Returns null on failure.
//
public static String getFriendlyName(String registryKey) {
    if (registryKey == null || registryKey.isEmpty()) {
        throw new IllegalArgumentException("'registryKey' null or empty");
    }
    try {
        int hkey = WinRegistry.HKEY_LOCAL_MACHINE;
        return WinRegistry.readString(hkey, registryKey, "FriendlyName");
    } catch (Exception ex) { // catch-all: 
        // readString() throws IllegalArg, IllegalAccess, InvocationTarget
        System.err.println(ex.getMessage());
        return null;
    }
}

// Given a registry key, attempts to parse out the integer after
// substring "COM" in the 'FriendlyName' value; returns -1 on failure.
// TODO: improve this code to match 2-digit com ports
public static int getComNumber(String registryKey) {
    String friendlyName = getFriendlyName(registryKey);

    if (friendlyName != null && friendlyName.indexOf("COM") >= 0) {
        String substr = friendlyName.substring(friendlyName.indexOf("COM"));
        Matcher matchInt = Pattern.compile("\\d+").matcher(substr);
        if (matchInt.find()) {
            return Integer.parseInt(matchInt.group());
        }
    }
    return -1;
}   

/**
 * Method to set up the serial connection
 */
void setupSerial(String comPort) {
  NightShiftPort = new Serial(this, comPort, 9600);
  NightShiftPort.bufferUntil(serialEventTrigger);  
}

/**
 * Serial port event, triggered with ASCII line feed char(10)
 * in serial output
 */
void serialEvent(Serial p) {
  serialInString = p.readString();
  p.clear();
}


void setButtonPositions() {
  buttonSize = 40;
  buttonSize2 = 30;  
  buttonPadding = 10;

  minusButton = new int[] { ((width/2)-buttonSize-buttonPadding), (maxGearHeight+80+50), 12,12,1,12};
  plusButton = new int[] { ((width/2)+buttonPadding), (maxGearHeight+80+50), 12,12,12,1};
  
  removeGearButton = new int[] {0, 55-buttonSize2/2, 1,12,12,1 };
  addGearButton = new int[] {width, 55-buttonSize2/2, 12,12,1,12 };
    
}



void setConnectionTextVariables() {
  textSize(14);
  connectingString = "connecting";
  connectingStringW = textWidth(connectingString);
  
  notConnectedString = "not connected";
  notConnectedStringW = textWidth(notConnectedString);
  
  connectedString = "connected";
  connectedStringW = textWidth(connectedString);
}


void draw() {
  checkConnectionState();
  drawGears();
  drawDerailleur();
  drawButtons();
  drawConnectionState();
  handleReceivedSerialMsgs(); // debug
}

/**
 * Quick method to dump messages received from serial
 */
void handleReceivedSerialMsgs() {
  
  if (connectionState == 1 && serialInString != null) {
    shout(serialInString);
    serialInString = null;
  }    
}
/**
 * Method to check if connection is initialized / still alive
 */
void checkConnectionState() {
  
  if (NightShiftPort == null) {
    connectionState = 0; // not connected
  } else if (NightShiftPort.available() > 0){ // bytes available in serial port
    connectionState = 1; 
  }
}

void mousePressed() {  
   // check for gear selection
   for (int i=0; i<=gearsInCassette;i++) {
     // gear position defined as follows
     int xSpread = (width-100) / gearsInCassette;
     int gearwidth = 18;
     int xpos = i*xSpread+(leftPadding/2)-(gearwidth/2);     
     
     // x, y, width, height, topLeft, topRight, bottomRight, bottomLeft
     // rect(xpos-9, 80, 18, 150-(i*15), 0,0,15,15); 
     if (mouseX >= xpos && mouseX <= xpos+gearwidth) {
        if (mouseY >= 80 && mouseY <= maxGearHeight+80-(i*15) ) {
          previousGear = selectedGear;
          selectedGear = i;
          shout("Gear "+i+" selected");
        }
     }
   }
   
   // check for buttons
   // adjustment minus button & plus button share same y-position
   if (mouseY >= minusButton[1] && mouseY <= minusButton[1]+buttonSize) {
     if (mouseX >= minusButton[0] && mouseX <= minusButton[0]+buttonSize) {     
       shout("Adjusting derailleur inwards");
     }
     if (mouseX >= plusButton[0] && mouseX <= plusButton[0]+buttonSize) {
       shout("Adjusting derailleur outwards");
     }
   }
   
   // check for add/remove gear buttons
   // gear add/remove buttons same share y-position
   if (mouseY >= removeGearButton[1] && mouseY <= removeGearButton[1]+buttonSize2) {
     if (mouseX >= removeGearButton[0] && mouseX <= removeGearButton[0]+buttonSize2) {
       if (gearsInCassette > minGearLimit) {
         shout("Removed one gear from the cassette");
         gearsInCassette--;
         
         if (selectedGear > gearsInCassette) {                      
           selectedGear = previousGear = gearsInCassette;
           shout("New largest gear "+selectedGear+" selected automatically");
         }
         stroke(bgColor); fill(bgColor);
         rect(0, 50, width, height);
       }
     }
     if (mouseX >= addGearButton[0]-buttonSize2 && mouseX <= addGearButton[0]) {
       if (gearsInCassette < maxGearLimit) { 
         shout("Added one gear to the cassette");
         gearsInCassette++;         
         stroke(bgColor); fill(bgColor);
         rect(0, 50, width, height);
       }
     }
   }
   
   // check for connection state button
   // positioned by longest string displayed, bounded by: 
   //   rect((width/5)-(notConnectedStringW/2)-2, height-45-20, notConnectedStringW, 24);
   if (mouseX >= width/5-notConnectedStringW/2-2 && mouseX <= width/5+notConnectedStringW/2) {
     if (mouseY >= height-45-20 && mouseY <= height-45) {       
       
       // todo: real-life connection state management & handshaking with arduino
       
       if (connectionState == 0) {         
         connectionState = 2; // connection state text to "connecting"
         drawConnectionState();
         //int timer = millis();
         //while (!((millis() - timer) >= 500)) {} // delay loop to actually see the text
         
         println(Serial.list());
         autoDetectSerial();  // serial port autodetection & connect 
         
       } else if ( connectionState == 2) {
         connectionState = 0; // connecting, user press -> cancel connection, not connected
         disconnectSerial();         
       }
       else if (connectionState == 1) {
         connectionState = 0; // connected, user press -> disconnect
         disconnectSerial();
       }
       
       /*
       if (connectionState <= 1) {                  
         connectionState++;
       } else {
         connectionState = 0;
       }
       */
       
       shout("Changed connection state to: "+connectionState);
     }
   }
   
}

/**
 * Method to gracefully disconnect serial device
 */
void disconnectSerial() {
  if (NightShiftPort != null) {
    NightShiftPort.stop();
  }
  NightShiftPort = null;
}

void shout(String text) {
  println(text);
}

/**
 * Draws UI buttons to the screen
 */ 
void drawButtons() {
   stroke(80); fill(255);
   strokeWeight(3);
   
   // adjustment buttons
   rect(minusButton[0],minusButton[1], buttonSize, buttonSize,
        minusButton[2],minusButton[3],minusButton[4], minusButton[5]);
   rect(plusButton[0],plusButton[1], buttonSize, buttonSize,
        plusButton[2],plusButton[3],plusButton[4], plusButton[5]);
        
   // add/remove gear buttons   
   bezier(removeGearButton[0], removeGearButton[1], 20, removeGearButton[1],
             20, removeGearButton[1]+buttonSize2, removeGearButton[0], removeGearButton[1]+buttonSize2);
   bezier(addGearButton[0], addGearButton[1], width-20, addGearButton[1],
            width-20, addGearButton[1]+buttonSize2, addGearButton[0], addGearButton[1]+buttonSize2);   
   strokeWeight(1);
   textSize(24);
   fill(80);
   
   // adjustment button texts
   text("-", minusButton[0]+buttonSize/2, minusButton[1]+27);
   text("+", plusButton[0]+buttonSize/2, plusButton[1]+27);
   
   // add/remove gear button texts
   text("-", removeGearButton[0]+5, removeGearButton[1]+22);
   text("+", width-5, removeGearButton[1]+22);
}

/**
 * Draws derailleur position icon
 */
void drawDerailleur() {  
  
  // todo: change derailleur y position according to real-life measurements
  // to optimize gear change in arduino end
  int distanceFromGear = 15;
  
  int xSpread = (width-(leftPadding*2)) / gearsInCassette;  
  int xpos = selectedGear*xSpread+(leftPadding/2)+9;
  int topY = maxGearHeight-(selectedGear*gearSizeDifference)+80+distanceFromGear; // gear height+distance from top+some space  
  int topX = xpos - 9;
  int bottomY = topY+20;
  
  int previousTopY = maxGearHeight-(previousGear*gearSizeDifference)+80+distanceFromGear; // gear height+distance from top+some space
  int previousTopX = previousGear*xSpread+(leftPadding/2);
  
  // first, draw black box over previous position indicator to hide it
  stroke(bgColor); fill(bgColor);
  rect(previousTopX-11, previousTopY-2, buttonSize2, buttonSize2);
  
  // todo: change triangle colour & position in relation to selected gear 
  // according to real-life servo position measurements from arduino
  
  // last, draw the triangle denoting derailleur position   
  stroke(100);
  triangle(topX, topY, topX+9, bottomY, topX-9,bottomY);  
}

/**
 * Function to draw gear image to screen
 */
void drawGears() {  
 
  textSize(14);   
  int xSpread = (width-(leftPadding*2)) / gearsInCassette;
  fill(255);  
    
  for (int i=gearsInCassette;i>0;i--) {
    if (i == selectedGear) {
      stroke(255,0,0);
    } else {
      stroke(100);
    }
    
    int xpos = i*xSpread+(leftPadding/2);
    // x, y, width, height, topLeft, topRight, bottomRight, bottomLeft
    rect(xpos-9, 80, 18, maxGearHeight-(i*gearSizeDifference), 0,0,15,15); 
    
    text(""+(i), xpos, 60); 
  }
  
  stroke(255); strokeWeight(4);
  strokeCap(ROUND);
  line(leftPadding, 80, width-leftPadding, 80);
  strokeWeight(1);
}

/**
 * Draws a string representing current connection state to the screen
 */ 
void drawConnectionState() {  
  textSize(14);
  stroke(bgColor);fill(bgColor);
  rect((width/5)-(notConnectedStringW/2)-2, height-45-20, notConnectedStringW, 24);
  if (connectionState == 0) { // not connected
    fill(200, 40, 40);    
    text(notConnectedString, width/5, height-45);
  }
  if (connectionState == 1) { // connected
    fill(10, 200, 40);    
    text(connectedString, width/5, height-45);
     
  }
  if (connectionState == 2) { // connecting
    fill(255, 180, 20);
    text(connectingString, width/5, height-45);
  }
}

 /*
void mouseReleased() {
  background(100, 0, 100); 
  text("mouse was released", width/2, height/2);
}
 
void mouseMoved() {
  background(150, 10, 70); 
  text("mouse was moved", width/2, height/2);
}
 
void mouseDragged() {
  background(10, 70, 100); 
  text("mouse was dragged", width/2, height/2);
}
*/
