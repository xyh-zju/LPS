#ifndef __SX1280_HAL_H
#define __SX1280_HAL_H
#include "main.h"
#include "sx1280.h"
#define bool _Bool
#define true 1
#define false 0
#define SX1280_IRAM_START_ADDRESS                          0x8000
#define SX1280_IRAM_SIZE                                   0x4000

#define SX1280_RADIO_DIO1_ENABLE	1
#define SX1280_RADIO_DIO2_ENABLE	0
#define SX1280_RADIO_DIO3_ENABLE	0
#ifndef __RADIO_COMMANDS
#define __RADIO_COMMANDS
typedef enum SX1280_RadioCommands_u
{
    SX1280_RADIO_GET_STATUS                        = 0xC0,
    SX1280_RADIO_WRITE_REGISTER                    = 0x18,
    SX1280_RADIO_READ_REGISTER                     = 0x19,
    SX1280_RADIO_WRITE_BUFFER                      = 0x1A,
    SX1280_RADIO_READ_BUFFER                       = 0x1B,
    SX1280_RADIO_SET_SLEEP                         = 0x84,
    SX1280_RADIO_SET_STANDBY                       = 0x80,
    SX1280_RADIO_SET_FS                            = 0xC1,
    SX1280_RADIO_SET_TX                            = 0x83,
    SX1280_RADIO_SET_RX                            = 0x82,
    SX1280_RADIO_SET_RXDUTYCYCLE                   = 0x94,
    SX1280_RADIO_SET_CAD                           = 0xC5,
    SX1280_RADIO_SET_TXCONTINUOUSWAVE              = 0xD1,
    SX1280_RADIO_SET_TXCONTINUOUSPREAMBLE          = 0xD2,
    SX1280_RADIO_SET_PACKETTYPE                    = 0x8A,
    SX1280_RADIO_GET_PACKETTYPE                    = 0x03,
    SX1280_RADIO_SET_RFFREQUENCY                   = 0x86,
    SX1280_RADIO_SET_TXPARAMS                      = 0x8E,
    SX1280_RADIO_SET_CADPARAMS                     = 0x88,
    SX1280_RADIO_SET_BUFFERBASEADDRESS             = 0x8F,
    SX1280_RADIO_SET_MODULATIONPARAMS              = 0x8B,
    SX1280_RADIO_SET_PACKETPARAMS                  = 0x8C,
    SX1280_RADIO_GET_RXBUFFERSTATUS                = 0x17,
    SX1280_RADIO_GET_PACKETSTATUS                  = 0x1D,
    SX1280_RADIO_GET_RSSIINST                      = 0x1F,
    SX1280_RADIO_SET_DIOIRQPARAMS                  = 0x8D,
    SX1280_RADIO_GET_IRQSTATUS                     = 0x15,
    SX1280_RADIO_CLR_IRQSTATUS                     = 0x97,
    SX1280_RADIO_CALIBRATE                         = 0x89,
    SX1280_RADIO_SET_REGULATORMODE                 = 0x96,
    SX1280_RADIO_SET_SAVECONTEXT                   = 0xD5,
    SX1280_RADIO_SET_AUTOTX                        = 0x98,
    SX1280_RADIO_SET_AUTOFS                        = 0x9E,
    SX1280_RADIO_SET_LONGPREAMBLE                  = 0x9B,
    SX1280_RADIO_SET_UARTSPEED                     = 0x9D,
    SX1280_RADIO_SET_RANGING_ROLE                  = 0xA3,
}SX1280_RadioCommands_t;
#endif
void SX1280HalWaitOnBusy( void );
void SX1280HalInit( void );
void SX1280HalIoInit( void );
void SX1280HalReset( void );
void SX1280HalClearInstructionRam( void );
void SX1280HalWakeup( void );
/*!
 * \brief Send a command that write data to the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [in]  buffer        Buffer to be send to the radio
 * \param [in]  size          Size of the buffer to send
 */
void SX1280HalWriteCommand( SX1280_RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

/*!
 * \brief Send a command that read data from the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [out] buffer        Buffer holding data from the radio
 * \param [in]  size          Size of the buffer
 */
void SX1280HalReadCommand( SX1280_RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

/*!
 * \brief Write data to the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 * \param [in]  buffer        The data to be written in radio's memory
 * \param [in]  size          The number of bytes to write in radio's memory
 */
void SX1280HalWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size );

/*!
 * \brief Write a single byte of data to the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 * \param [in]  value         The data to be written in radio's memory
 */
void SX1280HalWriteRegister( uint16_t address, uint8_t value );

/*!
 * \brief Read data from the radio memory
 *
 * \param [in]  address       The address of the first byte to read from the radio
 * \param [out] buffer        The buffer that holds data read from radio
 * \param [in]  size          The number of bytes to read from radio's memory
 */
void SX1280HalReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size );

/*!
 * \brief Read a single byte of data from the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the
     *                            radio
 *
 * \retval      value         The value of the byte at the given address in
     *                            radio's memory
 */
uint8_t SX1280HalReadRegister( uint16_t address );

/*!
 * \brief Write data to the buffer holding the payload in the radio
 *
 * \param [in]  offset        The offset to start writing the payload
 * \param [in]  buffer        The data to be written (the payload)
 * \param [in]  size          The number of byte to be written
 */
void SX1280HalWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

/*!
 * \brief Read data from the buffer holding the payload in the radio
 *
 * \param [in]  offset        The offset to start reading the payload
 * \param [out] buffer        A pointer to a buffer holding the data from the radio
 * \param [in]  size          The number of byte to be read
 */
void SX1280HalReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

/*!
 * \brief Returns the status of DIOs pins
 *
 * \retval      dioStatus     A byte where each bit represents a DIO state:
 *                            [ DIOx | BUSY ]
 */
uint8_t SX1280HalGetDioStatus( void );

#endif
