#include "ranging.h"

/*!
 * \brief The buffer
 */
uint8_t RngBufferSize = RNG_BUFFER_SIZE;
uint8_t RngBuffer[RNG_BUFFER_SIZE];

/*!
 * \brief Mask of IRQs to listen to in rx mode
 */
uint16_t RngRxIrqMask = SX1280_IRQ_RX_DONE | SX1280_IRQ_RX_TX_TIMEOUT;

/*!
 * \brief Mask of IRQs to listen to in tx mode
 */
uint16_t RngTxIrqMask = SX1280_IRQ_TX_DONE | SX1280_IRQ_RX_TX_TIMEOUT;

/**!
 * \brief Mask of IRQs to listen to in ranging slave mode
 */
uint16_t RngSlaveIrqMask = SX1280_IRQ_RANGING_SLAVE_RESPONSE_DONE | SX1280_IRQ_RANGING_SLAVE_REQUEST_DISCARDED;

/**!
 * \brief Mask of IRQs to listen to in ranging master mode
 */
uint16_t RngMasterIrqMask = SX1280_IRQ_RANGING_MASTER_RESULT_VALID | SX1280_IRQ_RANGING_MASTER_RESULT_TIMEOUT;

/*!
 * \brief Ranging raw factors
 *                                  SF5     SF6     SF7     SF8     SF9     SF10
 */
const uint16_t RNG_CALIB_0400[] = { 10299,  10271,  10244,  10242,  10230,  10246  };
const uint16_t RNG_CALIB_0800[] = { 11486,  11474,  11453,  11426,  11417,  11401  };
const uint16_t RNG_CALIB_1600[] = { 13308,  13493,  13528,  13515,  13430,  13376  };
const double   RNG_FGRAD_0400[] = { -0.148, -0.214, -0.419, -0.853, -1.686, -3.423 };
const double   RNG_FGRAD_0800[] = { -0.041, -0.811, -0.218, -0.429, -0.853, -1.737 };
const double   RNG_FGRAD_1600[] = { 0.103,  -0.041, -0.101, -0.211, -0.424, -0.87  };

SX1280_PacketParams_t RngPacketParams;

SX1280_PacketStatus_t RngPacketStatus;

SX1280_ModulationParams_t RngModulationParams;

void RangingSetParams( void )
{
    RngModulationParams.PacketType = SX1280_PACKET_TYPE_LORA;
    RngModulationParams.Params.LoRa.SpreadingFactor = SX1280_LORA_SF10;
    RngModulationParams.Params.LoRa.Bandwidth = SX1280_LORA_BW_1600;
    RngModulationParams.Params.LoRa.CodingRate = SX1280_LORA_CR_LI_4_7;

    RngPacketParams.PacketType = SX1280_PACKET_TYPE_LORA;
    RngPacketParams.Params.LoRa.PreambleLength = 0x12;
    RngPacketParams.Params.LoRa.HeaderType = SX1280_LORA_PACKET_VARIABLE_LENGTH;
    RngPacketParams.Params.LoRa.PayloadLength = RngBufferSize;
    RngPacketParams.Params.LoRa.CrcMode = SX1280_LORA_CRC_ON;
    RngPacketParams.Params.LoRa.InvertIQ = SX1280_LORA_IQ_NORMAL;
}
void RangingInitRadio( void )
{
    SX1280HalReset();

    SX1280SetRegulatorMode(SX1280_USE_DCDC);
    HAL_Delay(500);

    SX1280SetStandby(SX1280_STDBY_RC);
    SX1280SetPacketType(RngModulationParams.PacketType);
    SX1280SetModulationParams(&RngModulationParams);
    SX1280SetPacketParams(&RngPacketParams);
    SX1280SetRfFrequency(RNG_RF_FREQUENCY);
    SX1280SetBufferBaseAddresses(0x00, 0x00);
}
void RangingInit(SX1280_RadioRangingRoles_t role, uint32_t address)
{
    uint16_t calibration;
    RngModulationParams.PacketType = SX1280_PACKET_TYPE_RANGING;
    RngPacketParams.PacketType = SX1280_PACKET_TYPE_RANGING;

    SX1280HalReset();
    SX1280SetRegulatorMode(SX1280_USE_DCDC);
    HAL_Delay(500);
    SX1280SetStandby(SX1280_STDBY_RC);
    SX1280SetPacketType(RngModulationParams.PacketType);
    SX1280SetModulationParams(&RngModulationParams);
    SX1280SetPacketParams(&RngPacketParams);
    SX1280SetRfFrequency(RNG_RF_FREQUENCY);
    SX1280SetBufferBaseAddresses(0x00, 0x00);
    SX1280SetTxParams(RNG_TX_OUTPUT_POWER, SX1280_RADIO_RAMP_02_US);

    if (role == SX1280_RADIO_RANGING_ROLE_MASTER)
    {
        SX1280SetRangingRequestAddress(address);
        SX1280SetDioIrqParams( RngMasterIrqMask, RngMasterIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    }
    else
    {
        SX1280SetDeviceRangingAddress(address);
        SX1280SetDioIrqParams( RngSlaveIrqMask, RngSlaveIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    }

    switch (RngModulationParams.Params.LoRa.Bandwidth)
    {
    case SX1280_LORA_BW_0400:
        calibration = RNG_CALIB_0400[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    case SX1280_LORA_BW_0800:
        calibration = RNG_CALIB_0800[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    case SX1280_LORA_BW_1600:
        calibration = RNG_CALIB_1600[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    default:
        calibration = 10000;
        break;
    }

    SX1280SetRangingCalibration(calibration);

    SX1280SetRangingRole(role);

    if (role == SX1280_RADIO_RANGING_ROLE_MASTER)
    {
        SX1280SetTx((SX1280_TickTime_t){RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_TX_TIMEOUT_VALUE});
    }
    else
    {
        SX1280SetRx((SX1280_TickTime_t) {RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_RX_TIMEOUT_VALUE});
    }
}

void RangingStart(SX1280_RadioRangingRoles_t role, uint32_t address)
{
    uint16_t calibration;
    RngModulationParams.PacketType = SX1280_PACKET_TYPE_RANGING;
    RngPacketParams.PacketType = SX1280_PACKET_TYPE_RANGING;

    SX1280SetStandby(SX1280_STDBY_RC);
    SX1280SetPacketType(RngModulationParams.PacketType);
    SX1280SetModulationParams(&RngModulationParams);
    SX1280SetPacketParams(&RngPacketParams);
    SX1280SetRfFrequency(RNG_RF_FREQUENCY);
    SX1280SetBufferBaseAddresses(0x00, 0x00);
    SX1280SetTxParams(RNG_TX_OUTPUT_POWER, SX1280_RADIO_RAMP_02_US);

    if (role == SX1280_RADIO_RANGING_ROLE_MASTER)
    {
        SX1280SetRangingRequestAddress(address);
        SX1280SetDioIrqParams( RngMasterIrqMask, RngMasterIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    }
    else
    {
        SX1280SetDeviceRangingAddress(address);
        SX1280SetDioIrqParams( RngSlaveIrqMask, RngSlaveIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    }

    switch (RngModulationParams.Params.LoRa.Bandwidth)
    {
    case SX1280_LORA_BW_0400:
        calibration = RNG_CALIB_0400[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    case SX1280_LORA_BW_0800:
        calibration = RNG_CALIB_0800[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    case SX1280_LORA_BW_1600:
        calibration = RNG_CALIB_1600[( RngModulationParams.Params.LoRa.SpreadingFactor >> 4u ) - 5u];
        break;
    
    default:
        calibration = 10000;
        break;
    }

    SX1280SetRangingCalibration(calibration);

    SX1280SetRangingRole(role);

    if (role == SX1280_RADIO_RANGING_ROLE_MASTER)
    {
        SX1280SetTx((SX1280_TickTime_t){RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_TX_TIMEOUT_VALUE});
    }
    else
    {
        SX1280SetRx((SX1280_TickTime_t) {RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_RX_TIMEOUT_VALUE});
    }
}

void LoRaSetPayloadLength(uint8_t length)
{
    RngPacketParams.Params.LoRa.PayloadLength = length;

    SX1280SetPacketParams(&RngPacketParams);
}

void LoRaSendData(uint8_t* txBuffer, uint8_t size)
{
    LoRaSetPayloadLength(size);
    SX1280SetTxParams(RNG_TX_OUTPUT_POWER, SX1280_RADIO_RAMP_02_US);
    SX1280SetDioIrqParams(RngTxIrqMask, RngTxIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    SX1280SendPayload(txBuffer, size, (SX1280_TickTime_t){RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_TX_TIMEOUT_VALUE});
}

void LoRaSetRx( void )
{
    SX1280SetDioIrqParams(RngRxIrqMask, RngRxIrqMask, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
    SX1280SetRx((SX1280_TickTime_t) {RANGING_RX_TIMEOUT_TICK_SIZE, RANGING_RX_TIMEOUT_VALUE});
}

