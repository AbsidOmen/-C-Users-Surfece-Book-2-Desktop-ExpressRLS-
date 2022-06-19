#pragma once

#include "targets.h"
#include "SX1280_Regs.h"
#include "SX1280_hal.h"
#include "SX12xxDriverCommon.h"

#define RADIO_SNR_SCALE 4 // Units for LastPacketSNRRaw

class SX1280Driver: public SX12xxDriverCommon
{
public:
    static SX1280Driver *instance;


    ///////////Radio Variables////////
    uint16_t timeout = 0xFFFF;

    ///////////////////////////////////

    ////////////////Configuration Functions/////////////
    SX1280Driver();
    bool Begin();
    void End();
    void SetTxIdleMode() { SetMode(SX1280_MODE_FS, SX1280_Radio_All); }; // set Idle mode used when switching from RX to TX
    void Config(uint8_t bw, uint8_t sf, uint8_t cr, uint32_t freq,
                uint8_t PreambleLength, bool InvertIQ, uint8_t PayloadLength, uint32_t interval,
                uint32_t flrcSyncWord=0, uint16_t flrcCrcSeed=0, uint8_t flrc=0);
    void SetFrequencyHz(uint32_t freq);
    void SetFrequencyReg(uint32_t freq);
    void SetRxTimeoutUs(uint32_t interval);
    void SetOutputPower(int8_t power);


    bool GetFrequencyErrorbool();
    bool FrequencyErrorAvailable() const { return modeSupportsFei && (LastPacketSNRRaw > 0); }

    void TXnb(uint8_t * data, uint8_t size);
    void RXnb();

    uint16_t GetIrqStatus(SX1280_Radio_Number_t radioNumber);
    void ClearIrqStatus(uint16_t irqMask, SX1280_Radio_Number_t radioNumber);

    void GetStatus(SX1280_Radio_Number_t radioNumber);

    uint8_t GetRxBufferAddr(SX1280_Radio_Number_t radioNumber);
    int8_t GetRssiInst();
    void GetLastPacketStats();

private:
    SX1280_RadioOperatingModes_t currOpmode = SX1280_MODE_SLEEP;
    uint8_t packet_mode;
    bool modeSupportsFei;
    SX1280_Radio_Number_t processingPacketRadio;
    SX1280_Radio_Number_t lastSuccessfulPacketRadio = SX1280_Radio_1;

    void SetMode(SX1280_RadioOperatingModes_t OPmode, SX1280_Radio_Number_t radioNumber);
    void SetFIFOaddr(uint8_t txBaseAddr, uint8_t rxBaseAddr);

    // LoRa functions
    void ConfigModParamsLoRa(uint8_t bw, uint8_t sf, uint8_t cr);
    void SetPacketParamsLoRa(uint8_t PreambleLength, SX1280_RadioLoRaPacketLengthsModes_t HeaderType,
                             uint8_t PayloadLength, uint8_t InvertIQ);
    // FLRC functions
    void ConfigModParamsFLRC(uint8_t bw, uint8_t cr, uint8_t bt=SX1280_FLRC_BT_0_5);
    void SetPacketParamsFLRC(uint8_t HeaderType,
                             uint8_t PreambleLength,
                             uint8_t PayloadLength,
                             uint32_t syncWord,
                             uint16_t crcSeed,
                             uint8_t cr);

    void SetDioIrqParams(uint16_t irqMask,
                         uint16_t dio1Mask=SX1280_IRQ_RADIO_NONE,
                         uint16_t dio2Mask=SX1280_IRQ_RADIO_NONE,
                         uint16_t dio3Mask=SX1280_IRQ_RADIO_NONE);

    static void IsrCallback_1();
    static void IsrCallback_2();
    static void IsrCallback(SX1280_Radio_Number_t radioNumber);
    bool RXnbISR(uint16_t irqStatus, SX1280_Radio_Number_t radioNumber); // ISR for non-blocking RX routine
    void TXnbISR(); // ISR for non-blocking TX routine
};
