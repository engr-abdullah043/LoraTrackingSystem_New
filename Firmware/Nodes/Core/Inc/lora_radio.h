#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <stdbool.h>
#include <stdint.h>

#include "sx126x.h"

#define LORA_RADIO_MAX_PAYLOAD ( 64U )

typedef enum
{
    LORA_RADIO_OK = 0,
    LORA_RADIO_INVALID_ARGUMENT,
    LORA_RADIO_DRIVER_ERROR,
    LORA_RADIO_BOARD_ERROR
} lora_radio_result_t;

typedef struct
{
    int8_t rssi_dbm;
    int8_t snr_db;
} lora_radio_packet_status_t;

lora_radio_result_t lora_radio_init( void );
lora_radio_result_t lora_radio_start_tx( const uint8_t* payload, uint8_t length );
lora_radio_result_t lora_radio_start_rx( uint32_t timeout_ms );
lora_radio_result_t lora_radio_start_continuous_rx( void );
lora_radio_result_t lora_radio_take_irq( bool* has_event, sx126x_irq_mask_t* irq );
lora_radio_result_t lora_radio_read_packet( uint8_t* payload, uint8_t capacity, uint8_t* length,
                                            lora_radio_packet_status_t* status );

#endif
