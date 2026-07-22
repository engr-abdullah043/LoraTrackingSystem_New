#include "lora_protocol.h"

#include <stddef.h>
#include <string.h>

#define LORA_PROTOCOL_SEQUENCE_DIGITS ( 6U )
#define LORA_PROTOCOL_DATA_TEXT       "I am alive"
#define LORA_PROTOCOL_DATA_TEXT_LEN   ( 10U )
#define LORA_PROTOCOL_DATA_LEN        ( 24U )
#define LORA_PROTOCOL_ACK_LEN         ( 17U )
#define LORA_PROTOCOL_SEQUENCE_MAX    ( 999999UL )

static bool node_id_is_valid( const char* node_id )
{
    uint8_t index;
    if( node_id == NULL )
    {
        return false;
    }
    for( index = 0U; index < LORA_PROTOCOL_NODE_ID_MAX_LEN; ++index )
    {
        const char value = node_id[index];
        if( !( ( value >= 'A' ) && ( value <= 'Z' ) ) && !( ( value >= '0' ) && ( value <= '9' ) ) )
        {
            return false;
        }
    }
    return node_id[LORA_PROTOCOL_NODE_ID_MAX_LEN] == '\0';
}

static bool sequence_is_valid( uint32_t sequence )
{
    return ( sequence >= 1U ) && ( sequence <= LORA_PROTOCOL_SEQUENCE_MAX );
}

static void encode_sequence( uint32_t sequence, uint8_t* output )
{
    int8_t index;
    for( index = ( int8_t ) LORA_PROTOCOL_SEQUENCE_DIGITS - 1; index >= 0; --index )
    {
        output[index] = ( uint8_t ) ( '0' + ( sequence % 10U ) );
        sequence /= 10U;
    }
}

static bool decode_sequence( const uint8_t* input, uint32_t* sequence )
{
    uint8_t  index;
    uint32_t value = 0U;
    if( ( input == NULL ) || ( sequence == NULL ) )
    {
        return false;
    }
    for( index = 0U; index < LORA_PROTOCOL_SEQUENCE_DIGITS; ++index )
    {
        if( ( input[index] < ( uint8_t ) '0' ) || ( input[index] > ( uint8_t ) '9' ) )
        {
            return false;
        }
        value = ( value * 10U ) + ( uint32_t ) ( input[index] - ( uint8_t ) '0' );
    }
    if( !sequence_is_valid( value ) )
    {
        return false;
    }
    *sequence = value;
    return true;
}

bool lora_protocol_format_data( const char* node_id, uint32_t sequence, uint8_t* output, uint8_t capacity,
                                uint8_t* output_length )
{
    if( !node_id_is_valid( node_id ) || !sequence_is_valid( sequence ) || ( output == NULL ) ||
        ( output_length == NULL ) || ( capacity < LORA_PROTOCOL_DATA_LEN ) )
    {
        return false;
    }
    memcpy( output, node_id, LORA_PROTOCOL_NODE_ID_MAX_LEN );
    output[6] = ( uint8_t ) '|';
    memcpy( &output[7], LORA_PROTOCOL_DATA_TEXT, LORA_PROTOCOL_DATA_TEXT_LEN );
    output[17] = ( uint8_t ) '|';
    encode_sequence( sequence, &output[18] );
    *output_length = LORA_PROTOCOL_DATA_LEN;
    return true;
}

bool lora_protocol_parse_data( const uint8_t* payload, uint8_t length, lora_protocol_message_t* message )
{
    char node_id[LORA_PROTOCOL_NODE_ID_MAX_LEN + 1U];
    if( ( payload == NULL ) || ( message == NULL ) || ( length != LORA_PROTOCOL_DATA_LEN ) ||
        ( payload[6] != ( uint8_t ) '|' ) || ( payload[17] != ( uint8_t ) '|' ) ||
        ( memcmp( &payload[7], LORA_PROTOCOL_DATA_TEXT, LORA_PROTOCOL_DATA_TEXT_LEN ) != 0 ) )
    {
        return false;
    }
    memcpy( node_id, payload, LORA_PROTOCOL_NODE_ID_MAX_LEN );
    node_id[LORA_PROTOCOL_NODE_ID_MAX_LEN] = '\0';
    if( !node_id_is_valid( node_id ) || !decode_sequence( &payload[18], &message->sequence ) )
    {
        return false;
    }
    memcpy( message->node_id, node_id, sizeof( node_id ) );
    return true;
}

bool lora_protocol_format_ack( const char* node_id, uint32_t sequence, uint8_t* output, uint8_t capacity,
                               uint8_t* output_length )
{
    if( !node_id_is_valid( node_id ) || !sequence_is_valid( sequence ) || ( output == NULL ) ||
        ( output_length == NULL ) || ( capacity < LORA_PROTOCOL_ACK_LEN ) )
    {
        return false;
    }
    memcpy( output, "ACK|", 4U );
    memcpy( &output[4], node_id, LORA_PROTOCOL_NODE_ID_MAX_LEN );
    output[10] = ( uint8_t ) '|';
    encode_sequence( sequence, &output[11] );
    *output_length = LORA_PROTOCOL_ACK_LEN;
    return true;
}

bool lora_protocol_ack_matches( const uint8_t* payload, uint8_t length, const char* expected_node_id,
                                uint32_t expected_sequence )
{
    uint32_t sequence;
    if( ( payload == NULL ) || !node_id_is_valid( expected_node_id ) || !sequence_is_valid( expected_sequence ) ||
        ( length != LORA_PROTOCOL_ACK_LEN ) || ( memcmp( payload, "ACK|", 4U ) != 0 ) ||
        ( memcmp( &payload[4], expected_node_id, LORA_PROTOCOL_NODE_ID_MAX_LEN ) != 0 ) ||
        ( payload[10] != ( uint8_t ) '|' ) || !decode_sequence( &payload[11], &sequence ) )
    {
        return false;
    }
    return sequence == expected_sequence;
}

uint32_t lora_protocol_next_sequence( uint32_t current )
{
    return ( current >= LORA_PROTOCOL_SEQUENCE_MAX ) ? 1U : ( current + 1U );
}
