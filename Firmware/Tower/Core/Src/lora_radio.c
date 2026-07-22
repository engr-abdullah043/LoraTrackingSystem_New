#include "lora_radio.h"

#include <stddef.h>

#include "main.h"
#include "sx1262_board.h"

#define LORA_FREQUENCY_HZ       ( 915000000UL )
#define LORA_TX_TIMEOUT_MS       ( 5000U )
#define LORA_TX_BUFFER_BASE      ( 0x00U )
#define LORA_RX_BUFFER_BASE      ( 0x80U )
#define LORA_PRIVATE_SYNC_WORD   ( 0x12U )

static sx126x_pkt_params_lora_t packet_params = {
    .preamble_len_in_symb = 12U,
    .header_type = SX126X_LORA_PKT_EXPLICIT,
    .pld_len_in_bytes = LORA_RADIO_MAX_PAYLOAD,
    .crc_is_on = true,
    .invert_iq_is_on = false,
};

static lora_radio_result_t map_status( sx126x_status_t status )
{
    if( status == SX126X_STATUS_OK )
    {
        return LORA_RADIO_OK;
    }
    return ( sx1262_board_last_error() == SX1262_BOARD_ERROR_NONE ) ? LORA_RADIO_DRIVER_ERROR
                                                                    : LORA_RADIO_BOARD_ERROR;
}

static lora_radio_result_t set_packet_length( uint8_t length )
{
    packet_params.pld_len_in_bytes = length;
    return map_status( sx126x_set_lora_pkt_params( sx1262_board_context(), &packet_params ) );
}

lora_radio_result_t lora_radio_init( void )
{
    const void* context = sx1262_board_context();
    const sx126x_pa_cfg_params_t pa = { .pa_duty_cycle = 0x04U, .hp_max = 0x07U, .device_sel = 0x00U,
                                         .pa_lut = 0x01U };
    const sx126x_mod_params_lora_t modulation = { .sf = SX126X_LORA_SF12, .bw = SX126X_LORA_BW_125,
                                                   .cr = SX126X_LORA_CR_4_8, .ldro = 1U };
    sx126x_status_t result;

    HAL_GPIO_WritePin( RF_SW_SPI_GPIO_Port, RF_SW_SPI_Pin, GPIO_PIN_RESET );
    result = sx126x_set_standby( context, SX126X_STANDBY_CFG_RC );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_reg_mode( context, SX126X_REG_MODE_DCDC );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_dio2_as_rf_sw_ctrl( context, true );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_dio3_as_tcxo_ctrl( context, SX126X_TCXO_CTRL_3_0V,
                                           sx126x_convert_timeout_in_ms_to_rtc_step( 5U ) );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_cal_img( context, 0xE1U, 0xE9U );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_pkt_type( context, SX126X_PKT_TYPE_LORA );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_rf_freq( context, LORA_FREQUENCY_HZ );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_pa_cfg( context, &pa );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_tx_params( context, 0, SX126X_RAMP_200_US );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_lora_sync_word( context, LORA_PRIVATE_SYNC_WORD );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_lora_mod_params( context, &modulation );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    if( set_packet_length( LORA_RADIO_MAX_PAYLOAD ) != LORA_RADIO_OK ) return map_status( SX126X_STATUS_ERROR );
    result = sx126x_set_buffer_base_address( context, LORA_TX_BUFFER_BASE, LORA_RX_BUFFER_BASE );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    return map_status( sx126x_clear_irq_status( context, SX126X_IRQ_ALL ) );
}

lora_radio_result_t lora_radio_start_tx( const uint8_t* payload, uint8_t length )
{
    const void* context = sx1262_board_context();
    sx126x_status_t result;
    lora_radio_result_t mapped;
    const uint16_t mask = SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT;
    if( ( payload == NULL ) || ( length == 0U ) || ( length > LORA_RADIO_MAX_PAYLOAD ) )
    {
        return LORA_RADIO_INVALID_ARGUMENT;
    }
    mapped = set_packet_length( length );
    if( mapped != LORA_RADIO_OK ) return mapped;
    result = sx126x_write_buffer( context, LORA_TX_BUFFER_BASE, payload, length );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_set_dio_irq_params( context, mask, mask, SX126X_IRQ_NONE, SX126X_IRQ_NONE );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_clear_irq_status( context, SX126X_IRQ_ALL );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    return map_status( sx126x_set_tx( context, LORA_TX_TIMEOUT_MS ) );
}

static lora_radio_result_t start_rx( uint32_t timeout_ms )
{
    const void* context = sx1262_board_context();
    const uint16_t mask = SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT;
    sx126x_status_t result;
    lora_radio_result_t mapped = set_packet_length( LORA_RADIO_MAX_PAYLOAD );
    if( mapped != LORA_RADIO_OK ) return mapped;
    result = sx126x_set_dio_irq_params( context, mask, mask, SX126X_IRQ_NONE, SX126X_IRQ_NONE );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_clear_irq_status( context, SX126X_IRQ_ALL );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    if( timeout_ms == SX126X_RX_CONTINUOUS )
    {
        return map_status( sx126x_set_rx_with_timeout_in_rtc_step( context, SX126X_RX_CONTINUOUS ) );
    }
    return map_status( sx126x_set_rx( context, timeout_ms ) );
}

lora_radio_result_t lora_radio_start_rx( uint32_t timeout_ms )
{
    if( timeout_ms == 0U ) return LORA_RADIO_INVALID_ARGUMENT;
    return start_rx( timeout_ms );
}

lora_radio_result_t lora_radio_start_continuous_rx( void )
{
    return start_rx( SX126X_RX_CONTINUOUS );
}

lora_radio_result_t lora_radio_take_irq( bool* has_event, sx126x_irq_mask_t* irq )
{
    sx126x_status_t result;
    if( ( has_event == NULL ) || ( irq == NULL ) ) return LORA_RADIO_INVALID_ARGUMENT;
    *has_event = false;
    *irq = SX126X_IRQ_NONE;
    if( !sx1262_board_take_dio1_event() ) return LORA_RADIO_OK;
    result = sx126x_get_irq_status( sx1262_board_context(), irq );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_clear_irq_status( sx1262_board_context(), *irq );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    *has_event = true;
    return LORA_RADIO_OK;
}

lora_radio_result_t lora_radio_read_packet( uint8_t* payload, uint8_t capacity, uint8_t* length,
                                            lora_radio_packet_status_t* status )
{
    sx126x_rx_buffer_status_t buffer_status;
    sx126x_pkt_status_lora_t packet_status;
    sx126x_status_t result;
    if( ( payload == NULL ) || ( length == NULL ) || ( status == NULL ) || ( capacity == 0U ) )
        return LORA_RADIO_INVALID_ARGUMENT;
    result = sx126x_get_rx_buffer_status( sx1262_board_context(), &buffer_status );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    if( ( buffer_status.pld_len_in_bytes == 0U ) || ( buffer_status.pld_len_in_bytes > capacity ) ||
        ( buffer_status.pld_len_in_bytes > LORA_RADIO_MAX_PAYLOAD ) ) return LORA_RADIO_INVALID_ARGUMENT;
    result = sx126x_read_buffer( sx1262_board_context(), buffer_status.buffer_start_pointer, payload,
                                buffer_status.pld_len_in_bytes );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    result = sx126x_get_lora_pkt_status( sx1262_board_context(), &packet_status );
    if( result != SX126X_STATUS_OK ) return map_status( result );
    *length = buffer_status.pld_len_in_bytes;
    status->rssi_dbm = packet_status.rssi_pkt_in_dbm;
    status->snr_db = packet_status.snr_pkt_in_db;
    return LORA_RADIO_OK;
}


