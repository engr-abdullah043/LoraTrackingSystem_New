#include "sx1262_board.h"

#include <stddef.h>

#include "main.h"
#include "spi.h"
#include "sx126x_hal.h"

#define SX1262_BUSY_TIMEOUT_MS ( 100U )
#define SX1262_SPI_TIMEOUT_MS ( 100U )
#define SX1262_RESET_LOW_MS ( 1U )
#define SX1262_RESET_SETTLE_MS ( 10U )
#define SX1262_GET_STATUS_OPCODE ( 0xC0U )

typedef struct
{
    SPI_HandleTypeDef*    spi;
    GPIO_TypeDef*         nss_port;
    uint16_t              nss_pin;
    GPIO_TypeDef*         busy_port;
    uint16_t              busy_pin;
    GPIO_TypeDef*         reset_port;
    uint16_t              reset_pin;
    uint32_t              busy_timeout_ms;
    uint32_t              spi_timeout_ms;
    sx1262_board_error_t  last_error;
    uint8_t               last_status_byte;
} sx1262_board_context_t;

static sx1262_board_context_t board_context = {
    .spi                  = &hspi1,
    .nss_port             = NSS_SPI_GPIO_Port,
    .nss_pin              = NSS_SPI_Pin,
    .busy_port            = BUSY_LoRa_GPIO_Port,
    .busy_pin             = BUSY_LoRa_Pin,
    .reset_port           = RST_Lora_GPIO_Port,
    .reset_pin            = RST_Lora_Pin,
    .busy_timeout_ms      = SX1262_BUSY_TIMEOUT_MS,
    .spi_timeout_ms       = SX1262_SPI_TIMEOUT_MS,
    .last_error           = SX1262_BOARD_ERROR_NONE,
    .last_status_byte     = 0U,
};

static volatile bool dio1_event_pending;

static sx1262_board_context_t* sx1262_board_validate_context( const void* context )
{
    if( context != &board_context )
    {
        board_context.last_error = SX1262_BOARD_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    return &board_context;
}

static bool sx1262_board_wait_while_busy( sx1262_board_context_t* context )
{
    const uint32_t start = HAL_GetTick();

    while( HAL_GPIO_ReadPin( context->busy_port, context->busy_pin ) == GPIO_PIN_SET )
    {
        if( ( HAL_GetTick() - start ) >= context->busy_timeout_ms )
        {
            context->last_error = SX1262_BOARD_ERROR_BUSY_TIMEOUT;
            return false;
        }
    }
    return true;
}

const void* sx1262_board_context( void )
{
    return &board_context;
}

sx1262_board_error_t sx1262_board_last_error( void )
{
    return board_context.last_error;
}

uint8_t sx1262_board_last_status_byte( void )
{
    return board_context.last_status_byte;
}

bool sx1262_board_take_dio1_event( void )
{
    const bool was_pending = dio1_event_pending;
    dio1_event_pending     = false;
    return was_pending;
}

sx126x_hal_status_t sx126x_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    sx1262_board_context_t* board = sx1262_board_validate_context( context );

    if( ( board == NULL ) || ( command == NULL ) || ( command_length == 0U ) ||
        ( ( data_length > 0U ) && ( data == NULL ) ) )
    {
        board_context.last_error = SX1262_BOARD_ERROR_INVALID_ARGUMENT;
        return SX126X_HAL_STATUS_ERROR;
    }

    board->last_error = SX1262_BOARD_ERROR_NONE;
    if( !sx1262_board_wait_while_busy( board ) )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_RESET );
    if( HAL_SPI_Transmit( board->spi, command, command_length, board->spi_timeout_ms ) != HAL_OK )
    {
        board->last_error = SX1262_BOARD_ERROR_SPI;
        HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
        return SX126X_HAL_STATUS_ERROR;
    }
    if( ( data_length > 0U ) &&
        ( HAL_SPI_Transmit( board->spi, data, data_length, board->spi_timeout_ms ) != HAL_OK ) )
    {
        board->last_error = SX1262_BOARD_ERROR_SPI;
        HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
        return SX126X_HAL_STATUS_ERROR;
    }
    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    sx1262_board_context_t* board = sx1262_board_validate_context( context );

    if( ( board == NULL ) || ( command == NULL ) || ( command_length == 0U ) || ( data == NULL ) ||
        ( data_length == 0U ) )
    {
        board_context.last_error = SX1262_BOARD_ERROR_INVALID_ARGUMENT;
        return SX126X_HAL_STATUS_ERROR;
    }

    board->last_error = SX1262_BOARD_ERROR_NONE;
    if( !sx1262_board_wait_while_busy( board ) )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_RESET );
    if( HAL_SPI_Transmit( board->spi, command, command_length, board->spi_timeout_ms ) != HAL_OK )
    {
        board->last_error = SX1262_BOARD_ERROR_SPI;
        HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
        return SX126X_HAL_STATUS_ERROR;
    }
    if( HAL_SPI_Receive( board->spi, data, data_length, board->spi_timeout_ms ) != HAL_OK )
    {
        board->last_error = SX1262_BOARD_ERROR_SPI;
        HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
        return SX126X_HAL_STATUS_ERROR;
    }
    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
    board->last_status_byte = data[0];
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset( const void* context )
{
    sx1262_board_context_t* board = sx1262_board_validate_context( context );

    if( board == NULL )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    board->last_error = SX1262_BOARD_ERROR_NONE;
    HAL_GPIO_WritePin( board->reset_port, board->reset_pin, GPIO_PIN_RESET );
    HAL_Delay( SX1262_RESET_LOW_MS );
    HAL_GPIO_WritePin( board->reset_port, board->reset_pin, GPIO_PIN_SET );
    HAL_Delay( SX1262_RESET_SETTLE_MS );

    return sx1262_board_wait_while_busy( board ) ? SX126X_HAL_STATUS_OK : SX126X_HAL_STATUS_ERROR;
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context )
{
    static const uint8_t wakeup_command[] = { SX1262_GET_STATUS_OPCODE, SX126X_NOP };
    sx1262_board_context_t* board = sx1262_board_validate_context( context );

    if( board == NULL )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    board->last_error = SX1262_BOARD_ERROR_NONE;
    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_RESET );
    if( HAL_SPI_Transmit( board->spi, wakeup_command, sizeof( wakeup_command ), board->spi_timeout_ms ) != HAL_OK )
    {
        board->last_error = SX1262_BOARD_ERROR_SPI;
        HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );
        return SX126X_HAL_STATUS_ERROR;
    }
    HAL_GPIO_WritePin( board->nss_port, board->nss_pin, GPIO_PIN_SET );

    return sx1262_board_wait_while_busy( board ) ? SX126X_HAL_STATUS_OK : SX126X_HAL_STATUS_ERROR;
}

void HAL_GPIO_EXTI_Rising_Callback( uint16_t GPIO_Pin )
{
    if( GPIO_Pin == DIO1_Pin )
    {
        dio1_event_pending = true;
    }
}
