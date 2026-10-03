/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef SELFSEC_H
#define SELFSEC_H
extern int selfsec_init(void);


/*note: for self security the data encrypt is defined
    rawdatablock0--^IK(init IK is all 0)--encrypt block-->med_result0
    rawdatablock1--^med_result0--encrypt block-->med_result1
    rawdatablock2--^med_result1--encrypt block-->med_result2
    ...
    rawdatablock(n+1)--^med_result(n)--encrypt block-->result
@parameter key[in]:              the main key, must be 8-byte array
@parameter raw_data[in]:         the raw data, must be n*4-byte array
@parameter raw_len [in]:         the length of raw data, must be n*4
@parameter enc_data[out]:        the memory to save encrypt data, the length should not be less then raw_len
@parameter enc_len_ptr[in/out]:  the length of out memory/encrypt data, for in the length of output memory
                                                                      for out the length of encrypt data
@return 0 - success
*/
extern int selfsec_encrypt(unsigned char* key, const unsigned char* raw_data, int raw_len, unsigned char* enc_data, int* enc_len_ptr);


/*note: for self security the data decrypt is defined
    encdatablock0--^IK(init IK is all 0)-->med_result0
    encdatablock1--decrypt block--^med_result0-->med_result1
    encdatablock2--decrypt block--^med_result1-->med_result2
    ...
    encdatablock(n+1)--decrypt block--^med_result(n)-->result
@parameter key[in]:              the main key, must be 8-byte array
@parameter enc_data[in]:         the encrypt data, must be n*4-byte array
@parameter enc_len [in]:         the length of encrypt data, must be n*4
@parameter raw_data[out]:        the memory to save decrypt data, the length should not be less then enc_len
@parameter raw_len_ptr[in/out]:  the length of out memory/decrypt data, for in the length of output memory
                                                                      for out the length of decrypt data
@return 0 - success
*/
extern int selfsec_decrypt(unsigned char* key, const unsigned char* enc_data, int enc_len, unsigned char* raw_data, int* raw_len_ptr);
#endif //SELFSEC_H
