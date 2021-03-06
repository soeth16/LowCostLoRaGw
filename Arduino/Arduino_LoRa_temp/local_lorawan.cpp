/*
 *  simple lorawan lib to encode and decode lorawan packet
 *  
 *  Copyright (C) 2016-2020 Congduc Pham, University of Pau, France
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/
 
#include "local_lorawan.h"
#include "band_config.h"
#include "SX1272.h"

// we wrapped Serial.println to support the Arduino Zero or M0
#if defined __SAMD21G18A__ && not defined ARDUINO_SAMD_FEATHER_M0
#define PRINTLN                   SerialUSB.println("")              
#define PRINT_CSTSTR(fmt,param)   SerialUSB.print(F(param))
#define PRINT_STR(fmt,param)      SerialUSB.print(param)
#define PRINT_VALUE(fmt,param)    SerialUSB.print(param)
#define PRINT_HEX(fmt,param)      SerialUSB.print(param,HEX)
#define FLUSHOUTPUT               SerialUSB.flush();
#else
#define PRINTLN                   Serial.println("")
#define PRINT_CSTSTR(fmt,param)   Serial.print(F(param))
#define PRINT_STR(fmt,param)      Serial.print(param)
#define PRINT_VALUE(fmt,param)    Serial.print(param)
#define PRINT_HEX(fmt,param)      Serial.print(param,HEX)
#define FLUSHOUTPUT               Serial.flush();
#endif

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE SPREADING FACTOR - ONLY FOR LORAWAN MODE
uint8_t lorawanSF=12;
//////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//For TTN, use specify ABP mode on the TTN device setting

///////////////////////////////////////////////////////////////////
//ENTER HERE your App Session Key from the TTN device info (same order, i.e. msb)
//unsigned char AppSkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
///////////////////////////////////////////////////////////////////

//this is the default as LoRaWAN example
unsigned char AppSkey[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

///////////////////////////////////////////////////////////////////
//ENTER HERE your Network Session Key from the TTN device info (same order, i.e. msb)
//unsigned char NwkSkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
///////////////////////////////////////////////////////////////////

//this is the default as LoRaWAN example
unsigned char NwkSkey[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

///////////////////////////////////////////////////////////////////
// DO NOT CHANGE HERE
uint16_t Frame_Counter_Up = 0x0000;
// 0 for uplink, 1 for downlink - See LoRaWAN specifications
unsigned char Direction = 0x00;
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//set to the default LoRa datarate SF12BW125
//set to first frequency of LoRaWAN: 868.1 for EU868, 923.2 for EU900 and 433.175 for EU433
//set sync word to LoRaWAN networks, i.e. 0x34

void local_lorawan_init() {

  int e;

  e = sx1272.setBW(BW_125);
  PRINT_CSTSTR("%s","Set BW to 125kHz: state "); 
  PRINT_VALUE("%d", e);
  PRINTLN; 
  
  //we can also change the SF value for LoRaWAN
  e = sx1272.setSF(lorawanSF);
  PRINT_CSTSTR("%s","Set SF to ");
  PRINT_VALUE("%d", lorawanSF);    
  PRINT_CSTSTR("%s",": state ");
  PRINT_VALUE("%d", e);
  PRINTLN;  
  
  // Select frequency channel
#ifdef BAND868
  //868.1MHz
  e = sx1272.setChannel(CH_18_868);
  PRINT_CSTSTR("%s","Set frequency to 868.1MHz: state ");
#elif defined BAND900
  //hardcoded with the first LoRaWAN frequency
  uint32_t loraChannel=(double)923.2*1000000.0*RH_LORA_FCONVERT;
  e = sx1272.setChannel(loraChannel);
  PRINT_CSTSTR("%s","Set frequency to 923.2MHz: state ");
#elif defined BAND433
  //hardcoded with the first LoRaWAN frequency
  uint32_t loraChannel=(double)433.175*1000000.0*RH_LORA_FCONVERT;
  e = sx1272.setChannel(loraChannel);
  PRINT_CSTSTR("%s","Set frequency to 433.175MHz: state ");
#endif 

  PRINT_VALUE("%d", e);
  PRINTLN;
  
  e = sx1272.setSyncWord(0x34);
  PRINT_CSTSTR("%s","Set sync word to 0x34: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  PRINT_CSTSTR("%s","Using raw mode for native LoRaWAN ");
  // set raw format, both in transmission and reception to true for LoRaWAN mode
  sx1272._rawFormat=true;
  sx1272._rawFormat_send=true;
}

///////////////////////////////////////////////////////////////////
//create a LoRaWAN packet meaning that the payload is encrypted

uint8_t local_aes_lorawan_create_pkt(uint8_t* message, uint8_t pl, uint8_t app_key_offset) {

      PRINT_STR("%s",(char*)(message+app_key_offset));
      PRINTLN;
      PRINT_CSTSTR("%s","plain payload hex\n");
      for (int i=0; i<pl;i++) {
        if (message[i]<16)
          PRINT_CSTSTR("%s","0");
        PRINT_HEX("%X", message[i]);
        PRINT_CSTSTR("%s"," ");
      }
      PRINTLN; 

      PRINT_CSTSTR("%s","Encrypting\n");     
      Encrypt_Payload((unsigned char*)message, pl, Frame_Counter_Up, Direction);

      PRINT_CSTSTR("%s","encrypted payload\n");
      //Print encrypted message
      for (int i = 0; i < pl; i++)      
      {
        if (message[i]<16)
          PRINT_CSTSTR("%s","0");
        PRINT_HEX("%X", message[i]);         
        PRINT_CSTSTR("%s"," ");
      }
      PRINTLN;   

      // with only AES encryption, we still use a LoRaWAN packet format to reuse available LoRaWAN encryption libraries
      //
      unsigned char LORAWAN_Data[80];
      unsigned char LORAWAN_Package_Length;
      unsigned char MIC[4];
      //Unconfirmed data up
      unsigned char Mac_Header = 0x40;
      // no ADR, not an ACK and no options
      unsigned char Frame_Control = 0x00;
      // with application data so Frame_Port = 1..223
      unsigned char Frame_Port = 0x01; 

      //Build the Radio Package, LoRaWAN format
      //See LoRaWAN specification
      LORAWAN_Data[OFF_DAT_HDR] = Mac_Header;
    
      LORAWAN_Data[OFF_DAT_ADDR] = DevAddr[3];
      LORAWAN_Data[OFF_DAT_ADDR+1] = DevAddr[2];
      LORAWAN_Data[OFF_DAT_ADDR+2] = DevAddr[1];
      LORAWAN_Data[OFF_DAT_ADDR+3] = DevAddr[0];
    
      LORAWAN_Data[OFF_DAT_FCT] = Frame_Control;
    
      LORAWAN_Data[OFF_DAT_SEQNO] = (Frame_Counter_Up & 0x00FF);
      LORAWAN_Data[OFF_DAT_SEQNO+1] = ((Frame_Counter_Up >> 8) & 0x00FF);
    
      LORAWAN_Data[OFF_DAT_OPTS] = Frame_Port;
    
      //Set Current package length
      LORAWAN_Package_Length = OFF_DAT_OPTS+1;
      
      //Load Data
      for(int i = 0; i < pl; i++)
      {
        // see that we don't take the appkey, just the encrypted data that starts at message[app_key_offset]
        LORAWAN_Data[LORAWAN_Package_Length + i] = message[i];
      }
    
      //Add data Lenth to package length
      LORAWAN_Package_Length = LORAWAN_Package_Length + pl;
    
      PRINT_CSTSTR("%s","calculate MIC with NwkSKey\n");
      //Calculate MIC
      Calculate_MIC(LORAWAN_Data, MIC, LORAWAN_Package_Length, Frame_Counter_Up, Direction);
    
      //Load MIC in package
      for (int i=0; i < 4; i++)
      {
        LORAWAN_Data[i + LORAWAN_Package_Length] = MIC[i];
      }
    
      //Add MIC length to package length
      LORAWAN_Package_Length = LORAWAN_Package_Length + 4;
    
      PRINT_CSTSTR("%s","transmitted LoRaWAN-like packet:\n");
      PRINT_CSTSTR("%s","MHDR[1] | DevAddr[4] | FCtrl[1] | FCnt[2] | FPort[1] | EncryptedPayload | MIC[4]\n");
      //Print transmitted data
      for (int i = 0; i < LORAWAN_Package_Length; i++)
      {
        if (LORAWAN_Data[i]<16)
          PRINT_CSTSTR("%s","0");
        PRINT_HEX("%X", LORAWAN_Data[i]);         
        PRINT_CSTSTR("%s"," ");
      }
      PRINTLN;      

      // copy back to message
      memcpy(message,LORAWAN_Data,LORAWAN_Package_Length);
      pl = LORAWAN_Package_Length;

#ifdef AES_SHOWB64
      //need to take into account the ending \0 for string in C
      int b64_encodedLen = base64_enc_len(pl)+1;
      char b64_encoded[b64_encodedLen];
               
      base64_encode(b64_encoded, (char*)message, pl);
      PRINT_CSTSTR("%s","[base64 LoRaWAN HEADER+CIPHER+MIC]:");
      PRINT_STR("%s", b64_encoded);
      PRINTLN;
#endif      

      // in any case, we increment Frame_Counter_Up
      // even if the transmission will not succeed
      Frame_Counter_Up++;

      return pl;
}

///////////////////////////////////////////////////////////////////
//decode a LoRaWAN downlink packet and check the MIC

int8_t local_aes_lorawan_decode_pkt(uint8_t* message, uint8_t pl) {

      unsigned char MIC[4];
      
      //get FCntDn from LoRaWAN downlink packet
      uint16_t seqno=(uint16_t)((uint16_t)message[OFF_DAT_SEQNO] | ((uint16_t)message[OFF_DAT_SEQNO+1]<<8));

      //end of payload, removing the 4-byte MIC
      uint8_t pend=pl-4;

      Calculate_MIC(message, MIC, pend, seqno, 1);

      if ((uint8_t)MIC[3]==message[pl-1] &&
          (uint8_t)MIC[2]==message[pl-2] &&
          (uint8_t)MIC[1]==message[pl-3] &&
          (uint8_t)MIC[0]==message[pl-4]) {

          PRINT_CSTSTR("%s","Valid MIC\n");  
           
          //start of payload
          uint8_t poff=OFF_DAT_OPTS+1+(uint8_t)(message[OFF_DAT_FCT] & FCT_OPTLEN);
          
          //length of payload
          int8_t ml=pend-poff;
    
          if (ml>0) {
            PRINT_CSTSTR("%s","Decrypting\n");          
            Encrypt_Payload((unsigned char*)(message+poff), ml, seqno, 1);
    
            PRINT_CSTSTR("%s","clear payload\n"); 
            //Print decrypted message
            for (int i = poff; i < poff+ml; i++)      
            {
              if (message[i]<16)
                PRINT_CSTSTR("%s","0");
              PRINT_HEX("%X", message[i]);         
              PRINT_CSTSTR("%s"," ");
            }
            PRINTLN;
    
            return (int8_t)poff;
          }
          else
            PRINT_CSTSTR("%s","No payload\n"); 
      }
      else 
        PRINT_CSTSTR("%s","Bad MIC\n");
        
      return -1;              
}
