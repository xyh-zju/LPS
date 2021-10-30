#ifndef __RANGING_H
#define __RANGING_H
#include "main.h"
#include "sx1280-hal.h"
#include "sx1280.h"

/*!
 * \brief Defines the nominal frequency
 */
#define RNG_RF_FREQUENCY                                2426000000 // Hz
/*!
 * \brief Defines the output power in dBm
 *
 * \remark The range of the output power is [-18..+13] dBm
 */
#define RNG_TX_OUTPUT_POWER                             13
/*!
 * \brief Defines the buffer size, i.e. the payload size
 */
#define RNG_BUFFER_SIZE                                 20
/*!
 * \brief The device address
 */
#define RANGING_DIVICE_ADDR                             0x00010001
/*!
 * \brief Number of tick size steps for tx timeout
 */
#define RANGING_TX_TIMEOUT_VALUE                            10000 // ms

/*!
 * \brief Number of tick size steps for rx timeout
 */
#define RANGING_RX_TIMEOUT_VALUE                            0xffff // ms

/*!
 * \brief Size of ticks (used for Tx and Rx timeout)
 */
#define RANGING_RX_TIMEOUT_TICK_SIZE                        SX1280_RADIO_TICK_SIZE_1000_US

void RangingSetParams( void );
void RangingInitRadio( void );
void RangingInit(SX1280_RadioRangingRoles_t role, uint32_t address);
void RangingStart(SX1280_RadioRangingRoles_t role, uint32_t address);
void LoRaSetPayloadLength(uint8_t length);
void LoRaSendData(uint8_t* txBuffer, uint8_t size);
void LoRaSetRx( void );
#endif
