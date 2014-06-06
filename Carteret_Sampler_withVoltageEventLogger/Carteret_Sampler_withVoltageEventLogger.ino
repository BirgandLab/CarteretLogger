/*

This program controls 4 valves and a pump initialized
by the SCAN probe

Base program created on 25 May 2011
by Brad Smith

Altered by Randall Etheridge
Modified slightly by Kyle Aveni-Deforge
  adding data logging, clock, and float switch to reduce wear on pump and pump tubing.
Sequence (with Manta and water cleaning):
Measurement path is dry between measurements
Pump water from stream
Take measurement
Open drain valve
Open freshwater valve
Pump freshwater over windows
Pump = valve 3
Close freshwater valve
After delay purge Manta2

*/



#include <Wire.h>
#include "Chronodot.h"
#include <SD.h> 

Chronodot RTC;
 
   const int chipSelect = 10; 
   
// This assigns pin numbers to each component
const int probe = 9;
const int pump = 3;
const int purge = 4;
const int drainUnderScan = 5;
const int valve2 = 6;
const int wwPump = 7; //rinse pump
const int pumpValve = 8; //check valve for rinse pump
const int batteryVoltage = A1;
const int test=A0;
const int floatSwitchExcitation =A2;
const int floatSwitchSignal=A3;
//const int thermometer = A0;

// This initializes variables for counting the
// signal duration from the probe and setting the temperature.
int ProbeSignal = 0;
int count = 0;
int TestSignal = 0;
long val = 0;
long temp = 0;

// Set durations here (in seconds):
int SignalTime = 2;
const long int PumpTime = 10;  //seconds to pump from water source toward probe--during this part of the cycle the drain valve is open
const long int pumpToProbe=5;  //seconds to fill the sample resivoir --the drain valve is closed
const long int PurgeTime= 10;   //return water in lines to source
const long int MeasurementTime = 3; //seconds to wait for scanProbe to sample (counted after pumpToProbe is completed)
const long int lensCleaningTime = 1500; //milliseconds
const long int drainTime=3;  //seconds to leave the drain open after purging to allow maximum water draining (especially when manta is connected)
const long int mantaTime=1; //can this be zero?
long int startTime;
long int elapsedTime;
 //  int runSystem=0;
   char systemLogFile[12] = "test.log";
   int sampleCyclesSinceLastPowerCycle=0;
   int numberOfSamples=0;
void setup() {
  Serial.begin(9600);
  analogReference(INTERNAL); 

  pinMode(probe, INPUT);
  pinMode(batteryVoltage,INPUT);
  pinMode(pump, OUTPUT);
  pinMode(purge, OUTPUT);
  pinMode(drainUnderScan, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(wwPump, OUTPUT);
  pinMode(pumpValve, OUTPUT);
  pinMode(test, INPUT_PULLUP);
  pinMode(floatSwitchExcitation,OUTPUT);
  digitalWrite(floatSwitchExcitation,LOW);
  pinMode(floatSwitchSignal,INPUT);
  
   pinMode(chipSelect,OUTPUT);
   
   Serial.println("Initializing Chronodot.");
  Wire.begin();
  RTC.begin();

    

    
    digitalWrite(chipSelect,HIGH);   //Tell the SD Card it is needed
       if (!SD.begin(chipSelect)) {//IF THE SD DOES NOT START
          Serial.println("Card failed, or not present");
           } //END  if (!SD.begin(chipSelect))
           timeStamp();
  
}

void loop() {
    
    if (get_unixtime()%10==0){
    timeStamp();
    Serial.print("Sample Cycles since last power cycle: ");
    Serial.println(sampleCyclesSinceLastPowerCycle);
    Serial.print("float switch status: ");
    Serial.println(get_floatSwitch());
    
  //    writeSystemLogFile(numberOfSamples);
    delay(1000);
  }
  
    ProbeSignal = digitalRead(probe);
    TestSignal = digitalRead(test);   // sets the variable ProbeSignal to HIGH (on) or LOW (off)
    //val = analogRead(thermometer);
    //temp = (5*val*100/1024);

while((ProbeSignal == HIGH)|| (TestSignal == LOW)){// This is a while loop that counts the duration that the probe
     count++;                         // is sending a signal. While the probe is sending a signal, the
    delay(100);                      // Arduino will add 1 to the variable 'count' every .1 seconds 
    ProbeSignal = digitalRead(probe);// (written as 100 milliseconds). You must tell the Arduino to
    TestSignal = digitalRead(test);  // re-check the status of the probe signal within the loop or the
                                     // loop will continue counting indefinitely no matter the status of the signal.
      Serial.println("signal");
    if((count == SignalTime*10)){
            if(get_floatSwitch()==1){
             int runSystem=0;   //there is not water in the ditch
              writeSystemLogFile(runSystem);
            }
            
            if(get_floatSwitch()==0){
              sampleCycle();
              int runSystem=1;
              writeSystemLogFile(runSystem);
            }
    }//if count=signalTime*10
  }//while probeSignal==high or test==low
  count = 0; //Reset signal duration counter to 0 to avoid
             //count building from several short signals
  
  
} //end void loop


unsigned long int get_unixtime(){
    DateTime  now = RTC.now();  //DECLARE A RTC OBJECT
	unsigned long int time = now.unixtime();
	return time;			//RETURN  TIME IN SECONDS SINCE 1/1/1970 00:00:00
}//END unsigned long int get_unixTime()


void writeSystemLogFile(int runSystem){
//PREPARED DATA FOR FILE HEADER ABBREVIATIONS OF VARIABLES AND UNITS ARE HERE 
//EVERYTHING IS TAB DELIMITED, SO EXCEL SHOULD READ IT IN WELL
//MUST BE CHANGED IF THE OUTPUTS ARE CHANGED
    char* registerNames[]={"YYYY","MM","DD","HH","mm",
                          "runPump","sample#","voltage","raw","temperature"};
                          
          
	if (!SD.exists(systemLogFile)){ 					//IF THE LOG FILE DOES NOT EXIST
		File dataFile=SD.open(systemLogFile,FILE_WRITE);//CREATE THE LOG FILE

                for (int j=0;j<7;j++){							//FOR EACH LOG VARIABLE
                      dataFile.print(registerNames[j]);			//PRINT A SHORT VARIABLE NAME
                      dataFile.print("\t");      				//ADD A TAB
                      }//END for (int j=0;j<30;j++)
                      
                dataFile.print("\n");		
                      						//ADD A NEWLINE
               dataFile.close();   								//CLOSE THE FILE
       }//END 	if (!SD.exists(systemLogFile))
  
  
      DateTime now = RTC.now();				//PROVIDE CURRENT TIME FOR LOG FILE
  
      File dataFile=SD.open(systemLogFile, FILE_WRITE);   //open log file	  
        
        if (dataFile){			 //IF THE DATA FILE OPENED WRITE DATA INTO IT
			char dataString[27]; //A CONTAINER FOR THE FORMATTED DATE
			
			//PRINT THE DATE (YYYY MM DD HH mm SS) TO dateString variable
			int a = sprintf(dataString,"%d\t%02d\t%02d\t%02d\t%02d\t%02d\t",now.year(), 
					now.month(), now.day(),now.hour(),now.minute(),now.second());
			
				
			dataFile.print(dataString);	//WRITE THE DATE STRING TO THE LOG FILE	
			
		        dataFile.print("\t");	 	//ADD A TAB
			
			dataFile.print(runSystem);         //WRITE NUMBER OF SAMPLES 
			dataFile.print("\t");					//ADD A TAB
           
			dataFile.print(numberOfSamples);  
                        dataFile.print("\t"); 
                        dataFile.print(get_vBatt());					//ADD A TAB
                        dataFile.print("\t"); 
                        dataFile.print(analogRead(batteryVoltage));  			//ADD A TAB
                        dataFile.print("\t"); 
                        dataFile.print( get_tempRTC());   
	   		dataFile.print("\n");  		//add final carriage return
          
          dataFile.close();				//close file
          Serial.println("successful File Writing");
         // Serial.println(dataString);
		}//END if (dataFile)
    
  
}//END void writeSystemLogFile(float waterDepth, float collect, int numberOfSamples)

///PROVIDES SERIAL OUTPUT ABOUT TIME, TEMPERTURE AND THE LIKE
///GOOD FOR MAKING SURE RTC IS RUNNING AND CYCLES ARE EXECUTING ACCORDING TO PLAN
void timeStamp(){
  DateTime  now = RTC.now();         //start clock object "now"
  //Serial.println(timestamp);
 Serial.print(now.year(), DEC);
    Serial.print('/');
    if(now.month() < 10) Serial.print("0");
    Serial.print(now.month(), DEC);
    Serial.print('/');
    if(now.day() < 10) Serial.print("0");
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    if(now.hour() < 10) Serial.print("0");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    if(now.minute() < 10) Serial.print("0");
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    if(now.second() < 10) Serial.print("0");
    Serial.print(now.second(), DEC);
    Serial.print("\t");

    Serial.print(now.tempC(), 1);
    Serial.println(" degrees Celcius");
    //Serial.println(digitalRead(vBat));
    Serial.print("voltage: ");
    Serial.println(get_vBatt());
    Serial.println(analogRead(batteryVoltage));
    //Serial.print(now.tempF(), DEC);
    //Serial.println(" degrees Farenheit");
    
    Serial.println();
}//END timeStamp() FUNCTION

float get_vBatt(){
 float vBatt = (analogRead(batteryVoltage));	//READ BATTERY VALUE
 float vBattF = (vBatt/1023*14.0*1.1);
 return vBattF;			//RETURN CONVERTED TO VOLTS
}//END float get_vBatt()

float get_tempRTC(){
	DateTime now = RTC.now();//DECLARE A RTC OBJECT
	float tempC = now.tempC();
	return tempC;			//RETURN THE TEMPERATURE
	
}//END float get_tempRTC()


//FUNCTION TO READ THE STATUS OF THE FLOAT SWITCH
int get_floatSwitch(){
  digitalWrite(floatSwitchExcitation,HIGH);
  delay(100);
  int waterPresent=digitalRead(floatSwitchSignal);  
  return waterPresent;
}


void sampleCycle(){
    Serial.println("enterSampleCycle");
   //   writeSystemLogFile(runSystem);
 //RUN PUMP AND OPEN DRAIN UNDER SCAN   
          startTime=millis();
          digitalWrite(drainUnderScan, HIGH); //Open valve
          digitalWrite(pump, HIGH);   //Turn pump on
          Serial.println("Pumping");
              while (millis()-startTime<PumpTime*1000){ //fill time while pumping
               delay(100); 
              }
 //pump to probe
          digitalWrite(drainUnderScan, LOW);  //Close the valve so water accumulates
          Serial.println("Pump To Probe");
          long int timer=millis();
          while (millis()-timer<pumpToProbe*1000){
             delay(100);
            }
      
          digitalWrite(pump,LOW);     //Turn pump off
          
//wait for scan to finish measurement          
          timer=millis();
              while (millis()-timer<MeasurementTime*1000){
                delay(100);
                }

//drain the sample sleeve
      digitalWrite(drainUnderScan, HIGH); //Open valve 1
      Serial.println("Open Drain");
            timer=millis();
            while (millis()-timer<drainTime*1000){
               delay(100);
               }      //Delay 3 seconds for draining
               
 //Wash the lenses
       digitalWrite(pumpValve, HIGH); //Open valve 4 open check valve
       delay(200);                 //Wait .2 seconds to avoid system damage
       digitalWrite(wwPump, HIGH); //Open valve 3 turn on pump
        Serial.println("Cleaning lens");
         timer=millis();
          while (millis()-timer<lensCleaningTime){
              delay(100);
              }   
      
      digitalWrite(wwPump, LOW);  //turn off pump
      delay(200);                 //Wait .2 seconds to avoid system damage
      digitalWrite(pumpValve, LOW);  //Close valve

//wait for manta to take measurement

      Serial.println("Delay for Manta");
      timer=millis();
      while (millis()-timer<mantaTime*1000){
          delay(100);
        } 
      
//purge lines         
      digitalWrite(purge, HIGH);  //Reverse pump
      Serial.println("Purging");
      
      timer=millis();
      while (millis()-timer<PurgeTime*1000){
           delay(100);
        } 
      
      digitalWrite(purge, LOW);   //Turn off pump
      digitalWrite(drainUnderScan, LOW);  //Close valve 1 //drain valve
      Serial.println("Waiting... ");
      numberOfSamples++;
  }//if get_floatSwitch==0
