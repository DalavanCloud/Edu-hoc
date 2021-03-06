#include "RFM12B_arssi.h"
#include "../common.h"

//radio module pins
#define RFM_CS_PIN  10 // RFM12B Chip Select Pin
#define RFM_IRQ_PIN 2  // RFM12B IRQ Pin

//jeenodes run on 3.3 V, this value is in mV
#define BOARD_VOLTAGE 3300
#define MAX 1000
RFM12B radio;

byte nodeID = 10;
byte pairID = 20;
uint16_t counter;
typedef struct {
  int           nodeId; //store this nodeId
  uint16_t      seqNum;    // current sequence number

} Payload;
Payload theData;

byte rssiValues[MAX];

void setup(){
  Serial.begin(57600);
  counter = 0;

  //radio present check
  if ( radio.isPresent( RFM_CS_PIN, RFM_IRQ_PIN) )
  //Serial.println(F("RFM12B Detected OK!"));
  //else
  //Serial.println(F("RFM12B Detection FAIL!"));

  //init rssi measurement - pin and iddle voltage
  radio.SetRSSI( 0, 450);

  //init radio (node id, freq., group id)
  radio.Initialize(nodeID, FREQUENCY, 200);
}

void displayRSSI(int8_t rssi){
  Serial.print(F("\nRSSI "));

  if (rssi == RF12_ARSSI_DISABLED )
  Serial.print(F("disabled")); // ARSSI was not enabled on sketch
  else if (rssi == RF12_ARSSI_BAD_IDLE )
  Serial.print(F("has bad idle settings")); // Vidle for ARSSI has incorrect value
  else if (rssi == RF12_ARSSI_RECV )
  Serial.print(F("gateway RF reception in progress")); // can't get value another packet is in reception
  else if (rssi == RF12_ARSSI_ABOVE_MAX )
  // Value above max limit, may be set up vidle is wrong
  Serial.print(F("above maximum limit (measure and set vidle on gateway sketch"));
  else if (rssi == RF12_ARSSI_BELOW_MIN )
  // Value below min limit, may be set up vidle is wrong
  Serial.print(F("below minimum limit (measure and set vidle on gateway sketch"));
  else if (rssi == RF12_ARSSI_NB_BYTES )
  {
    // We did not sent enough byte received to for accurate RSSI calculation
    Serial.print(F("not enough bytes ("));
    Serial.print(-(rssi-RF12_ARSSI_NB_BYTES));
    Serial.print(F(") sent to gateway to have accurate ARSSI"));
  }
  else
  {
    // all sounds good, display
    // display bargraph
    Serial.print(F("["));

    for (int k=RF12_ARSSI_MIN; k<=RF12_ARSSI_MAX; k++)
    Serial.print(rssi>=k ? '=' : ' ');

    Serial.print(F("] "));

    Serial.print(rssi);
    Serial.print(F(" dB"));
  }

}

void blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS);
  digitalWrite(PIN,HIGH);
}

//function for responder node
int receiveRSSI(){
  //wait for msg
  //save rssi
  //respond
  //repeat

  if (radio.ReceiveComplete()){
    if (radio.CRCPass()){
      int8_t rssi = radio.ReadARSSI(BOARD_VOLTAGE);
      byte thisNodeID = radio.GetSender();
      if(thisNodeID != pairID){
        //not the messsage we are waiting for
        return 0;
      }
      if (*radio.DataLen != sizeof(Payload)){
        return 0;
      }
      theData = *(Payload*)radio.Data; //assume radio.DATA actually contains our struct and not something else
      rssiValues[theData.seqNum] = rssi;

      //send same data back
      theData.nodeId = nodeID;
      radio.Send(pairID, (const void*)(&theData), sizeof(theData), true);
      if(theData.seqNum >= MAX -1){
        return -1;
      }
    }

  }
  return 0;
}


//function for initiating node
int sendRSSI(){
  if(counter == MAX){
    return -1;
  }
  //send
  //wait for response
  //save rssi
  //repeat or resend
  theData.nodeId = nodeID;
  theData.seqNum = counter;
  radio.Send(pairID, (const void*)(&theData), sizeof(theData), true);

  for(int i = 0; i < 100; i++){
    if (radio.ReceiveComplete()){
      if (radio.CRCPass()){
        int8_t rssi = radio.ReadARSSI(BOARD_VOLTAGE);

        byte thisNodeID = radio.GetSender();
        if(thisNodeID != pairID){
          //not the messsage we are waiting for
          continue;
        }
        if (*radio.DataLen != sizeof(Payload)){
          continue;
        }
        theData = *(Payload*)radio.Data; //assume radio.DATA actually contains our struct and not something else
        rssiValues[theData.seqNum] = rssi;
        counter++;
        //Serial.println(counter);
        //Serial.println(rssi);
        return 0;
    }
  }
  delay(10);
}
return 0;
}

void printAndStop(){
  for(uint16_t i = 0; i < MAX; i++){
    Serial.print(rssiValues[i]);
    Serial.print(";");
  }
  Serial.println("&&");
  while(true){
    delay(1000);
  }
}

void loop(){
  byte result = 0;
  if(nodeID < 15){
    result = sendRSSI();
  } else {
    result = receiveRSSI();
  }
  blink(9,10);
  //Serial.println(result);
  if( result != 0){
    printAndStop();
  }
}
