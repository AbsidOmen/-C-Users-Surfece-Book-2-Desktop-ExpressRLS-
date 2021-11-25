#ifdef HAS_THERMAL_LM75A
#include "lm75a.h"
#include "logging.h"


int LM75A::init()
{
    uint8_t buffer[5];

    ReadAccRegister(LM75A_REG_TOS, buffer, 2);

    INFOLN("thermal lma75 read Tos = 0x%x", buffer[0]);

    if(buffer[0] == 0)
    {
        ERRLN("thermal lma75 init failed!");
        return -1;
    }

    return 0;
}   

void LM75A::read_lm75a(float *temp)
{
     uint8_t buffer[5];
     float temp_data = 0;

    ReadAccRegister(LM75A_REG_TEMP, buffer, 2);

    temp_data = buffer[0];
    if((buffer[1] & 0x80) == 0x80)
    {
        temp_data += 0.5;
    }

    *temp = temp_data;

}

void LM75A::update_lm75a_threshold(uint8_t tos, uint8_t thyst)
{
    uint8_t buffer[5];
    buffer[0] = thyst;
    buffer[1] = 0;
    WriteAccRegister(LM75A_REG_THYST, buffer, 2);

    buffer[0] = tos;
    buffer[1] = 0;
    WriteAccRegister(LM75A_REG_TOS, buffer, 2);
}


void LM75A::ReadAccRegister(uint8_t reg, uint8_t *data, int size)
{
    Wire.beginTransmission(LM75A_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(LM75A_I2C_ADDRESS, size);    // request 1 bytes from slave device
    Wire.readBytes(data, size);
}

void LM75A::WriteAccRegister(uint8_t reg, uint8_t *data, int size)
{
    Wire.beginTransmission(LM75A_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(data, size);
    Wire.endTransmission();
}


#endif