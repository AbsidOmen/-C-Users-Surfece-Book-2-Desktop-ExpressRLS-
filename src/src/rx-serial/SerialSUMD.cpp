#include "SerialSUMD.h"
#include "CRSF.h"
#include "device.h"

void SerialSUMD::setLinkQualityStats(uint16_t lq, uint16_t rssi)
{
    linkQuality = lq;
    rssiDBM = rssi;
}

uint32_t SerialSUMD::sendRCFrameToFC(bool frameAvailable, uint32_t *channelData)
{
    if (!frameAvailable) {
        return DURATION_IMMEDIATELY;
    }

	uint8_t outBuffer[SUMD_FRAME_16CH_LEN];

	outBuffer[0] = 0xA8;		//Graupner
	outBuffer[1] = 0x01;	    //SUMD
	outBuffer[2] = 0x10;		//16CH	

    uint16_t us = (CRSF_to_US(ChannelData[0]) << 3);
    outBuffer[3] = us >> 8;		
    outBuffer[4] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[1]) << 3);
    outBuffer[5] = us >> 8;		
    outBuffer[6] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[2]) << 3);
    outBuffer[7] = us >> 8;		
    outBuffer[8] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[3]) << 3);
    outBuffer[9] = us >> 8;		
    outBuffer[10] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[7]) << 3); //channel 8 mapped to 5 to move arm channel away from the aileron function
    outBuffer[11] = us >> 8;		
    outBuffer[12] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[5]) << 3);
    outBuffer[13] = us >> 8;		
    outBuffer[14] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[6]) << 3);
    outBuffer[15] = us >> 8;		
    outBuffer[16] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[4]) << 3); //channel 5 mapped to 8
    outBuffer[17] = us >> 8;		
    outBuffer[18] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[8]) << 3);
    outBuffer[19] = us >> 8;		
    outBuffer[20] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[9]) << 3);
    outBuffer[21] = us >> 8;		
    outBuffer[22] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[10]) << 3);
    outBuffer[23] = us >> 8;		
    outBuffer[24] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[11]) << 3);
    outBuffer[25] = us >> 8;		
    outBuffer[26] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[12]) << 3);
    outBuffer[27] = us >> 8;		
    outBuffer[28] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[13]) << 3);
    outBuffer[29] = us >> 8;		
    outBuffer[30] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[14]) << 3);
    outBuffer[31] = us >> 8;		
    outBuffer[32] = us & 0x00ff;
    us = (CRSF_to_US(ChannelData[15]) << 3);
    outBuffer[33] = us >> 8;		
    outBuffer[34] = us & 0x00ff;
	  
	uint16_t crc = crc16(outBuffer, (SUMD_HEADER_SIZE + SUMD_DATA_SIZE_16CH));
	outBuffer[35] = (uint8_t)(crc >> 8);
	outBuffer[36] = (uint8_t)(crc & 0x00ff);	  	
	
	_outputPort->write(outBuffer, sizeof(outBuffer));
	
    return DURATION_IMMEDIATELY;
}

uint16_t SerialSUMD::crc16(uint8_t *data, uint8_t len)
{		
	uint16_t crc = 0;

	for(uint8_t  i = 0; i < len; i++)
    {
        crc = crc ^ ((int16_t)data[i] << 8); 
    
        for(uint8_t i = 0; i < 8; i++) 
        {   
            if (crc & 0x8000) 
                crc = (crc << 1) ^ CRC_POLYNOME; 
            else 
                crc = (crc << 1); 
        }
	}
	
	return crc;
}

void SerialSUMD::sendLinkStatisticsToFC()
{
    // unsupported	
}

void SerialSUMD::sendMSPFrameToFC(uint8_t* data)
{
    // unsupported
}

void processByte(uint8_t byte) {
    // unsupported
}