#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define LORA_PROTOCOL_NODE_ID_MAX_LEN ( 6U )
#define LORA_PROTOCOL_PACKET_MAX_LEN  ( 64U )

typedef struct
{
    char     node_id[LORA_PROTOCOL_NODE_ID_MAX_LEN + 1U];
    uint32_t sequence;
} lora_protocol_message_t;

bool lora_protocol_format_data( const char* node_id, uint32_t sequence, uint8_t* output, uint8_t capacity,
                                uint8_t* output_length );
bool lora_protocol_parse_data( const uint8_t* payload, uint8_t length, lora_protocol_message_t* message );
bool lora_protocol_format_ack( const char* node_id, uint32_t sequence, uint8_t* output, uint8_t capacity,
                               uint8_t* output_length );
bool lora_protocol_ack_matches( const uint8_t* payload, uint8_t length, const char* expected_node_id,
                                uint32_t expected_sequence );
uint32_t lora_protocol_next_sequence( uint32_t current );

#endif
