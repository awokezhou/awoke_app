
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/sem.h>

#include "sec_aes.h"
#include "sec_padding.h"


/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4
// The number of 32 bit words in a key.
#define Nk 4
// Key length in bytes [128 bit]
// The number of rounds in AES Cipher.
#define Nr 10

// jcallan@github points out that declaring Multiply as a function
// reduces code size considerably with the Keil ARM compiler.
// See this link for more information: https://github.com/kokke/tiny-AES128-C/pull/3


static state_t* state;
//static int aes_init_done = 0;

static uint8_t* Key;

static uint8_t* RoundKey;
/*
static uint8_t sbox[256];
static uint8_t rsbox[256];
*/
static uint8_t sbox[256]=   {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };


static uint8_t rsbox[256]
=
{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };


static uint8_t getSBoxValue(uint8_t num)
{
  return sbox[num];
}

static uint8_t getSBoxInvert(uint8_t num)
{
  return rsbox[num];
}


static uint8_t xtime(uint8_t x)
{
  return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}


  static  uint8_t Rcon[255]
  = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
    0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
    0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,
    0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
    0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
    0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,
    0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,
    0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
    0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
    0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
    0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,
    0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
    0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
    0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,
    0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,
    0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb  };

/*
  static void aes_gen_tables( void )
  {
      int i, x, y;
      int pow[256];
      int log[256];


       // compute pow and log tables over GF(2^8)

      i = 0;
      x = 1;
      y=1;
      pow[i] = x;
      log[x] = i;
      Rcon[0] = 0x8d;
      x = ( x ^ xtime( x ) ) & 0xFF;
      i++;
      for( ; i < 256; i++ )
      {
          pow[i] = x;
          log[x] = i;
          Rcon[i] = y;
          x = ( x ^ xtime( x ) ) & 0xFF;
          y = xtime( y ) & 0xFF;
      }


       // calculate the round constants



       // generate the forward and reverse S-boxes

      sbox[0x00] = 0x63;
      rsbox[0x63] = 0x00;

      for( i = 1; i < 256; i++ )
      {
          x = pow[255 - log[i]];

          y  = x; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
          x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
          x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
          x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
          x ^= y ^ 0x63;

          sbox[i] = (unsigned char) x;
          rsbox[x] = (unsigned char) i;
      }

  }

*/

  static void AddRoundKey(uint8_t round)
  {
    uint8_t i,j;
    for(i=0;i<4;++i)
    {
      for(j = 0; j < 4; ++j)
      {
        (*state)[i][j] ^= RoundKey[round * Nb * 4 + i * Nb + j];
      }
    }
  }

// This function produces Nb(Nr+1) round keys. The round keys are used in each round to decrypt the states.
static void KeyExpansion(void)
{
  uint32_t i, j, k;
  uint8_t tempa[4]; // Used for the column/row operations

  // The first round key is the key itself.
  for(i = 0; i < Nk; ++i)
  {
    RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
    RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
    RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
    RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
  }

  // All other round keys are found from the previous round keys.
  for(; (i < (Nb * (Nr + 1))); ++i)
  {
    for(j = 0; j < 4; ++j)
    {
      tempa[j]=RoundKey[(i-1) * 4 + j];
    }
    if (i % Nk == 0)
    {
      // This function rotates the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

      // Function RotWord()
      {
        k = tempa[0];
        tempa[0] = tempa[1];
        tempa[1] = tempa[2];
        tempa[2] = tempa[3];
        tempa[3] = k;
      }

      // SubWord() is a function that takes a four-byte input word and
      // applies the S-box to each of the four bytes to produce an output word.

      // Function Subword()
      {
        tempa[0] = getSBoxValue(tempa[0]);
        tempa[1] = getSBoxValue(tempa[1]);
        tempa[2] = getSBoxValue(tempa[2]);
        tempa[3] = getSBoxValue(tempa[3]);
      }

      tempa[0] =  tempa[0] ^ Rcon[i/Nk];
    }
    RoundKey[i * 4 + 0] = RoundKey[(i - Nk) * 4 + 0] ^ tempa[0];
    RoundKey[i * 4 + 1] = RoundKey[(i - Nk) * 4 + 1] ^ tempa[1];
    RoundKey[i * 4 + 2] = RoundKey[(i - Nk) * 4 + 2] ^ tempa[2];
    RoundKey[i * 4 + 3] = RoundKey[(i - Nk) * 4 + 3] ^ tempa[3];
  }
}

// This function adds the round key to state.
// The round key is added to the state by an XOR function.

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.

static void SubBytes(void)
{
  uint8_t i, j;
  for(i = 0; i < 4; ++i)
  {
    for(j = 0; j < 4; ++j)
    {
      (*state)[j][i] = getSBoxValue((*state)[j][i]);
    }
  }
}

static void InvSubBytes(void)
{
  uint8_t i,j;
  for(i=0;i<4;++i)
  {
    for(j=0;j<4;++j)
    {
      (*state)[j][i] = getSBoxInvert((*state)[j][i]);
    }
  }
}




// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void ShiftRows(void)
{
  uint8_t temp;

  // Rotate first row 1 columns to left
  temp           = (*state)[0][1];
  (*state)[0][1] = (*state)[1][1];
  (*state)[1][1] = (*state)[2][1];
  (*state)[2][1] = (*state)[3][1];
  (*state)[3][1] = temp;

  // Rotate second row 2 columns to left
  temp           = (*state)[0][2];
  (*state)[0][2] = (*state)[2][2];
  (*state)[2][2] = temp;

  temp       = (*state)[1][2];
  (*state)[1][2] = (*state)[3][2];
  (*state)[3][2] = temp;

  // Rotate third row 3 columns to left
  temp       = (*state)[0][3];
  (*state)[0][3] = (*state)[3][3];
  (*state)[3][3] = (*state)[2][3];
  (*state)[2][3] = (*state)[1][3];
  (*state)[1][3] = temp;
}


static void MixColumns( int encrypt){
	 uint8_t c0 = 0,  t;
	 while(c0!=4){
		 uint8_t c1 = 0, a0 = 0, a1 = 0, a2 = 0, a3 = 0;
		 while(c1!=4){
			 t= (*state)[c0][c1];
			 //t = x[c0*4+c1];
			 a0 = a0^t;
			 a1 = a1^t;
			 a2 = a2^t;
			 t = xtime(t);
			 if (encrypt != MBEDTLS_AES_ENCRYPT){
				 a2 = a2^t;
				 a3 = a3^t;
				 t = xtime(t);
				 a1 = a1^t;
				 a3 = a3^t;
				 t = xtime(t);
				 a0 = a0^t;
				 a1 = a1^t;
			 }
			 a2 = a2^t;
			 a3 = a3^t;
			 t = a0;
			 a0 = a1;
			 a1 = a2;
			 a2 = a3;
			 a3 = t;
			 c1++;

		 }
		 (*state)[c0][0] = a3;
		 (*state)[c0][1] = a0;
		 (*state)[c0][2] = a1;
		 (*state)[c0][3] = a2;
		 c0++;
	 }
}




static void InvShiftRows(void)
{
  uint8_t temp;

  // Rotate first row 1 columns to right
  temp=(*state)[3][1];
  (*state)[3][1]=(*state)[2][1];
  (*state)[2][1]=(*state)[1][1];
  (*state)[1][1]=(*state)[0][1];
  (*state)[0][1]=temp;

  // Rotate second row 2 columns to right
  temp=(*state)[0][2];
  (*state)[0][2]=(*state)[2][2];
  (*state)[2][2]=temp;

  temp=(*state)[1][2];
  (*state)[1][2]=(*state)[3][2];
  (*state)[3][2]=temp;

  // Rotate third row 3 columns to right
  temp=(*state)[0][3];
  (*state)[0][3]=(*state)[1][3];
  (*state)[1][3]=(*state)[2][3];
  (*state)[2][3]=(*state)[3][3];
  (*state)[3][3]=temp;
}
// Cipher is the main function that encrypts the PlainText.
static void Cipher(void)
{
  uint8_t round = 0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(0);

  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  for(round = 1; round < Nr; ++round)
  {
    SubBytes();
    ShiftRows();
    MixColumns(MBEDTLS_AES_ENCRYPT);
    AddRoundKey(round);
  }

  // The last round is given below.
  // The MixColumns function is not here in the last round.
  SubBytes();
  ShiftRows();
  AddRoundKey(Nr);
}

static void InvCipher(void)
{
  uint8_t round=0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(Nr);

  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  for(round=Nr-1;round>0;round--)
  {
    InvShiftRows();
    InvSubBytes();
    AddRoundKey(round);
    MixColumns(MBEDTLS_AES_DECRYPT);
  }

  // The last round is given below.
  // The MixColumns function is not here in the last round.
  InvShiftRows();
  InvSubBytes();
  AddRoundKey(0);
}

static void BlockCopy(uint8_t* output, const uint8_t* input)
{
  uint8_t i;
  for (i=0;i<KEYLEN;++i)
  {
    output[i] = input[i];
  }
}


#if !defined(MBEDTLS_AES_SETKEY_ENC_ALT)
int mbedtls_aes_setkey_enc( mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits )
{
    if( keybits !=128 )
        return( MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH );
 //   hexdump("key",key,16);
/*
    if( aes_init_done == 0 )
    {
        aes_gen_tables();
        aes_init_done = 1;
    }
    */
  //  BlockCopy(ctx->key, key);
    RoundKey = ctx->RoundKey;
    Key = (unsigned char *) key;
    KeyExpansion();
    return( 0 );
}
#endif /* !MBEDTLS_AES_SETKEY_ENC_ALT */

/*
 * AES key schedule (decryption)
 */
#if !defined(MBEDTLS_AES_SETKEY_DEC_ALT)
int mbedtls_aes_setkey_dec( mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits )
{
	return mbedtls_aes_setkey_enc(ctx,key,keybits);
}
#endif /* !MBEDTLS_AES_SETKEY_DEC_ALT */

void mbedtls_aes_init( mbedtls_aes_context *ctx )
{
  memset( ctx, 0, sizeof( mbedtls_aes_context ) );
//  memset( RoundKey, 0, sizeof( RoundKey ) );
}

void mbedtls_aes_free( mbedtls_aes_context *ctx )
{
    if( ctx == NULL )
        return;

    memset( ctx, 0, sizeof( mbedtls_aes_context ) );
}
/*****************************************************************************/
/* Public functions:                                                         */
/*****************************************************************************/

void mbedtls_aes_encrypt( mbedtls_aes_context *ctx,  int mode,  const unsigned char input[16],
        unsigned char output[16])
{

RoundKey=ctx->RoundKey;

  // Copy input to output, and work in-memory on output
  BlockCopy(output, input);
  state = (state_t*)output;


  // The next function call encrypts the PlainText with the Key using AES algorithm.
  if( mode == MBEDTLS_AES_ENCRYPT )
  Cipher();
  else
  InvCipher();

}

int mbedtls_aes_crypt_ecb( mbedtls_aes_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] )
{

        mbedtls_aes_encrypt( ctx, mode, input, output );

    return( 0 );
}



#define MBEDTLS_CIPHER_MODE_CBC
#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * AES-CBC buffer encryption/decryption
 */
int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output )
{
	int i;
    unsigned char temp[16];

    if( length % 16 )
        return( MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH );

    if( mode == MBEDTLS_AES_DECRYPT )
    {
        while( length > 0 )
        {
            memcpy( temp, input, 16 );
            mbedtls_aes_crypt_ecb( ctx, mode, input, output );

            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 16 );

            input  += 16;
            output += 16;
            length -= 16;
        }
    }
    else
    {
        while( length > 0 )
        {
            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            mbedtls_aes_crypt_ecb( ctx, mode, output, output );
            memcpy( iv, output, 16 );

            input  += 16;
            output += 16;
            length -= 16;
        }
    }

    return( 0 );
}

#endif /* MBEDTLS_CIPHER_MODE_CBC */

uint8_t *awoke_string_padding(uint8_t *s, char c, int align, int *rlen) 
{
	int i;
	uint8_t *p;
	uint8_t *new_s;
	size_t len;
	size_t align_len;

	if ((!s) || (align <= 0))
        return NULL;
	
	len = strlen(s);
	if (len==align)
		align_len = align;
	else
		align_len = ((len/align)+1)*align;
	new_s = mem_alloc_z(align_len);
	p = new_s;
	uint8_t *end = &new_s[align_len-1];
	buf_push_stri_safe(*s, p, end, len);

	for (i=0; i<align_len-len; i++) {
		buf_push_byte_safe(c, p, end);
	}
	*rlen = align_len;
	return new_s;
}

uint8_t *awoke_byte_padding(uint8_t *buf, int buf_len, uint8_t c, int align, int *rlen)
{
	int i;
	uint8_t *p;
	uint8_t *new_buf;
	size_t align_len;	

	if ((!buf) || (align <= 0))
        return NULL;
	if (buf_len==align)
		align_len = align;
	else
		align_len = ((buf_len/align)+1)*align;
	new_buf = mem_alloc_z(align_len);
	p = new_buf;
	memcpy(p, buf, buf_len);
	p += buf_len;
	for (i=0; i<align_len-buf_len; i++) {
		buf_push_byte(c, p);
	}
	*rlen = align_len;
	return new_buf;
}

err_type aes_cbc_enc_string(uint8_t *key, uint8_t *inv, int key_len, 
								   uint8_t *str, uint8_t **out)
{
	int len;
	int safe_key_len;
	int safe_inv_len;
	int safe_str_len;
	uint8_t *safe_key;
	uint8_t *safe_inv;
	uint8_t *safe_str;
	uint8_t *safe_enc;
	mbedtls_aes_context ctx;
	
	if (!key || !inv || !str || key_len<=0)
		return et_aes_enc;

	len = strlen(key);
	if (len > key_len)
		return et_aes_enc;
	len = strlen(inv);
	if (len > key_len)
		return et_aes_enc;

	log_debug("aes_cbc_enc_string in");
	
	safe_key = awoke_string_padding(key, ' ', key_len, &safe_key_len);
	if (!safe_key)
		goto err;

	safe_inv = awoke_string_padding(inv, ' ', key_len, &safe_inv_len);
	if (!safe_inv)
		goto err;

	safe_str = awoke_string_padding(str, ' ', key_len, &safe_str_len);
	if (!safe_str)
		goto err;

	mbedtls_aes_init(&ctx);
	safe_enc = mem_alloc_z(safe_str_len);

	printf("key:%s, len:%d\n", key, strlen(key));
	buf_dump(key, strlen(key));
	printf("\n");
	printf("safe_key:%s, len:%d\n", safe_key, strlen(safe_key));
	buf_dump(safe_key, safe_key_len);
	printf("\n");

	printf("inv:%s, len:%d\n", inv, strlen(inv));
	buf_dump(inv, strlen(inv));
	printf("\n");
	printf("safe_inv:%s, len:%d\n", safe_inv, strlen(safe_inv));
	buf_dump(safe_inv, safe_inv_len);
	printf("\n");

	printf("plain:%s, len:%d\n", str, strlen(str));
	buf_dump(str, strlen(str));
	printf("\n");
	printf("safe plain:%s, len:%d\n", safe_str, strlen(safe_str));
	buf_dump(safe_str, safe_str_len);
	printf("\n");

	mbedtls_aes_setkey_enc(&ctx, safe_key, 128);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, safe_str_len, 
		safe_inv, safe_str, safe_enc);
	
	printf("cipher:%s, len:%d\n", safe_enc, strlen(safe_enc));
	printf("0x%2x, cipher len %d\n", safe_enc[15], safe_str_len);
	buf_dump(safe_enc, safe_str_len);
	printf("\n");
	
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_str) mem_free(safe_str);
	
	*out = safe_enc;
	log_debug("aes_cbc_enc_string ok");
	return et_ok;
	
err:
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_str) mem_free(safe_str);
	if (safe_enc) mem_free(safe_enc);
	return et_aes_enc;
}

err_type aes_cbc_dec_string(uint8_t *key, uint8_t *inv, int key_len, 
								   uint8_t *str, uint8_t **out)
{
	int len;
	int safe_key_len;
	int safe_inv_len;
	int safe_str_len;
	uint8_t *safe_key;
	uint8_t *safe_inv;
	uint8_t *safe_dec;
	mbedtls_aes_context ctx;

	if (!key || !inv || !str || key_len<=0)
		return et_aes_enc;

	len = strlen(key);
	if (len > key_len)
		return et_aes_enc;
	len = strlen(inv);
	if (len > key_len)
		return et_aes_enc;

	printf("\n\n");
	log_debug("aes_cbc_dec_string in");
	
	safe_key = awoke_string_padding(key, ' ', key_len, &safe_key_len);
	if (!safe_key)
		goto err;

	safe_inv = awoke_string_padding(inv, ' ', key_len, &safe_inv_len);
	if (!safe_inv)
		goto err;	

	mbedtls_aes_init(&ctx);
	safe_dec = mem_alloc_z(strlen(str));

	printf("cipher:%s, len:%d\n", str, strlen(str));
	buf_dump(str, strlen(str));
	printf("\n");

	mbedtls_aes_setkey_enc(&ctx, safe_key, 128);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, strlen(str), 
		safe_inv, str, safe_dec);

	printf("plain:%s, len:%d\n", safe_dec, strlen(safe_dec));
	buf_dump(safe_dec, strlen(safe_dec));
	printf("\n");		
		
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	*out = safe_dec;
	log_debug("aes_cbc_dec_string ok");
	return et_ok;
err:
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_dec) mem_free(safe_dec);
	return et_aes_enc;
}
								   
err_type aes_cbc_enc_byte(uint8_t *key, int key_len, 
							    uint8_t *inv, int inv_len, int align, 
							    uint8_t *buf, int buf_len, uint8_t *out,
							    int *out_len)
{
	log_debug("g_aes_128_vec");
	buf_dump(inv, inv_len);

   	int len;
   	int safe_key_len;
   	int safe_inv_len;
   	int safe_buf_len;
   	uint8_t *safe_key;
   	uint8_t *safe_inv;
	uint8_t *safe_buf;
	uint8_t *safe_out;
	mbedtls_aes_context ctx;

	if (!key || !inv || !buf || key_len<=0)
		return et_aes_enc;

	if (key_len > align)
		return et_aes_enc;
	if (inv_len > align)
		return et_aes_enc;

	log_debug("aes_cbc_enc_byte in");

	mbedtls_aes_init(&ctx);

	safe_key = awoke_byte_padding(key, key_len, 0x0, align, &safe_key_len);
	if (!safe_key)
		goto err;

	safe_inv = awoke_byte_padding(inv, inv_len, 0x0, align, &safe_inv_len);
	if (!safe_inv)
		goto err;

	/*safe_buf = awoke_byte_padding(buf, buf_len, 0x0, align, &safe_buf_len);
	if (!safe_buf)
		goto err;

	safe_out = mem_alloc_z(safe_buf_len);*/
	#define AES_128_ALIGN(len)	((((len)>>4)+1)<<4)    
	safe_buf_len = AES_128_ALIGN(buf_len);

	printf("key len %d, byte:\n", key_len);
	buf_dump(key, key_len);
	printf("\n");
	printf("safe key len %d, byte:\n", safe_key_len);
	buf_dump(safe_key, safe_key_len);
	printf("\n");

	printf("inv len %d, byte:\n", inv_len);
	buf_dump(inv, inv_len);
	printf("\n");
	printf("safe inv len %d, byte:\n", safe_inv_len);
	buf_dump(safe_inv, safe_inv_len);
	printf("\n");

	printf("plain len %d, byte:\n", buf_len);
	buf_dump(buf, buf_len);
	printf("\n");
	/*
	printf("safe plain len %d, byte:\n", safe_buf_len);
	buf_dump(safe_buf, safe_buf_len);
	printf("\n");*/

	mbedtls_aes_setkey_enc(&ctx, safe_key, 128);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, safe_buf_len, 
		safe_inv, buf, out);	

	printf("cipher len %d, byte:\n", safe_buf_len);
	buf_dump(out, safe_buf_len);
	printf("\n");	
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_buf) mem_free(safe_buf);
	//*out = safe_out;
	*out_len = safe_buf_len;
	log_debug("aes_cbc_enc_byte ok");
	printf("\n");	
	return et_ok;
err:
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_buf) mem_free(safe_buf);
	return et_ok;
}

err_type aes_cbc_dec_byte(uint8_t *key, int key_len, 
								uint8_t *inv, int inv_len, int align, 
								uint8_t *buf, int buf_len, uint8_t *out,
								int *out_len)
{
   	int len;
   	int safe_key_len;
   	int safe_inv_len;
   	uint8_t *safe_key;
   	uint8_t *safe_inv;
	uint8_t *safe_out;
	mbedtls_aes_context ctx;

	if (!key || !inv || !buf || key_len<=0)
		return et_aes_enc;

	if (key_len > align)
		return et_aes_enc;
	if (inv_len > align)
		return et_aes_enc;	

	printf("\n\n");
	log_debug("aes_cbc_dec_byte in");

	mbedtls_aes_init(&ctx);

	safe_key = awoke_byte_padding(key, key_len, 0x0, align, &safe_key_len);
	if (!safe_key)
		goto err;

	safe_inv = awoke_byte_padding(inv, inv_len, 0x0, align, &safe_inv_len);
	if (!safe_inv)
		goto err;

	//safe_out = mem_alloc_z(buf_len);

	printf("cipher len %d, byte:\n", buf_len);
	buf_dump(buf, buf_len);
	printf("\n");

	mbedtls_aes_setkey_enc(&ctx, safe_key, 128);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, buf_len, 
		safe_inv, buf, out);		

	printf("plain len %d, byte:\n", buf_len);
	buf_dump(out, buf_len);
	printf("\n");	

	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	//*out = safe_out;
	*out_len = buf_len;
	log_debug("aes_cbc_dec_byte ok");
	return et_ok;
	
err:
	if (safe_key) mem_free(safe_key);
	if (safe_inv) mem_free(safe_inv);
	if (safe_out) mem_free(safe_out);
	return et_aes_enc;
}

static uint8_t g_aes_128_vec[16] = {0x1};
static uint8_t g_aes_128_key[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
									0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00};

uint16_t hqnb_security_aes128_enc(uint8_t *in, uint16_t len, uint8_t *out)
{
	uint16_t align_len;
	mbedtls_aes_context ctx;   

	if (!in || len==0)
		return -1;

	uint8_t vec[16];
	uint8_t key[16];

	memcpy(vec, g_aes_128_vec, 16);
	memcpy(key, g_aes_128_key, 16);
	
	    
	align_len = SEC_AES128_ALIGN(len);

	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_enc(&ctx, key, 128);	

	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, align_len, 
		vec, in, out);
	mbedtls_aes_free(&ctx);

	return align_len;
}

int sec_aes128_enc_bytes(uint8_t *key, uint8_t *inv, uint8_t *bytes, 
						 uint16_t len, uint8_t *out, uint16_t out_len)
{
	uint16_t align_len;
	mbedtls_aes_context ctx;  

	if (!key || !inv || !bytes || !out || len==0)
		return -1;

	uint8_t _inv[SEC_AES128_KEY_LEN];
	uint8_t _key[SEC_AES128_KEY_LEN];

	memcpy(_inv, inv, SEC_AES128_KEY_LEN);
	memcpy(_key, key, SEC_AES128_KEY_LEN);

	align_len = SEC_AES128_ALIGN(len);
	if (align_len > out_len)
		return -1;

	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_enc(&ctx, _key, SEC_AES128_KEYBITS);	
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, align_len, 
		_inv, bytes, out);

	mbedtls_aes_free(&ctx);

	return align_len;
}

int sec_aes128_dec_bytes(uint8_t *key, uint8_t *inv, uint8_t *bytes, 
						 uint16_t len, uint8_t *out, uint16_t out_len)
{
	mbedtls_aes_context ctx;  

	if (!key || !inv || !bytes || !out || len==0)
		return -1;

	if (len > out_len)
		return -1;
	
	uint8_t _inv[SEC_AES128_KEY_LEN];
	uint8_t _key[SEC_AES128_KEY_LEN];

	memcpy(_inv, inv, SEC_AES128_KEY_LEN);
	memcpy(_key, key, SEC_AES128_KEY_LEN);

	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_dec(&ctx, _key, SEC_AES128_KEYBITS);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, len, 
		_inv, bytes, out);

	mbedtls_aes_free(&ctx);

	return len;
}
