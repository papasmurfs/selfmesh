// selfsec.cpp : Defines the sec handle.
//
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfsec.h"
#include "string.h"

static const unsigned char s_s_box[16][16] = {
    {0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76},
    {0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0},
    {0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15},
    {0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75},
    {0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84},
    {0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf},
    {0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8},
    {0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2},
    {0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73},
    {0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb},
    {0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79},
    {0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08},
    {0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a},
    {0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e},
    {0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf},
    {0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16}
};

static const unsigned char s_invs_box[16][16] = {
    {0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb},
    {0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb},
    {0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e},
    {0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25},
    {0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92},
    {0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84},
    {0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06},
    {0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b},
    {0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73},
    {0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e},
    {0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b},
    {0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4},
    {0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f},
    {0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef},
    {0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61},
    {0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d}
};

/*note: for self security the 8*4 matrix traspose is defined
the source matrix
    b00 b01 b02 b03 b04 b05 b06 b07
    b10 b11 b12 b13 b14 b15 b16 b17
    b20 b21 b22 b23 b24 b25 b26 b27
    b30 b31 b32 b33 b34 b35 b36 b37
step1: transpose to
    b00 b04 b10 b14 b20 b24 b30 b34
    b01 b05 b11 b15 b21 b25 b31 b35
    b02 b06 b12 b16 b22 b26 b32 b36
    b03 b07 b13 b17 b23 b27 b33 b37
step2: move the first column to tail
    b04 b10 b14 b20 b24 b30 b34 b00
    b05 b11 b15 b21 b25 b31 b35 b01
    b06 b12 b16 b22 b26 b32 b36 b02
    b07 b13 b17 b23 b27 b33 b37 b03
@parameter input_matrix [in]: must be 4-byte array
@parameter out_matrix [in]:   must be 4-byte array
@return 0 - success
*/
static int s_8s4_matrix_transpose(unsigned char* input_matrix, unsigned char* out_matrix)
{
    out_matrix[0] |= (input_matrix[0]<<4)&0x80;
    out_matrix[1] |= (input_matrix[0]<<5)&0x80;
    out_matrix[2] |= (input_matrix[0]<<6)&0x80;
    out_matrix[3] |= (input_matrix[0]<<7)&0x80;

    out_matrix[0] |= (input_matrix[1]>>1)&0x40;
    out_matrix[1] |= (input_matrix[1])&0x40;
    out_matrix[2] |= (input_matrix[1]<<1)&0x40;
    out_matrix[3] |= (input_matrix[1]<<2)&0x40;
    
    out_matrix[0] |= (input_matrix[1]<<2)&0x20;
    out_matrix[1] |= (input_matrix[1]<<3)&0x20;
    out_matrix[2] |= (input_matrix[1]<<4)&0x20;
    out_matrix[3] |= (input_matrix[1]<<5)&0x20;
    
    out_matrix[0] |= (input_matrix[2]>>3)&0x10;
    out_matrix[1] |= (input_matrix[2]>>2)&0x10;
    out_matrix[2] |= (input_matrix[2]>>1)&0x10;
    out_matrix[3] |= (input_matrix[2])&0x10;

    out_matrix[0] |= (input_matrix[2])&0x08;
    out_matrix[1] |= (input_matrix[2]<<1)&0x08;
    out_matrix[2] |= (input_matrix[2]<<2)&0x08;
    out_matrix[3] |= (input_matrix[2]<<3)&0x08;
    
    out_matrix[0] |= (input_matrix[3]>>5)&0x04;
    out_matrix[1] |= (input_matrix[3]>>4)&0x04;
    out_matrix[2] |= (input_matrix[3]>>3)&0x04;
    out_matrix[3] |= (input_matrix[3]>>2)&0x04;

    out_matrix[0] |= (input_matrix[3]>>2)&0x02;
    out_matrix[1] |= (input_matrix[3]>>1)&0x02;
    out_matrix[2] |= (input_matrix[3])&0x02;
    out_matrix[3] |= (input_matrix[3]<<1)&0x02;

    out_matrix[0] |= (input_matrix[0]>>7)&0x01;
    out_matrix[1] |= (input_matrix[0]>>6)&0x01;
    out_matrix[2] |= (input_matrix[0]>>5)&0x01;
    out_matrix[3] |= (input_matrix[0]>>4)&0x01;

    return 0;
}

/*note: for self security the 8*4 matrix retraspose is defined
the source matrix
    b00 b01 b02 b03 b04 b05 b06 b07
    b10 b11 b12 b13 b14 b15 b16 b17
    b20 b21 b22 b23 b24 b25 b26 b27
    b30 b31 b32 b33 b34 b35 b36 b37
step 1: move the last column to the head
    b07 b00 b01 b02 b03 b04 b05 b06
    b17 b10 b11 b12 b13 b14 b15 b16
    b27 b20 b21 b22 b23 b24 b25 b26
    b37 b30 b31 b32 b33 b34 b35 b36
step 2: retranspose to
    b07 b17 b27 b37 b00 b10 b20 b30
    b01 b11 b21 b31 b02 b12 b22 b32
    b03 b13 b23 b33 b04 b14 b24 b34
    b05 b15 b25 b35 b06 b16 b26 b36
@parameter input_matrix [in]: must be 4-byte array
@parameter out_matrix [in]:   must be 4-byte array
@return 0 - success
*/
static int s_8s4_matrix_retranspose(unsigned char* input_matrix, unsigned char* out_matrix)
{
    
    out_matrix[0] |= (input_matrix[0]<<7)&0x80;
    out_matrix[0] |= (input_matrix[1]<<6)&0x40;
    out_matrix[0] |= (input_matrix[2]<<5)&0x20;
    out_matrix[0] |= (input_matrix[3]<<4)&0x10;
    out_matrix[0] |= (input_matrix[0]>>4)&0x08;
    out_matrix[0] |= (input_matrix[1]>>5)&0x04;
    out_matrix[0] |= (input_matrix[2]>>6)&0x02;
    out_matrix[0] |= (input_matrix[3]>>7)&0x01;
    
    out_matrix[1] |= (input_matrix[0]<<1)&0x80;
    out_matrix[1] |= (input_matrix[1])&0x40;
    out_matrix[1] |= (input_matrix[2]>>1)&0x20;
    out_matrix[1] |= (input_matrix[3]>>2)&0x10;
    out_matrix[1] |= (input_matrix[0]>>2)&0x08;
    out_matrix[1] |= (input_matrix[1]>>3)&0x04;
    out_matrix[1] |= (input_matrix[2]>>4)&0x02;
    out_matrix[1] |= (input_matrix[3]>>5)&0x01;

    out_matrix[2] |= (input_matrix[0]<<3)&0x80;
    out_matrix[2] |= (input_matrix[1]<<2)&0x40;
    out_matrix[2] |= (input_matrix[2]<<1)&0x20;
    out_matrix[2] |= (input_matrix[3])&0x10;
    out_matrix[2] |= (input_matrix[0])&0x08;
    out_matrix[2] |= (input_matrix[1]>>1)&0x04;
    out_matrix[2] |= (input_matrix[2]>>2)&0x02;
    out_matrix[2] |= (input_matrix[3]>>3)&0x01;
    
    out_matrix[3] |= (input_matrix[0]<<5)&0x80;
    out_matrix[3] |= (input_matrix[1]<<4)&0x40;
    out_matrix[3] |= (input_matrix[2]<<3)&0x20;
    out_matrix[3] |= (input_matrix[3]<<2)&0x10;
    out_matrix[3] |= (input_matrix[0]<<2)&0x08;
    out_matrix[3] |= (input_matrix[1]<<1)&0x04;
    out_matrix[3] |= (input_matrix[2])&0x02;
    out_matrix[3] |= (input_matrix[3]>>1)&0x01;

    return 0;
}

/*note: for self security the subkey array creation is defined
    mainkey--s_8s4_matrix_transpose-->subkey0
    subkey0--s_8s4_matrix_transpose-->subkey1
    subkey1--s_8s4_matrix_transpose-->subkey2
    subkey2--s_8s4_matrix_transpose-->subkey3
    subkey3--s_8s4_matrix_transpose-->subkey4
    subkey4--s_8s4_matrix_transpose-->subkey5
    subkey5--s_8s4_matrix_transpose-->subkey6
    subkey6--s_8s4_matrix_transpose-->subkey7
@parameter key    [in]:    the mainkey, must be 4-byte array
@parameter subkey [out]:   must be 8*4-byte array
@return 0 - success
*/
static int s_creat_subkey(unsigned char* key, unsigned char* subkey)
{
    s_8s4_matrix_transpose(key, subkey);
    for(int i=0; i<7; i++)
    {
        s_8s4_matrix_transpose(subkey+i*4, subkey+((i+1)*4));
    }
    return 0;
}

/*note: for self security the data block encrypt is defined
    rawdata--rawdata^subkey0--s_box-->med_result0
    med_result0--med_result0^subkey1--s_box-->med_result1
    med_result1--med_result1^subkey2--s_box-->med_result2
    med_result2--med_result2^subkey3--s_box-->med_result3
    med_result3--med_result3^subkey4--s_box-->med_result4
    med_result4--med_result4^subkey5--s_box-->med_result5
    med_result5--med_result5^subkey6--s_box-->med_result6
    med_result6--med_result6^subkey7--s_box-->result
@parameter raw_data[in]:   the raw data, must be 4-byte array
@parameter subkey [in]:    must be 8*4-byte array
@parameter enc_data[out]:  the encrypt data, must be 4-byte array
@return 0 - success
*/
static int s_encrypt_block(unsigned char * raw_data, unsigned char* subkeyarray, unsigned char * enc_data)
{
    unsigned char temp_result[4] = {0x00};
    int turn = 0;
    enc_data[0] = raw_data[0]^subkeyarray[0];
    enc_data[1] = raw_data[1]^subkeyarray[1];
    enc_data[2] = raw_data[2]^subkeyarray[2];
    enc_data[3] = raw_data[3]^subkeyarray[3];
    temp_result[0] = (enc_data[0]<<4)|(enc_data[1]>>4);
    temp_result[1] = (enc_data[1]<<4)|(enc_data[2]>>4);
    temp_result[2] = (enc_data[2]<<4)|(enc_data[3]>>4);
    temp_result[3] = (enc_data[3]<<4)|(enc_data[0]>>4);
    enc_data[0] = s_s_box[(temp_result[0]>>4)&0x0F][(temp_result[0])&0x0F];
    enc_data[1] = s_s_box[(temp_result[1]>>4)&0x0F][(temp_result[1])&0x0F];
    enc_data[2] = s_s_box[(temp_result[2]>>4)&0x0F][(temp_result[2])&0x0F];
    enc_data[3] = s_s_box[(temp_result[3]>>4)&0x0F][(temp_result[3])&0x0F];
    memcpy(temp_result, enc_data, 4);

    for(turn=1; turn<8; turn++)
    {
        enc_data[0] = temp_result[0]^subkeyarray[turn*4+0];
        enc_data[1] = temp_result[1]^subkeyarray[turn*4+1];
        enc_data[2] = temp_result[2]^subkeyarray[turn*4+2];
        enc_data[3] = temp_result[3]^subkeyarray[turn*4+3];
        temp_result[0] = (enc_data[0]<<4)|(enc_data[1]>>4);
        temp_result[1] = (enc_data[1]<<4)|(enc_data[2]>>4);
        temp_result[2] = (enc_data[2]<<4)|(enc_data[3]>>4);
        temp_result[3] = (enc_data[3]<<4)|(enc_data[0]>>4);
        enc_data[0] = s_s_box[(temp_result[0]>>4)&0x0F][(temp_result[0])&0x0F];
        enc_data[1] = s_s_box[(temp_result[1]>>4)&0x0F][(temp_result[1])&0x0F];
        enc_data[2] = s_s_box[(temp_result[2]>>4)&0x0F][(temp_result[2])&0x0F];
        enc_data[3] = s_s_box[(temp_result[3]>>4)&0x0F][(temp_result[3])&0x0F];
        memcpy(temp_result, enc_data, 4);
    }

    memcpy(enc_data, temp_result, 4);
    return 0;
}

/*note: for self security the data block decrypt is defined
    encdata--invs_box--^subkey7-->med_result0
    med_result0--invs_box--^subkey6-->med_result1
    med_result1--invs_box--^subkey5-->med_result2
    med_result2--invs_box--^subkey4-->med_result3
    med_result3--invs_box--^subkey3-->med_result4
    med_result4--invs_box--^subkey2-->med_result5
    med_result5--invs_box--^subkey1-->med_result6
    med_result6--invs_box--^subkey0-->result
@parameter enc_data[in]:   the encrypt data, must be 4-byte array
@parameter subkey [in]:    must be 8*4-byte array
@parameter raw_data[out]:  the raw data, must be 4-byte array
@return 0 - success
*/
static int s_decrypt_block(unsigned char * enc_data, unsigned char* subkeyarray, unsigned char * raw_data)
{
    unsigned char temp_result[4] = {0x00};
    int turn = 0;
    raw_data[0] = s_invs_box[(enc_data[0]>>4)&0x0F][(enc_data[0])&0x0F];
    raw_data[1] = s_invs_box[(enc_data[1]>>4)&0x0F][(enc_data[1])&0x0F];
    raw_data[2] = s_invs_box[(enc_data[2]>>4)&0x0F][(enc_data[2])&0x0F];
    raw_data[3] = s_invs_box[(enc_data[3]>>4)&0x0F][(enc_data[3])&0x0F];

    temp_result[0] = (raw_data[3]<<4)|(raw_data[0]>>4);
    temp_result[1] = (raw_data[0]<<4)|(raw_data[1]>>4);
    temp_result[2] = (raw_data[1]<<4)|(raw_data[2]>>4);
    temp_result[3] = (raw_data[2]<<4)|(raw_data[3]>>4);

    raw_data[0] = temp_result[0]^subkeyarray[28];
    raw_data[1] = temp_result[1]^subkeyarray[29];
    raw_data[2] = temp_result[2]^subkeyarray[30];
    raw_data[3] = temp_result[3]^subkeyarray[31];
    memcpy(temp_result, raw_data, 4);

    for(turn=6; turn>=0; turn--)
    {
        raw_data[0] = s_invs_box[(temp_result[0]>>4)&0x0F][(temp_result[0])&0x0F];
        raw_data[1] = s_invs_box[(temp_result[1]>>4)&0x0F][(temp_result[1])&0x0F];
        raw_data[2] = s_invs_box[(temp_result[2]>>4)&0x0F][(temp_result[2])&0x0F];
        raw_data[3] = s_invs_box[(temp_result[3]>>4)&0x0F][(temp_result[3])&0x0F];
        temp_result[0] = (raw_data[3]<<4)|(raw_data[0]>>4);
        temp_result[1] = (raw_data[0]<<4)|(raw_data[1]>>4);
        temp_result[2] = (raw_data[1]<<4)|(raw_data[2]>>4);
        temp_result[3] = (raw_data[2]<<4)|(raw_data[3]>>4);
        raw_data[0] = temp_result[0]^subkeyarray[turn*4+0];
        raw_data[1] = temp_result[1]^subkeyarray[turn*4+1];
        raw_data[2] = temp_result[2]^subkeyarray[turn*4+2];
        raw_data[3] = temp_result[3]^subkeyarray[turn*4+3];
        memcpy(temp_result, raw_data, 4);
    }

    return 0;
}

int selfsec_init(void)
{
    return 0;
}

/*note: for self security the data encrypt is defined
    rawdatablock0--^IK(init IK is all 0)--encrypt block-->med_result0
    rawdatablock1--^med_result0--encrypt block-->med_result1
    rawdatablock2--^med_result1--encrypt block-->med_result2
    ...
    rawdatablock(n+1)--^med_result(n)--encrypt block-->result
@parameter key[in]:              the main key, must be 4-byte array
@parameter raw_data[in]:         the raw data, must be n*4-byte array
@parameter raw_len [in]:         the length of raw data, must be n*4
@parameter enc_data[out]:        the memory to save encrypt data, the length should not be less then raw_len
@parameter enc_len_ptr[in/out]:  the length of out memory/encrypt data, for in the length of output memory
                                                                      for out the length of encrypt data
@return 0 - success
*/
int selfsec_encrypt(unsigned char* key, const unsigned char* raw_data, int raw_len, unsigned char* enc_data, int* enc_len_ptr)
{
    unsigned char subkey[8][4] = {0x00};
    unsigned char temp_ik[4] = {0x00};
    unsigned char temp_result[4] = {0x00};
    if(raw_len %4 != 0 || ((*enc_len_ptr) < raw_len))
    {
        return -1;
    }
    s_creat_subkey(key, &subkey[0][0]);
    for(int i=0;i<raw_len;i+=4)
    {
        temp_result[0] = raw_data[i+0]^temp_ik[0];
        temp_result[1] = raw_data[i+1]^temp_ik[1];
        temp_result[2] = raw_data[i+2]^temp_ik[2];
        temp_result[3] = raw_data[i+3]^temp_ik[3];
        s_encrypt_block(temp_result, (unsigned char*)subkey, enc_data+i);
        temp_ik[0] = enc_data[i+0];
        temp_ik[1] = enc_data[i+1];
        temp_ik[2] = enc_data[i+2];
        temp_ik[3] = enc_data[i+3];
    }
    *(enc_len_ptr) = raw_len;
    return 0;
}

/*note: for self security the data decrypt is defined
    encdatablock0--^IK(init IK is all 0)-->med_result0
    encdatablock1--decrypt block--^med_result0-->med_result1
    encdatablock2--decrypt block--^med_result1-->med_result2
    ...
    encdatablock(n+1)--decrypt block--^med_result(n)-->result
@parameter key[in]:              the main key, must be 4-byte array
@parameter enc_data[in]:         the encrypt data, must be n*4-byte array
@parameter enc_len [in]:         the length of encrypt data, must be n*4
@parameter raw_data[out]:        the memory to save decrypt data, the length should not be less then enc_len
@parameter raw_len_ptr[in/out]:  the length of out memory/decrypt data, for in the length of output memory
                                                                      for out the length of decrypt data
@return 0 - success
*/
int selfsec_decrypt(unsigned char* key, const unsigned char* enc_data, int enc_len, unsigned char* raw_data, int* raw_len_ptr)
{
    unsigned char subkey[8][4] = {0x00};
    unsigned char temp_ik[4] = {0x00};
    unsigned char temp_result[4] = {0x00};
    unsigned char* temp_enc_ptr = (unsigned char*)enc_data;
    if(enc_len %4 != 0 || ((*raw_len_ptr) < enc_len))
    {
        return -1;
    }
    s_creat_subkey(key, &subkey[0][0]);
    for(int i=0;i<enc_len;i+=4)
    {
        s_decrypt_block(temp_enc_ptr+i, (unsigned char*)subkey, temp_result);
        raw_data[i+0] = temp_result[0]^temp_ik[0];
        raw_data[i+1] = temp_result[1]^temp_ik[1];
        raw_data[i+2] = temp_result[2]^temp_ik[2];
        raw_data[i+3] = temp_result[3]^temp_ik[3];
        temp_ik[0] = temp_enc_ptr[i+0];
        temp_ik[1] = temp_enc_ptr[i+1];
        temp_ik[2] = temp_enc_ptr[i+2];
        temp_ik[3] = temp_enc_ptr[i+3];
    }
    s_decrypt_block(temp_enc_ptr, (unsigned char*)subkey, raw_data);
    (*raw_len_ptr) = enc_len;
    return 0;
}

