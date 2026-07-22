#include "node_app.h"

#include <stdint.h>

#include "app_uart.h"
#include "lora_protocol.h"
#include "lora_radio.h"
#include "main.h"
#include "sx1262_bringup.h"

#define NODE_ID                    "NODE01"
#define NODE_MAX_ATTEMPTS          ( 3U )
#define NODE_ACK_TIMEOUT_MS        ( 5000U )
#define NODE_TX_GUARD_MS            ( 6000U )
#define NODE_NEXT_PACKET_DELAY_MS  ( 10000U )

typedef enum
{
    NODE_APP_DISABLED = 0,
    NODE_APP_START_TX,
    NODE_APP_WAIT_TX_DONE,
    NODE_APP_WAIT_ACK,
    NODE_APP_WAIT_NEXT
} node_app_state_t;

static node_app_state_t state = NODE_APP_DISABLED;
static uint32_t sequence = 1U;
static uint32_t deadline_ms;
static uint8_t attempt;
static uint8_t tx_payload[LORA_PROTOCOL_PACKET_MAX_LEN];
static uint8_t tx_length;

static bool time_reached( uint32_t now, uint32_t deadline )
{
    return ( ( int32_t ) ( now - deadline ) >= 0 );
}

static void log_sequence( const char* prefix )
{
    ( void ) app_uart_write( prefix );
    ( void ) app_uart_write_u32( sequence );
    ( void ) app_uart_write( ", attempt=" );
    ( void ) app_uart_write_u32( attempt );
    ( void ) app_uart_write_line( "" );
}

static void finish_transaction( bool success )
{
    ( void ) app_uart_write( success ? "NODE: ACK success, sequence=" : "NODE: final failure, sequence=" );
    ( void ) app_uart_write_u32( sequence );
    ( void ) app_uart_write_line( "" );
    sequence = lora_protocol_next_sequence( sequence );
    deadline_ms = HAL_GetTick() + NODE_NEXT_PACKET_DELAY_MS;
    state = NODE_APP_WAIT_NEXT;
}

static void fail_attempt( const char* reason )
{
    ( void ) app_uart_write( "NODE: " );
    ( void ) app_uart_write_line( reason );
    if( attempt < NODE_MAX_ATTEMPTS )
    {
        ++attempt;
        state = NODE_APP_START_TX;
        log_sequence( "NODE: retry sequence=" );
    }
    else
    {
        finish_transaction( false );
    }
}

static void start_attempt( void )
{
    if( !lora_protocol_format_data( NODE_ID, sequence, tx_payload, sizeof( tx_payload ), &tx_length ) )
    {
        finish_transaction( false );
        return;
    }
    log_sequence( "NODE: TX sequence=" );
    if( lora_radio_start_tx( tx_payload, tx_length ) != LORA_RADIO_OK )
    {
        fail_attempt( "TX start error" );
        return;
    }
    deadline_ms = HAL_GetTick() + NODE_TX_GUARD_MS;
    state = NODE_APP_WAIT_TX_DONE;
}

static void restart_ack_rx( uint32_t now )
{
    uint32_t remaining;
    if( time_reached( now, deadline_ms ) )
    {
        fail_attempt( "ACK timeout" );
        return;
    }
    remaining = deadline_ms - now;
    if( lora_radio_start_rx( remaining ) != LORA_RADIO_OK )
    {
        fail_attempt( "ACK RX restart error" );
    }
}

bool node_app_init( void )
{
    state = NODE_APP_DISABLED;
    if( sx1262_bringup_run() != SX1262_BRINGUP_OK )
    {
        ( void ) app_uart_write_line( "NODE LINK: FAIL (bring-up)" );
        return false;
    }
    if( lora_radio_init() != LORA_RADIO_OK )
    {
        ( void ) app_uart_write_line( "NODE LINK: FAIL (radio init)" );
        return false;
    }
    sequence = 1U;
    attempt = 1U;
    state = NODE_APP_START_TX;
    ( void ) app_uart_write_line( "NODE LINK: READY 915MHz SF12 BW125 CR4/8 +22dBm" );
    return true;
}

void node_app_process( void )
{
    bool has_event;
    sx126x_irq_mask_t irq;
    uint32_t now = HAL_GetTick();
    uint8_t rx_payload[LORA_PROTOCOL_PACKET_MAX_LEN];
    uint8_t rx_length;
    lora_radio_packet_status_t packet_status;
    lora_radio_result_t radio_result;

    if( state == NODE_APP_DISABLED ) return;
    if( state == NODE_APP_START_TX )
    {
        start_attempt();
        return;
    }
    if( state == NODE_APP_WAIT_NEXT )
    {
        if( time_reached( now, deadline_ms ) )
        {
            attempt = 1U;
            state = NODE_APP_START_TX;
        }
        return;
    }

    radio_result = lora_radio_take_irq( &has_event, &irq );
    if( radio_result != LORA_RADIO_OK )
    {
        fail_attempt( "IRQ read error" );
        return;
    }

    if( state == NODE_APP_WAIT_TX_DONE )
    {
        if( !has_event )
        {
            if( time_reached( now, deadline_ms ) ) fail_attempt( "TX interrupt timeout" );
            return;
        }
        if( ( irq & SX126X_IRQ_TX_DONE ) != 0U )
        {
            ( void ) app_uart_write_line( "NODE: TX done, waiting for ACK" );
            deadline_ms = now + NODE_ACK_TIMEOUT_MS;
            if( lora_radio_start_rx( NODE_ACK_TIMEOUT_MS ) != LORA_RADIO_OK ) fail_attempt( "ACK RX start error" );
            else state = NODE_APP_WAIT_ACK;
        }
        else if( ( irq & SX126X_IRQ_TIMEOUT ) != 0U ) fail_attempt( "TX timeout" );
        return;
    }

    if( !has_event )
    {
        if( time_reached( now, deadline_ms ) ) fail_attempt( "ACK timeout" );
        return;
    }
    if( ( irq & SX126X_IRQ_RX_DONE ) != 0U )
    {
        if( lora_radio_read_packet( rx_payload, sizeof( rx_payload ), &rx_length, &packet_status ) != LORA_RADIO_OK )
        {
            restart_ack_rx( now );
        }
        else if( lora_protocol_ack_matches( rx_payload, rx_length, NODE_ID, sequence ) )
        {
            ( void ) app_uart_write( "NODE: matching ACK, RSSI=" );
            ( void ) app_uart_write_i8( packet_status.rssi_dbm );
            ( void ) app_uart_write( " dBm, SNR=" );
            ( void ) app_uart_write_i8( packet_status.snr_db );
            ( void ) app_uart_write_line( " dB" );
            finish_transaction( true );
        }
        else
        {
            ( void ) app_uart_write_line( "NODE: ignored mismatched ACK" );
            restart_ack_rx( now );
        }
    }
    else if( ( irq & ( SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR ) ) != 0U )
    {
        ( void ) app_uart_write_line( "NODE: ignored invalid ACK" );
        restart_ack_rx( now );
    }
    else if( ( irq & SX126X_IRQ_TIMEOUT ) != 0U ) fail_attempt( "ACK timeout" );
}

