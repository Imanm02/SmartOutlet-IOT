/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2019
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_algorithm.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to the algorithm.
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#ifndef __QL_ALGORITHM_H__
#define __QL_ALGORITHM_H__

typedef unsigned int    mp_digit;  /* long could be 64 now, changed TAO */
/* the infamous mp_int structure */
typedef struct  {
    int used, alloc, sign;
    mp_digit *dp;
} mp_int;
struct Ql_Ssl_RsaKey {
	mp_int n, e, d, p, q, dP, dQ, u;
	void* heap; 							  /* for user memory overrides */
	byte* data; 							  /* temp buffer for async RSA */
	int   type; 							  /* public or private */
	int   state;
	word32 dataLen;
	byte   dataIsAlloc;
};

typedef struct Ql_Ssl_RsaKey Ql_RsaKey;

/* Signature type, by OID sum */
enum SigType {
    MD5_RSA      = 648,
    SHA_RSA      = 649,
    SHA256_RSA   = 655,

};
s32 Ql_SSL_KeyPemToDer(const unsigned char* pem, long pemSz,unsigned char * der,int derSz);
s32 Ql_SSL_RsaPrivateDecrypt(const byte* in, word32 inLen, byte* out, word32 outLen, Ql_RsaKey *ssl_RasKey);
s32 Ql_SSL_RsaPrivateKeyDecode(const byte* input, word32* inOutIdx,Ql_RsaKey *ssl_RasKey, word32 inSz);
s32 Ql_SSL_MakeSignature(const byte* buffer, int sz, byte* sig, int sigSz,Ql_RsaKey *ssl_RasKey,int sigAlgoType);

s32 Ql_SSL_Base64_Encode(const byte * in,word32 inLen,byte * out,word32 * outLen);
s32 Ql_SSL_Base64_Decode(const byte * in,word32 inLen,byte * out,word32 * outLen);

#endif 
