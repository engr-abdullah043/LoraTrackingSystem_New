#ifndef SX1262_BOARD_H
#define SX1262_BOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SX1262_BOARD_ERROR_NONE = 0,
    SX1262_BOARD_ERROR_INVALID_ARGUMENT,
    SX1262_BOARD_ERROR_BUSY_TIMEOUT,
    SX1262_BOARD_ERROR_SPI
} sx1262_board_error_t;

const void*            sx1262_board_context( void );
sx1262_board_error_t   sx1262_board_last_error( void );
uint8_t                sx1262_board_last_status_byte( void );
bool                   sx1262_board_take_dio1_event( void );

#endif
