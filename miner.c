#include <gb/gb.h>
#include <stdio.h>
#include <string.h>
#include <gb/metasprites.h>
#include "keccak256.h"

#include <gb/console.h>
#include "sha2.h"

#include "sprite.h"

uint32_t logo_offset = 40;

void print_header() {
        gotoxy(0, 7);
        cls();
        puts("Game Boy");
        puts("Bitcoin Miner");
        puts("by stacksmashing\n\n");
}

void hex_print(unsigned char *data, int len) {
    for(int i=0; i < len; i++) {
        printf("%x ", data[i] & 0xFF);
    }
    printf("\n");
}

int receive_block_header_fake(char *header) {
    memset(header, 0x11, 76);
    return 0;
    
}

void sb(unsigned char c) {
    _io_out = c;
    receive_byte();
    while(_io_status == IO_RECEIVING) {
            // sending...
        }
        // printf("don\n");
        if(_io_status != IO_IDLE) {
            printf("failStatus\n#%d\n", _io_status);
            return;
        }
        return;
}

int receive_bytes(unsigned char *header, size_t len) {
    for(int i = 0; i < len; i++) {
        receive_byte();
        while(_io_status == IO_RECEIVING) {
            // Receiving...
        }
        // printf("don\n");
        if(_io_status != IO_IDLE) {
            printf("Status #%d\n", _io_status);
            return 1;
        }
        header[i] = _io_in;
    }
    // printf("Done.");
    // hex_print(header, 10);
    return 0;
}

const uint8_t CMD_BLOCK_HEADER = 0x41;
const uint8_t CMD_TARGET = 0x42;
const uint8_t CMD_NEW_BLOCK = 0x43;
const uint8_t CMD_STATUS = 0x44;
const uint8_t RESP_WAIT_CMD = 0x61;
const uint8_t RESP_WAIT_DATA = 0x62;
const uint8_t RESP_SUCCESS = 0x63;

int receive_block_header(unsigned char *header) {
    for(int i = 0; i < 76; i++) {
        _io_out = RESP_WAIT_DATA;
        receive_byte();
        while(_io_status == IO_RECEIVING) {
            // Receiving...
        }
        // printf("don\n");
        if(_io_status != IO_IDLE) {
            printf("Status #%d\n", _io_status);
            return 1;
        }
        header[i] = _io_in;
    }
    _io_out = 0x41;
    // printf("Done.");
    // hex_print(header, 10);
    return 0;
}


void wait_byte(const uint8_t b) {    
    while(1) {

        // _io_out = 0x88;
        receive_byte();
        while(_io_status == IO_RECEIVING) {
            // Receiving...
        }
        // _io_out = 0x41;
        if(_io_status != IO_IDLE) {
            printf("FAILED TO RECEIVE\n#%d\n", _io_status);
            return;
        }
        if(_io_in == b) {
            _io_out = 0x0;
            return;
        } else {
            printf("Invalid command.\n");
        }
    }
}

void hexdump(unsigned char *f, size_t len) {
    for(int i=0; i < len; i++) {
        unsigned char c = f[i];
        unsigned char c1 = c & 0x0F;
        unsigned char c2 = (c >> 4) & 0x0F;
        if(c2 < 10) {
            printf("%c", '0' + c2);
        } else {
            printf("%c", 'A' + (c2 - 10));
        }
        if(c1 < 10) {
            printf("%c", '0' + c1);
        } else {
            printf("%c", 'A' + (c1 - 10));
        }

    }
    puts("");
}

void receive_block_data(unsigned char *block_header, unsigned char *target) {
    _io_out = RESP_WAIT_CMD;
    wait_byte(CMD_BLOCK_HEADER);
    receive_block_header(block_header);
    _io_out = RESP_WAIT_CMD;
    wait_byte(CMD_TARGET);
    receive_bytes(target, 32);
}

int mine_nonce(unsigned char *block_header, unsigned char *target, uint32_t *nonce_out) {
    uint32_t nonce = 0;
    while(1) {
        receive_byte();

        // Append nonce
        memcpy(&block_header[76], &nonce, 4);
        print_header();
        // gotoxy(0, 7);
        // cls();
        // puts("Game Boy");
        // puts("Bitcoin Miner");
        // puts("by stacksmashing\n\n");

        printf("Nonce: %5d\n", nonce);

        // Increase nonce
        nonce++;
        
        printf("Hashing...\n");
        uint8_t hash_rev[32];
        uint8_t hash2[32];
        uint8_t hash[32];
        calc_sha_256(hash2, block_header, 80);
        calc_sha_256(hash_rev, hash2, 32);
        for(int i=0; i < 32; i++) {
            hash[31-i] = hash_rev[i];
        }
        // printf("");

        move_metasprite(sprite_metasprites[0], 0, 0, logo_offset, 40);
        logo_offset += 4;
        logo_offset %= 198;
        

        receive_byte();
        // Check hash...
        for(int i=0; i < 32; i++) {
            if(hash[i] < target[i]) {
                printf("Success!\n");
                // _io_out = RESP_SUCCESS;
                // hexdump(block_header, 80);
                *nonce_out = nonce - 1;
                return 0;
            } else if(hash[i] > target[i]) {
                // printf("Fail!\n");
                // return 1;
                break;
            }
        }
    
        
        // Check if we need to receive a new block.
        if(_io_in == CMD_NEW_BLOCK) {
            printf("New block data.\n");
            return 1;
        }
    }
}

void miner(void) {
    unsigned char block_header[80];
    unsigned char target[32];

    while(1) {
        print_header();
        printf("Waiting for data...\n");
        receive_block_data(block_header, target);
        
        _io_out = 0x0;
        uint32_t nonce = 0;
        int mine_result = mine_nonce(block_header, target, &nonce);

        // Indicate success to host
        if(mine_result == 0) {
            _io_out = RESP_SUCCESS;
            printf("Sending status...\n");
            wait_byte(CMD_STATUS);
            // send nonce over.
            char data[4];
            memcpy(data, &nonce, 4);
            for(int i=0; i < 4; i++) {
                sb(data[i]);
            }
        }
    }
}

void main(void)
{
    UBYTE i, n = 0;
    unsigned char *s;

    // load tile data into VRAM
    set_sprite_data(0, sizeof(sprite_data) >> 4, sprite_data);

    // set sprite tile
    set_sprite_tile(0, 0);
    SHOW_SPRITES;
    #if sprite_TILE_H == 16
		SPRITES_8x16;
	#else
		SPRITES_8x8;
	#endif
    move_metasprite(sprite_metasprites[0], 0, 0, 40, 40);

        puts("Game Boy");
        puts("Bitcoin Miner");
        puts("by stacksmashing\n\n");
    CRITICAL {
        add_SIO(nowait_int_handler);
        set_interrupts(SIO_IFLAG | VBL_IFLAG);
    }
    miner();
}
