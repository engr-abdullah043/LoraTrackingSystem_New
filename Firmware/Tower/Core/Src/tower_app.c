#include "tower_app.h"

#include <stdint.h>
#include <string.h>

#include "app_uart.h"
#include "lora_protocol.h"
#include "lora_radio.h"
#include "main.h"
#include "sx1262_bringup.h"

#define TOWER_ACK_GUARD_MS ( 6000U )

typedef enum
{
    TOWER_APP_DISABLED = 0,
    TOWER_APP_RECEIVING,
    TOWER_APP_WAIT_ACK_TX_DONE
} tower_app_state_t;

static tower_app_state_t state = TOWER_APP_DISABLED;
static bool have_last_packet;
static lora_protocol_message_t last_packet;
static uint32_t ack_deadline_ms;

static bool time_reached( uint32_t now, uint32_t deadline )
{
    return ( ( int32_t ) ( now - deadline ) >= 0 );
}

static void restore_receive( void )
{
    if( lora_radio_start_continuous_rx() == LORA_RADIO_OK )
    {
        state = TOWER_APP_RECEIVING;
    }
    else
    {
        state = TOWER_APP_DISABLED;
        ( void ) app_uart_write_line( "TOWER: FAIL restoring continuous RX" );
    }
}

static void log_packet( const lora_protocol_message_t* message, const lora_radio_packet_status_t* status,
                        bool duplicate )
{
    ( void ) app_uart_write( duplicate ? "TOWER: duplicate DATA node=" : "TOWER: DATA node=" );
    ( void ) app_uart_write( message->node_id );
    ( void ) app_uart_write( ", sequence=" );
    ( void ) app_uart_write_u32( message->sequence );
    ( void ) app_uart_write( ", RSSI=" );
    ( void ) app_uart_write_i8( status->rssi_dbm );
    ( void ) app_uart_write( " dBm, SNR=" );
    ( void ) app_uart_write_i8( status->snr_db );
    ( void ) app_uart_write_line( " dB" );
}

static void handle_data( void )
{
    uint8_t payload[LORA_PROTOCOL_PACKET_MAX_LEN];
    uint8_t length;
    uint8_t ack[LORA_PROTOCOL_PACKET_MAX_LEN];
    uint8_t ack_length;
    lora_protocol_message_t message;
    lora_radio_packet_status_t status;
    bool duplicate;

    if( lora_radio_read_packet( payload, sizeof( payload ), &length, &status ) != LORA_RADIO_OK )
    {
        ( void ) app_uart_write_line( "TOWER: packet read error" );
        restore_receive();
        return;
    }
    if( !lora_protocol_parse_data( payload, length, &message ) )
    {
        ( void ) app_uart_write_line( "TOWER: invalid DATA ignored" );
        restore_receive();
        return;
    }
    duplicate = have_last_packet && ( last_packet.sequence == message.sequence ) &&
                ( strcmp( last_packet.node_id, message.node_id ) == 0 );
    log_packet( &message, &status, duplicate );
    if( !duplicate )
    {
        last_packet = message;
        have_last_packet = true;
    }
    if( !lora_protocol_format_ack( message.node_id, message.sequence, ack, sizeof( ack ), &ack_length ) ||
        ( lora_radio_start_tx( ack, ack_length ) != LORA_RADIO_OK ) )
    {
        ( void ) app_uart_write_line( "TOWER: ACK start error" );
        restore_receive();
        return;
    }
    ( void ) app_uart_write( "TOWER: ACK TX node=" );
    ( void ) app_uart_write( message.node_id );
    ( void ) app_uart_write( ", sequence=" );
    ( void ) app_uart_write_u32( message.sequence );
    ( void ) app_uart_write_line( "" );
    ack_deadline_ms = HAL_GetTick() + TOWER_ACK_GUARD_MS;
    state = TOWER_APP_WAIT_ACK_TX_DONE;
}

bool tower_app_init( void )
{
    state = TOWER_APP_DISABLED;
    if( sx1262_bringup_run() != SX1262_BRINGUP_OK )
    {
        ( void ) app_uart_write_line( "TOWER LINK: FAIL (bring-up)" );
        return false;
    }
    if( lora_radio_init() != LORA_RADIO_OK )
    {
        ( void ) app_uart_write_line( "TOWER LINK: FAIL (radio init)" );
        return false;
    }
    have_last_packet = false;
    if( lora_radio_start_continuous_rx() != LORA_RADIO_OK )
    {
        ( void ) app_uart_write_line( "TOWER LINK: FAIL (RX start)" );
        return false;
    }
    state = TOWER_APP_RECEIVING;
    ( void ) app_uart_write_line( "TOWER LINK: LISTENING 915MHz SF12 BW125 CR4/8 +22dBm" );
    return true;
}

void tower_app_process( void )
{
    bool has_event;
    sx126x_irq_mask_t irq;
    if( state == TOWER_APP_DISABLED ) return;
    if( lora_radio_take_irq( &has_event, &irq ) != LORA_RADIO_OK )
    {
        ( void ) app_uart_write_line( "TOWER: IRQ read error" );
        restore_receive();
        return;
    }
    if( state == TOWER_APP_WAIT_ACK_TX_DONE )
    {
        if( has_event && ( ( irq & SX126X_IRQ_TX_DONE ) != 0U ) )
        {
            ( void ) app_uart_write_line( "TOWER: ACK TX done" );
            restore_receive();
        }
        else if( ( has_event && ( ( irq & SX126X_IRQ_TIMEOUT ) != 0U ) ) ||
                 time_reached( HAL_GetTick(), ack_deadline_ms ) )
        {
            ( void ) app_uart_write_line( "TOWER: ACK TX timeout" );
            restore_receive();
        }
        return;
    }
    if( !has_event ) return;
    if( ( irq & SX126X_IRQ_RX_DONE ) != 0U )
    {
        handle_data();
    }
    else if( ( irq & SX126X_IRQ_CRC_ERROR ) != 0U )
    {
        ( void ) app_uart_write_line( "TOWER: RX CRC error" );
        restore_receive();
    }
    else if( ( irq & SX126X_IRQ_HEADER_ERROR ) != 0U )
    {
        ( void ) app_uart_write_line( "TOWER: RX header error" );
        restore_receive();
    }
    else if( ( irq & SX126X_IRQ_TIMEOUT ) != 0U )
    {
        ( void ) app_uart_write_line( "TOWER: RX timeout, restarting" );
        restore_receive();
    }
}
