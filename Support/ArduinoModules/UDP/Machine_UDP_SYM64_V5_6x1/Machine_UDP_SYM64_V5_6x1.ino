    //Machine Control - Brian Tee - Cut and paste from everywhere


    //-----------------------------------------------------------------------------------------------
    // Change this number to reset and reload default parameters To EEPROM
    #define EEP_Ident 0x5422

    
    
    //the default network address
    struct ConfigIP {
        uint8_t ipOne = 192;
        uint8_t ipTwo = 168;
        uint8_t ipThree = 0;
    };  ConfigIP networkAddress;   //3 bytes
    //-----------------------------------------------------------------------------------------------

    #include <EEPROM.h> 
    #include <Wire.h>
    #include "EtherCard_AOG.h"
    #include <IPAddress.h>

    // ethernet interface board. Moved to board setup location all setup in one spot.
    //static uint8_t myip[] = { 0,0,0,123 };

    // gateway ip address
    static uint8_t gwip[] = { 0,0,0,1 };

    //DNS- you just need one anyway
    static uint8_t myDNS[] = { 8,8,8,8 };

    //mask
    static uint8_t mask[] = { 255,255,255,0 };

    //this is port of this autosteer module
    uint16_t portMy = 5123;

    //sending back to where and which port
    static uint8_t ipDestination[] = { 0,0,0,255 };
    uint16_t portDestination = 9999; //AOG port that listens

    // ethernet mac address - must be unique on your network
    static uint8_t mymac[] = { 0x00,0x00,0x56,0x00,0x00,0x7B };

    uint8_t Ethernet::buffer[200]; // udp send and receive buffer

    //Variables for config - 0 is false  
    struct Config {
        uint8_t raiseTime = 2;
        uint8_t lowerTime = 4;
        uint8_t enableToolLift = 0;
        uint8_t isRelayActiveHigh = 0; //if zero, active low (default)

        uint8_t user1 = 0; //user defined values set in machine tab
        uint8_t user2 = 0;
        uint8_t user3 = 0;
        uint8_t user4 = 10;

    };  Config aogConfig;   //4 bytes

    //Program counter reset
    void(*resetFunc) (void) = 0;

    //ethercard 10,11,12,13 Nano = 10 depending how CS of ENC28J60 is Connected
    #define CS_Pin 10

    /*
    * Functions as below assigned to pins
    0: -
    1 thru 16: Section 1,Section 2,Section 3,Section 4,Section 5,Section 6,Section 7,Section 8,
                Section 9, Section 10, Section 11, Section 12, Section 13, Section 14, Section 15, Section 16,
    17,18    Hyd Up, Hyd Down,
    19 Tramline,
    20: Geo Stop
    21,22,23 - unused so far*/    
    uint8_t pin[] = { 1,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };//24

    //read value from Machine data and set 1 or zero according to list
    uint8_t relayState[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //23

 //*********************************************************************************************************************
 // Setup your networked board in this location 
    
    //Step one, Choose 192.168.0.X ip address for board, each will need a unique number not used on the network.
    //Select by deleting the "//" from infront of "static uint8_t myip[]" only pick one, one must be chosen.
    
    //static uint8_t myip[] = { 0,0,0,123 }; uint8_t BoardRow = 0; uint8_t BoardColumn = 0;//Aog classic board
    
    static uint8_t myip[] = { 0,0,0,111 }; uint8_t BoardRow = 1; uint8_t BoardColumn = 1;//Symmetrical sections neworked board 1-1
    //static uint8_t myip[] = { 0,0,0,112 }; uint8_t BoardRow = 2; uint8_t BoardColumn = 1;//Symmetrical sections neworked board 2-1
    //static uint8_t myip[] = { 0,0,0,113 }; uint8_t BoardRow = 3; uint8_t BoardColumn = 1;//Symmetrical sections neworked board 3-1
    //static uint8_t myip[] = { 0,0,0,114 }; uint8_t BoardRow = 4; uint8_t BoardColumn = 1;//Symmetrical sections neworked board 4-1

    //static uint8_t myip[] = { 0,0,0,115 }; uint8_t BoardRow = 1; uint8_t BoardColumn = 2;//Symmetrical sections neworked board 1-2
    //static uint8_t myip[] = { 0,0,0,116 }; uint8_t BoardRow = 2; uint8_t BoardColumn = 2;//Symmetrical sections neworked board 2-2
    //static uint8_t myip[] = { 0,0,0,117 }; uint8_t BoardRow = 3; uint8_t BoardColumn = 2;//Symmetrical sections neworked board 3-2
    //static uint8_t myip[] = { 0,0,0,118 }; uint8_t BoardRow = 4; uint8_t BoardColumn = 2;//Symmetrical sections neworked board 4-2

    //Step 4, Choose Time=1 or Distance=2 based section trip
    //value of user3 = 0 Column 1 (not used yet)
    //value of user4 = 0 Column 2 (not used yet)
    
    uint8_t ClmnOneTripType = 1;
    uint8_t ClmnTwoTripType = 1;
    
    
    
   //Extra vairables for Symmetric sections

    
    uint32_t relayStateSS[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
                                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //32 
                                
    uint32_t relayStateSS2[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //32                                

    uint32_t relaytriggerSS[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //32                                  

    uint32_t relaytriggerSS2[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //32                                                         

      uint32_t relayDelaySS[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; //32
                                
 //*********************************************************************************************************************   

    //hello from AgIO
    uint8_t helloFromMachine[] = { 128, 129, 123, 123, 2, 1, 1, 71 };

    const uint8_t LOOP_TIME = 200; //5hz
    uint32_t lastTime = LOOP_TIME;
    uint32_t currentTime = LOOP_TIME;
    uint32_t fifthTime = 0;
    uint16_t count = 0;

    //Comm checks
    uint8_t watchdogTimer = 20; //make sure we are talking to AOG
    uint8_t serialResetTimer = 0; //if serial buffer is getting full, empty it

    bool isRaise = false, isLower = false;

    //Communication with AgOpenGPS
    int16_t temp, EEread = 0;

    //Parsing PGN
    bool isPGNFound = false, isHeaderFound = false;
    uint8_t pgn = 0, dataLength = 0, idx = 0;
    int16_t tempHeader = 0;

    //settings pgn
    uint8_t PGN_237[] = { 0x80,0x81, 0x7f, 237, 8, 1, 2, 3, 4, 0,0,0,0, 0xCC };
    int8_t PGN_237_Size = sizeof(PGN_237) - 1;

    //The variables used for storage
    uint8_t relayHi = 0, relayLo = 0, tramline = 0, uTurn = 0, hydLift = 0, geoStop = 0;
    
//**********
    uint8_t SCtoEight = 0, SCtoSixteen = 0, SCtoTwentyfour = 0,  SCtoThirtytwo = 0, SCtoFourty = 0, SCtoFourtyeight = 0, SCtoFiftysix = 0, SCtoSixtyfour = 0;
//***********

    float gpsSpeed;
    uint8_t raiseTimer = 0, lowerTimer = 0, lastTrigger = 0;
 

    void setup()
    {

        //set the baud rate
        //Serial.begin(38400);  //Commented out to allow 15 and 16th section to be used with UDP
        //while (!Serial) { ; } // wait for serial port to connect. Needed for native USB, but disable for 16 sections over UDP.

        EEPROM.get(0, EEread);              // read identifier

        if (EEread != EEP_Ident)   // check on first start and write EEPROM
        {
            EEPROM.put(0, EEP_Ident);
            EEPROM.put(6, aogConfig);
            EEPROM.put(20, pin);
            EEPROM.put(50, networkAddress);
        }
        else
        {
            EEPROM.get(6, aogConfig);
            EEPROM.get(20, pin);
            EEPROM.get(50, networkAddress);
        }

        if (ether.begin(sizeof Ethernet::buffer, mymac, CS_Pin) == 0)
            Serial.println(F("Failed to access Ethernet controller"));

        //grab the ip from EEPROM
        myip[0] = networkAddress.ipOne;
        myip[1] = networkAddress.ipTwo;
        myip[2] = networkAddress.ipThree;

        gwip[0] = networkAddress.ipOne;
        gwip[1] = networkAddress.ipTwo;
        gwip[2] = networkAddress.ipThree;

        ipDestination[0] = networkAddress.ipOne;
        ipDestination[1] = networkAddress.ipTwo;
        ipDestination[2] = networkAddress.ipThree;

        //set up connection
        ether.staticSetup(myip, gwip, myDNS, mask);
        ether.printIp("_IP_: ", ether.myip);
        ether.printIp("GWay: ", ether.gwip);
        ether.printIp("AgIO: ", ipDestination);

        //register to port 8888
        ether.udpServerListenOnPort(&udpSteerRecv, 8888);
        
        //Ethernet Shield It's wired:
        //RST - RST
        //GND - GND
        //VCC - 3.3V
        //CS - D10 Cant use the following pins
        //SI - D11
        //SO - D12
        //SCK - D13
        //AO7 no DO
        
        //Networked Board Layout
        pinMode(14, OUTPUT); //section 1 A0
        
        pinMode(2, OUTPUT); //section 2 D2
        pinMode(3, OUTPUT); //section 3
        pinMode(4, OUTPUT); //section 4
        pinMode(5, OUTPUT); //section 5
        pinMode(6, OUTPUT); //section 6
        pinMode(7, OUTPUT); //section 7
        pinMode(8, OUTPUT); //section 8 
        pinMode(9, OUTPUT); //section 9 D9
        
        pinMode(15, OUTPUT); //section 10 A1
        pinMode(16, OUTPUT); //section 11
        pinMode(17, OUTPUT); //section 12
        pinMode(18, OUTPUT); //section 13
        pinMode(19, OUTPUT); //section 14 A5

        pinMode(1, OUTPUT); //section 15 TX, comment out if ttl needed
        pinMode(0, OUTPUT); //section 16 RX, comment out if ttl needed

        //16 sections possible using nano with ethernet shield, but only with no debug ttl on rx tx.

        Serial.println("Setup complete, waiting for AgOpenGPS");

    }

    void loop()
    {

        //section relays put here to test realtime --tbr
        //SetRelays();
        
        //Loop triggers every 200 msec and sends back gyro heading, and roll, steer angle etc

        currentTime = millis();

        if (currentTime - lastTime >= LOOP_TIME)
        {
            lastTime = currentTime;

            //If connection lost to AgOpenGPS, the watchdog will count up 
            if (watchdogTimer++ > 250) watchdogTimer = 20;

            //clean out serial buffer to prevent buffer overflow
            if (serialResetTimer++ > 20)
            {
                while (Serial.available() > 0) Serial.read();
                serialResetTimer = 0;
            }

            if (watchdogTimer > 20)
            {
                if (aogConfig.isRelayActiveHigh) {
                    relayLo = 255;
                    relayHi = 255;
                }
                else {
                    relayLo = 0;
                    relayHi = 0;
                }
            }

            //hydraulic lift

            if (hydLift != lastTrigger && (hydLift == 1 || hydLift == 2))
            {
                lastTrigger = hydLift;
                lowerTimer = 0;
                raiseTimer = 0;

                //200 msec per frame so 5 per second
                switch (hydLift)
                {
                    //lower
                case 1:
                    lowerTimer = aogConfig.lowerTime * 5;
                    break;

                    //raise
                case 2:
                    raiseTimer = aogConfig.raiseTime * 5;
                    break;
                }
            }

            //countdown if not zero, make sure up only
            if (raiseTimer)
            {
                raiseTimer--;
                lowerTimer = 0;
            }
            if (lowerTimer) lowerTimer--;

            //if anything wrong, shut off hydraulics, reset last
            if ((hydLift != 1 && hydLift != 2) || watchdogTimer > 10) //|| gpsSpeed < 2)
            {
                lowerTimer = 0;
                raiseTimer = 0;
                lastTrigger = 0;
            }

            if (aogConfig.isRelayActiveHigh)
            {
                isLower = isRaise = false;
                if (lowerTimer) isLower = true;
                if (raiseTimer) isRaise = true;
            }
            else
            {
                isLower = isRaise = true;
                if (lowerTimer) isLower = false;
                if (raiseTimer) isRaise = false;
            }

            //section relays
            SetRelays();

            //checksum
            int16_t CK_A = 0;
            for (uint8_t i = 2; i < PGN_237_Size; i++)
            {
                CK_A = (CK_A + PGN_237[i]);
            }
            PGN_237[PGN_237_Size] = CK_A;

            //off to AOG
            ether.sendUdp(PGN_237, sizeof(PGN_237), portMy, ipDestination, portDestination);

        } //end of timed loop

        delay(1);

        //this must be called for ethercard functions to work. Calls udpSteerRecv() defined way below.
        ether.packetLoop(ether.packetReceive());
    }

  //callback when received packets
    void udpSteerRecv(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, uint8_t* udpData, uint16_t len)
    {
        /* IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);
        Serial.print("dPort:");  Serial.print(dest_port);
        Serial.print("  sPort: ");  Serial.print(src_port);
        Serial.print("  sIP: ");  ether.printIp(src_ip);  Serial.println("  end");

        //for (int16_t i = 0; i < len; i++) {
        //Serial.print(udpData[i],HEX); Serial.print("\t"); } Serial.println(len);
        */

        if (udpData[0] == 0x80 && udpData[1] == 0x81 && udpData[2] == 0x7F) //Data
        {

            if (udpData[3] == 239)  //machine data
            {
                uTurn = udpData[5];
                gpsSpeed = (float)udpData[6];//actual speed times 4, single uint8_t

                hydLift = udpData[7];
                tramline = udpData[8];  //bit 0 is right bit 1 is left

                relayLo = udpData[11];          // read relay control from AgOpenGPS
                relayHi = udpData[12];

                if (aogConfig.isRelayActiveHigh)
                {
                    tramline = 255 - tramline;
                    relayLo = 255 - relayLo;
                    relayHi = 255 - relayHi;
                }

                //Bit 13 CRC

                //reset watchdog
                watchdogTimer = 0;
            }
//************************************************************************************************************************
 // Symmetric Sections PGN Parse

            if (udpData[3] == 229)  //machine data
            {
                SCtoEight = udpData[5];
                SCtoSixteen = udpData[6];
                SCtoTwentyfour = udpData[7];
                SCtoThirtytwo = udpData[8];  
                SCtoFourty = udpData[9];
                SCtoFourtyeight = udpData[10];          
                SCtoFiftysix = udpData[11];
                SCtoSixtyfour = udpData[12];
                //reset watchdog
                watchdogTimer = 0;
            }
         
//***************************************************************************************************************************************
            else if (udpData[3] == 200) // Hello from AgIO
            {
                if (udpData[7] == 1)
                {
                    relayLo -= 255;
                    relayHi -= 255;
                    watchdogTimer = 0;
                }

                helloFromMachine[5] = relayLo;
                helloFromMachine[6] = relayHi;

                ether.sendUdp(helloFromMachine, sizeof(helloFromMachine), portMy, ipDestination, portDestination);
            }


            else if (udpData[3] == 238)
            {
                aogConfig.raiseTime = udpData[5];
                aogConfig.lowerTime = udpData[6];
                aogConfig.enableToolLift = udpData[7];

                //set1 
                uint8_t sett = udpData[8];  //setting0     
                if (bitRead(sett, 0)) aogConfig.isRelayActiveHigh = 1; else aogConfig.isRelayActiveHigh = 0;

                aogConfig.user1 = udpData[9];
                aogConfig.user2 = udpData[10];
                aogConfig.user3 = udpData[11];
                aogConfig.user4 = udpData[12];

                //crc

                //save in EEPROM and restart
                EEPROM.put(6, aogConfig);
                //resetFunc();
            }

            else if (udpData[3] == 201)
            {
                //make really sure this is the subnet pgn
                if (udpData[4] == 5 && udpData[5] == 201 && udpData[6] == 201)
                {
                    networkAddress.ipOne = udpData[7];
                    networkAddress.ipTwo = udpData[8];
                    networkAddress.ipThree = udpData[9];

                    //save in EEPROM and restart
                    EEPROM.put(50, networkAddress);
                    resetFunc();
                }
            }

            //whoami
            else if (udpData[3] == 202)
            {
                //make really sure this is the subnet pgn
                if (udpData[4] == 3 && udpData[5] == 202 && udpData[6] == 202)
                {
                    //hello from AgIO
                    uint8_t scanReply[] = { 128, 129, 123, 203, 4, 
                        networkAddress.ipOne, networkAddress.ipTwo, networkAddress.ipThree, 123, 23   };

                    //checksum
                    int16_t CK_A = 0;
                    for (uint8_t i = 2; i < sizeof(scanReply) - 1; i++)
                    {
                        CK_A = (CK_A + scanReply[i]);
                    }
                    scanReply[sizeof(scanReply)] = CK_A;

                    static uint8_t ipDest[] = { 255,255,255,255 };
                    uint16_t portDest = 9999; //AOG port that listens

                    //off to AOG
                    ether.sendUdp(scanReply, sizeof(scanReply), portMy, ipDest, portDest);
                }
            }

            else if (udpData[3] == 236) //EC Relay Pin Settings 
            {
                for (uint8_t i = 0; i < 24; i++)
                {
                    pin[i] = udpData[i + 5];
                }

                //save in EEPROM and restart
                EEPROM.put(20, pin);
            }
        }
    }

    void SetRelays(void)
    {
        //pin, rate, duration  130 pp meter, 3.6 kmh = 1 m/sec or gpsSpeed * 130/3.6 or gpsSpeed * 36.1111
        //gpsSpeed is 10x actual speed so 3.61111
        gpsSpeed *= 3.61111;
        //tone(13, gpsSpeed);

        if( BoardRow == 0){
        //Load the current pgn relay state - Sections
        for (uint8_t i = 0; i < 8; i++)
        {
            relayState[i] = bitRead(relayLo, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayState[i + 8] = bitRead(relayHi, i);
        }

      }
 //********************************************************************************************************************************
 //Load the current pgn relay state - Symmetric Sections up to 64

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS[i] = bitRead(SCtoEight, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS[i+8] = bitRead(SCtoSixteen, i);
        }
 
        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS[i+16] = bitRead(SCtoTwentyfour, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS[i+24] = bitRead(SCtoThirtytwo, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS2[i] = bitRead(SCtoFourty, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS2[i+8] = bitRead(SCtoFourtyeight, i);
        }

        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS2[i+16] = bitRead(SCtoFiftysix, i);
        }
        
        for (uint8_t i = 0; i < 8; i++)
        {
            relayStateSS2[i+24] = bitRead(SCtoSixtyfour, i);
        }

        
 
 
 //********************************************************************************************************************************

        // Hydraulics
        relayState[16] = isLower;
        relayState[17] = isRaise;

        //Tram
        relayState[18] = bitRead(tramline, 0); //right
        relayState[19] = bitRead(tramline, 1); //left

        //GeoStop
        relayState[20] = (geoStop == 0) ? 0 : 1;


        
//********************************************************************************************************************************
        if(BoardRow == 0){
        //First Classic Networked Board
        if (pin[0]) digitalWrite(14, relayState[pin[0] - 1]); //if aog section 1, activate nano pin 14, A0
        
        if (pin[1]) digitalWrite(2, relayState[pin[1] - 1]); //if aog section 2, activate nano pin 2, D2
        if (pin[2]) digitalWrite(3, relayState[pin[2] - 1]); //if aog section 3, activate nano pin 3, D3
        if (pin[3]) digitalWrite(4, relayState[pin[3] - 1]); //if aog section 4, activate nano pin 4, D4
        if (pin[4]) digitalWrite(5, relayState[pin[4] - 1]); //if aog section 5, activate nano pin 5, D5
        if (pin[5]) digitalWrite(6, relayState[pin[5] - 1]); //if aog section 6, activate nano pin 6, D6
        if (pin[6]) digitalWrite(7, relayState[pin[6] - 1]); //if aog section 7, activate nano pin 7, D7
        if (pin[7]) digitalWrite(8, relayState[pin[7] - 1]); //if aog section 8, activate nano pin 8, D8
        if (pin[8]) digitalWrite(9, relayState[pin[8] - 1]); //if aog section 9, activate nano pin 9, D9

        if (pin[9]) digitalWrite(15, relayState[pin[9] - 1]); //if aog section 10, activate nano pin 15, A1
        if (pin[10]) digitalWrite(16, relayState[pin[10] - 1]); //if aog section 11, activate nano pin 16, A2
        if (pin[11]) digitalWrite(17, relayState[pin[11] - 1]); //if aog section 12, activate nano pin 17, A3
        if (pin[12]) digitalWrite(18, relayState[pin[12] - 1]); //if aog section 13, activate nano pin 18, A4
        if (pin[13]) digitalWrite(19, relayState[pin[13] - 1]); //if aog section 14, activate nano pin 19, A5
        
        if (pin[14]) digitalWrite(1, relayState[pin[14] - 1]); //if aog section 15, activate nano pin 1, TX If serial.begin disabled
        if (pin[15]) digitalWrite(0, relayState[pin[15] - 1]); //if aog section 16, activate nano pin 0, RX If serial.begin disabled

        }


       //Pass section data without change if in first column
       if (BoardColumn == 1) {

        for (uint32_t i = 0; i < 32; i++)
          {
            relaytriggerSS [i] = relayStateSS [i];
            relaytriggerSS2 [i] = relayStateSS2 [i];
          }
        }
        
        
        
        if (BoardColumn == 2){
 
             

      
          }


        
        if(BoardRow == 1){                  
        //First Symmetrical Section 16ch Networked Board Sections 1 to 16
        digitalWrite(14, relaytriggerSS[0]); //if aog section 1, activate nano pin 14, A0
        
        digitalWrite(2, relaytriggerSS[1]); //if aog section 2, activate nano pin 2, D2
        digitalWrite(3, relaytriggerSS[2]); //if aog section 3, activate nano pin 3, D3
        digitalWrite(4, relaytriggerSS[3]); //if aog section 4, activate nano pin 4, D4
        digitalWrite(5, relaytriggerSS[4]); //if aog section 5, activate nano pin 5, D5
        digitalWrite(6, relaytriggerSS[5]); //if aog section 6, activate nano pin 6, D6
        digitalWrite(7, relaytriggerSS[6]); //if aog section 7, activate nano pin 7, D7
        digitalWrite(8, relaytriggerSS[7]); //if aog section 8, activate nano pin 8, D8
        digitalWrite(9, relaytriggerSS[8]); //if aog section 9, activate nano pin 9, D9

        digitalWrite(15, relaytriggerSS[9]); //if aog section 10, activate nano pin 15, A1
        digitalWrite(16, relaytriggerSS[10]); //if aog section 11, activate nano pin 16, A2
        digitalWrite(17, relaytriggerSS[11]); //if aog section 12, activate nano pin 17, A3
        digitalWrite(18, relaytriggerSS[12]); //if aog section 13, activate nano pin 18, A4
        digitalWrite(19, relaytriggerSS[13]); //if aog section 14, activate nano pin 19, A5
        
        digitalWrite(1, relaytriggerSS[14]); //if aog section 15, activate nano pin 1, TX If serial.begin disabled
        digitalWrite(0, relaytriggerSS[15]); //if aog section 16, activate nano pin 0, RX If serial.begin disabled

        }

        if(BoardRow == 2){
        //Second Symmetrical Section 16ch Networked Board Sections 17 to 32
        digitalWrite(14, relayStateSS[16]); //if aog section 17, activate nano pin 14, A0
        
        digitalWrite(2, relayStateSS[17]); //if aog section 18, activate nano pin 2, D2
        digitalWrite(3, relayStateSS[18]); //if aog section 19, activate nano pin 3, D3
        digitalWrite(4, relayStateSS[19]); //if aog section 20, activate nano pin 4, D4
        digitalWrite(5, relayStateSS[20]); //if aog section 21, activate nano pin 5, D5
        digitalWrite(6, relayStateSS[21]); //if aog section 22, activate nano pin 6, D6
        digitalWrite(7, relayStateSS[22]); //if aog section 23, activate nano pin 7, D7
        digitalWrite(8, relayStateSS[23]); //if aog section 24, activate nano pin 8, D8
        digitalWrite(9, relayStateSS[24]); //if aog section 25, activate nano pin 9, D9

        digitalWrite(15, relayStateSS[25]); //if aog section 26, activate nano pin 15, A1
        digitalWrite(16, relayStateSS[26]); //if aog section 27, activate nano pin 16, A2
        digitalWrite(17, relayStateSS[27]); //if aog section 28, activate nano pin 17, A3
        digitalWrite(18, relayStateSS[28]); //if aog section 29, activate nano pin 18, A4
        digitalWrite(19, relayStateSS[29]); //if aog section 30, activate nano pin 19, A5
        
        digitalWrite(1, relayStateSS[30]); //if aog section 31, activate nano pin 1, TX If serial.begin disabled
        digitalWrite(0, relayStateSS[31]); //if aog section 32, activate nano pin 0, RX If serial.begin disabled

        }

        if(BoardRow == 3){
        //Third Symmetrical Section 16ch Networked Board
        digitalWrite(14, relayStateSS2[0]); //if aog section 33, activate nano pin 14, A0
        
        digitalWrite(2, relayStateSS2[1]); //if aog section 34, activate nano pin 2, D2
        digitalWrite(3, relayStateSS2[2]); //if aog section 35, activate nano pin 3, D3
        digitalWrite(4, relayStateSS2[3]); //if aog section 36, activate nano pin 4, D4
        digitalWrite(5, relayStateSS2[4]); //if aog section 37, activate nano pin 5, D5
        digitalWrite(6, relayStateSS2[5]); //if aog section 38, activate nano pin 6, D6
        digitalWrite(7, relayStateSS2[6]); //if aog section 39, activate nano pin 7, D7
        digitalWrite(8, relayStateSS2[7]); //if aog section 40, activate nano pin 8, D8
        digitalWrite(9, relayStateSS2[8]); //if aog section 41, activate nano pin 9, D9

        digitalWrite(15, relayStateSS2[9]); //if aog section 42, activate nano pin 15, A1
        digitalWrite(16, relayStateSS2[10]); //if aog section 43, activate nano pin 16, A2
        digitalWrite(17, relayStateSS2[11]); //if aog section 44, activate nano pin 17, A3
        digitalWrite(18, relayStateSS2[12]); //if aog section 45, activate nano pin 18, A4
        digitalWrite(19, relayStateSS2[13]); //if aog section 46, activate nano pin 19, A5
        
        digitalWrite(1, relayStateSS2[14]); //if aog section 47, activate nano pin 1, TX If serial.begin disabled
        digitalWrite(0, relayStateSS2[15]); //if aog section 48, activate nano pin 0, RX If serial.begin disabled

        }

        if(BoardRow == 4){
        //Fourth Symmetrical Section 16ch Networked Board
        digitalWrite(14, relayStateSS2[16]); //if aog section 49, activate nano pin 14, A0
        
        digitalWrite(2, relayStateSS2[17]); //if aog section 50, activate nano pin 2, D2
        digitalWrite(3, relayStateSS2[18]); //if aog section 51, activate nano pin 3, D3
        digitalWrite(4, relayStateSS2[19]); //if aog section 52, activate nano pin 4, D4
        digitalWrite(5, relayStateSS2[20]); //if aog section 53, activate nano pin 5, D5
        digitalWrite(6, relayStateSS2[21]); //if aog section 54, activate nano pin 6, D6
        digitalWrite(7, relayStateSS2[22]); //if aog section 55, activate nano pin 7, D7
        digitalWrite(8, relayStateSS2[23]); //if aog section 56, activate nano pin 8, D8
        digitalWrite(9, relayStateSS2[24]); //if aog section 57, activate nano pin 9, D9

        digitalWrite(15, relayStateSS2[25]); //if aog section 58, activate nano pin 15, A1
        digitalWrite(16, relayStateSS2[26]); //if aog section 59, activate nano pin 16, A2
        digitalWrite(17, relayStateSS2[27]); //if aog section 60, activate nano pin 17, A3
        digitalWrite(18, relayStateSS2[28]); //if aog section 61, activate nano pin 18, A4
        digitalWrite(19, relayStateSS2[29]); //if aog section 62, activate nano pin 19, A5
        
        digitalWrite(1, relayStateSS2[30]); //if aog section 63, activate nano pin 1, TX If serial.begin disabled
        digitalWrite(0, relayStateSS2[31]); //if aog section 64, activate nano pin 0, RX If serial.begin disabled

        }
    }
//********************************************************************************************************************************        
        
    


  //Example funciotns currently not used
  //Timed second column trigger function
 
  //bool TimerOnOff (bool trigger){
 // bool result;

 // result = trigger;
 // return result;

 // }
  

  //Distance second column trigger function
 
  //bool DistanceOnOff (bool trigger){
  //bool result;

  //result = trigger;
  //return result;
  
  //}
