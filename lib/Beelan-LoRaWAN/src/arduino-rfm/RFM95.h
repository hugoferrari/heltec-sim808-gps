/******************************************************************************************
* Copyright 2017 Ideetron B.V.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************************/
/****************************************************************************************
* File:     RFM95.h
* Author:   Gerben den Hartog
* Compagny: Ideetron B.V.
* Website:  http://www.ideetron.nl/LoRa
* E-mail:   info@ideetron.nl
****************************************************************************************/
/****************************************************************************************
* Created on:         06-01-2017
* Supported Hardware: ID150119-02 Nexus board with RFM95
****************************************************************************************/

#ifndef RFM95_H
#define RFM95_H

/*
********************************************************************************************
* INCLUDES
********************************************************************************************
*/

#include "Struct.h"

/*
*****************************************************************************************
* TYPE DEFINITIONS
*****************************************************************************************
*/

typedef enum {NO_MESSAGE,NEW_MESSAGE,CRC_OK,MIC_OK,ADDRESS_OK,MESSAGE_DONE,TIMEOUT,WRONG_MESSAGE} message_t;

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

/*
*****************************************************************************************
* REGISTER DEFINITIONS
*****************************************************************************************
*/

typedef enum {
    RFM_REG_FIFO            = 0x00,                 //Entrada/salida de datos FIFO de banda base LoRa.
    RFM_REG_OP_MODE         = 0x01,                 //bit7: 1->LoRa Mode, bit6: 0-> Acceda a la página de registros LoRa 0x0D: 0x3F, bit3: 1-> Low Frequency Mode On, bit2-0: Device modes 
    RFM_REG_FR_MSB          = 0x06,                 //MSB of RF carrier frequency
    RFM_REG_FR_MID          = 0x07,                 //MSB of RF carrier frequency
    RFM_REG_FR_LSB          = 0x08,                 //LSB of RF carrier frequency
    RFM_REG_PA_CONFIG       = 0x09,
    RFM_REG_FIFO_RX_CU_ADDR = 0x10,                 //Start address (in data buffer) of last packet received
    RFM_REG_OCP             = 0x0b,
    RFM_REG_LNA             = 0x0C,
    RFM_REG_FIFO_ADDR_PTR   = 0x0D,
    RFM_REG_FIFO_TX_BA_ADDR = 0x0E,                  //write base address in FIFO data buffer for TX modulator
    RFM_REG_FIFO_RX_BA_ADDR = 0x0F,                  //read base address in FIFO data buffer for RX demodulator
    RFM_REG_IRQ_FLAGS       = 0x12,                  //Interrupt Register
    RFM_REG_RX_NB_BYTES     = 0x13,                  //Number of payload bytes of latest packet received
    RFM_REG_PKT_SNR_VALUE   = 0x19,                  //SNR del ultimo paquete, en pasos de 0.25 dB
    RFM_REG_LAST_RSSI       = 0x1A,
    RFM_REG_MODEM_CONFIG1   = 0x1D,
    RFM_REG_MODEM_CONFIG2   = 0x1E,
    RFM_REG_SYM_TIMEOUT_LSB = 0x1F,
    RFM_REG_PREAMBLE_MSB    = 0x20,
    RFM_REG_PREAMBLE_LSB    = 0x21,
    RFM_REG_PAYLOAD_LENGTH  = 0x22,
    RFM_REG_MODEM_CONFIG3   = 0x26, 
    RFM_REG_INVERT_IQ       = 0x33,
    RFM_REG_INVERT_IQ2      = 0x3b,
    RFM_REG_SYNC_WORD       = 0x39,
    RFM_REG_DIO_MAPPING1    = 0x40,
    RFM_REG_DIO_MAPPING2    = 0x41,
    RFM_REG_PA_DAC          = 0x4d
    
    } rfm_register_t;

//Device modes bits2-0 RFM_REG_OP_MODE + LoRa mode bit7
typedef enum {
    RFM_MODE_SLEEP      = 0b000,            //SLEEP
    RFM_MODE_STANDBY    = 0b001,            //STANDBY
    RFM_MODE_FSTX       = 0b010,            //Frequency synthesis TX (FSTX)
    RFM_MODE_TX         = 0b011,            //Transmit (TX)
    RFM_MODE_FSRX       = 0b100,            //Frequency synthesis RX (FSRX)
    RFM_MODE_RXCONT     = 0b101,            //Continuous receive (RXCONT)
    RFM_MODE_RXSINGLE   = 0b110,            //Single receive (RXSINGLE)
    RFM_MODE_CAD        = 0b111,            //Channel activity detection (CAD)
    RFM_MODE_LORA       = 0b10000000        //LoRa mode bit (bit7)
    } frm_mode_t;


/*
*****************************************************************************************
* FUNCTION PROTOTYPES
*****************************************************************************************
*/

bool RFM_Init();
void RFM_Send_Package(sBuffer *RFM_Tx_Package, sSettings *LoRa_Settings);
message_t RFM_Single_Receive(sSettings *LoRa_Settings);
void RFM_Continuous_Receive(sSettings *LoRa_Settings);
message_t RFM_Get_Package(sBuffer *RFM_Rx_Package);
void RFM_Write(unsigned char RFM_Address, unsigned char RFM_Data);
void RFM_Switch_Mode(unsigned char Mode);
void RFM_Set_Tx_Power(int level, int outputPin);
void RFM_Set_OCP(unsigned char mA);

unsigned char RFM_Get_Rssi();
signed char RFM_Get_PktSnrRaw();


#endif

