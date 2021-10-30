#include "sx1280-hal.h"
/*!
 * \brief Define the size of tx and rx hal buffers. should be much larger than maximal useful size
 */
#define MAX_HAL_BUFFER_SIZE   0xFF
static uint8_t halTxBuffer[MAX_HAL_BUFFER_SIZE] = {0x00};
static uint8_t halRxBuffer[MAX_HAL_BUFFER_SIZE] = {0x00};
/*!
 * \brief Used to block execution waiting for low state on radio busy pin.
 */
void SX1280HalWaitOnBusy( void )
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
    while( HAL_GPIO_ReadPin( SX1280_BUSY_GPIO_Port, SX1280_BUSY_Pin ) == 1 ) HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
}
/*!
 * \brief Reset SX1280. (Init Irq was removed when transplanting)
 */
void SX1280HalInit( void )
{
    SX1280HalReset( );
    // SX1280HalIoIrqInit( irqHandlers );
}
/*!
 * \brief Reset SX1280.
 */
void SX1280HalReset( void )
{
    HAL_Delay( 20 );
    HAL_GPIO_WritePin( SX1280_NRESET_GPIO_Port, SX1280_NRESET_Pin, GPIO_PIN_RESET );
    HAL_Delay( 50 );
    HAL_GPIO_WritePin( SX1280_NRESET_GPIO_Port, SX1280_NRESET_Pin, GPIO_PIN_SET );
    HAL_Delay( 20 );
}
void SpiIn(uint8_t *txBuffer, uint16_t size)
{
    HAL_SPI_Transmit(&hspi1, txBuffer, size, HAL_MAX_DELAY);
}
void SpiInOut(uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t size)
{
    HAL_SPIEx_FlushRxFifo( &hspi1 );
    HAL_SPI_TransmitReceive(&hspi1, txBuffer, rxBuffer, size, HAL_MAX_DELAY);
}
/*!
 * \brief writing 0x00s on every bytes of the instruction RAM
 */
void SX1280HalClearInstructionRam( void )
{
    uint16_t halSize = 3 + SX1280_IRAM_SIZE;
    halTxBuffer[0] = SX1280_RADIO_WRITE_REGISTER;
    halTxBuffer[1] = ( SX1280_IRAM_START_ADDRESS >> 8 ) & 0x00FF;//address[15:8]
    halTxBuffer[2] = SX1280_IRAM_START_ADDRESS & 0x00FF;//address[7:0]
    for( uint16_t index = 0; index < SX1280_IRAM_SIZE; index++ )
    {
        halTxBuffer[3+index] = 0x00;
    }
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiIn( halTxBuffer, halSize );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
/*!
 * \brief make transceiver leave sleep mode
 */
void SX1280HalWakeup( void )
{
    //__disable_irq( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );//Transceiver can leave the Sleep Mode if NSS pin is driven low.
    uint16_t halSize = 2;
    halTxBuffer[0] = SX1280_RADIO_GET_STATUS;
    halTxBuffer[1] = 0x00;
    SpiIn( halTxBuffer, halSize );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
    //__enable_irq( );
}
void SX1280HalWriteCommand( SX1280_RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint16_t halSize  = size + 1;
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    halTxBuffer[0] = command;
    memcpy( halTxBuffer + 1, ( uint8_t * )buffer, size * sizeof( uint8_t ) );
    SpiIn( halTxBuffer, halSize );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    if( command != SX1280_RADIO_SET_SLEEP )
    {
        SX1280HalWaitOnBusy( );
    }
}
void SX1280HalReadCommand( SX1280_RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint16_t halSize = 2 + size;
    halTxBuffer[0] = command;
    halTxBuffer[1] = 0x00;
    for( uint16_t index = 0; index < size; index++ )
    {
        halTxBuffer[2+index] = 0x00;
    }
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiInOut( halTxBuffer, halRxBuffer, halSize );
    memcpy( buffer, halRxBuffer + 2, size );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
void SX1280HalWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    uint16_t halSize = size + 3;
    halTxBuffer[0] = SX1280_RADIO_WRITE_REGISTER;
    halTxBuffer[1] = ( address & 0xFF00 ) >> 8;
    halTxBuffer[2] = address & 0x00FF;
    memcpy( halTxBuffer + 3, buffer, size );
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiIn( halTxBuffer, halSize );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
void SX1280HalWriteRegister( uint16_t address, uint8_t value )
{
    SX1280HalWriteRegisters( address, &value, 1 );
}
void SX1280HalReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    uint16_t halSize = 4 + size;
    halTxBuffer[0] = SX1280_RADIO_READ_REGISTER;
    halTxBuffer[1] = ( address & 0xFF00 ) >> 8;
    halTxBuffer[2] = address & 0x00FF;
    halTxBuffer[3] = 0x00;
    for( uint16_t index = 0; index < size; index++ )
    {
        halTxBuffer[4+index] = 0x00;
    }
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiInOut( halTxBuffer, halRxBuffer, halSize );
    memcpy( buffer, halRxBuffer + 4, size );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
uint8_t SX1280HalReadRegister( uint16_t address )
{
    uint8_t data;
    SX1280HalReadRegisters( address, &data, 1 );
    return data;
}
void SX1280HalWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    uint16_t halSize = size + 2;
    halTxBuffer[0] = SX1280_RADIO_WRITE_BUFFER;
    halTxBuffer[1] = offset;
    memcpy( halTxBuffer + 2, buffer, size );
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiIn( halTxBuffer, halSize );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
void SX1280HalReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    uint16_t halSize = size + 3;
    halTxBuffer[0] = SX1280_RADIO_READ_BUFFER;
    halTxBuffer[1] = offset;
    halTxBuffer[2] = 0x00;
    for( uint16_t index = 0; index < size; index++ )
    {
        halTxBuffer[3+index] = 0x00;
    }
    SX1280HalWaitOnBusy( );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_RESET );
    SpiInOut( halTxBuffer, halRxBuffer, halSize );
    memcpy( buffer, halRxBuffer + 3, size );
    HAL_GPIO_WritePin( SX1280_NSS_CTS_GPIO_Port, SX1280_NSS_CTS_Pin, GPIO_PIN_SET );
    SX1280HalWaitOnBusy( );
}
uint8_t SX1280HalGetDioStatus( void )
{
	uint8_t Status = HAL_GPIO_ReadPin( SX1280_BUSY_GPIO_Port, SX1280_BUSY_Pin );
#if( SX1280_RADIO_DIO1_ENABLE )
	Status |= (HAL_GPIO_ReadPin( SX1280_DIO1_GPIO_Port, SX1280_DIO1_Pin ) << 1);
#endif
#if( SX1280_RADIO_DIO2_ENABLE )
	//Status |= (HAL_GPIO_ReadPin( DIO2_2G4_GPIO_Port, DIO2_2G4_Pin ) << 2);
#endif
#if( SX1280_RADIO_DIO3_ENABLE )
	//Status |= (HAL_GPIO_ReadPin( DIO3_2G4_GPIO_Port, DIO3_2G4_Pin ) << 3);
#endif
// #if( !SX1280_RADIO_DIO1_ENABLE && !SX1280_RADIO_DIO2_ENABLE && !SX1280_RADIO_DIO3_ENABLE )
// #error "Please define a DIO" 
// #endif
	return Status;
}






