#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
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

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"
#include "awoke_string.h"
#include "awoke_socket.h"
#include "awoke_event.h"

#include "fix_math.h"

fix16_t fix16_sqrt(fix16_t inValue)
{
	uint8_t  neg = (inValue < 0);
	uint32_t num = (neg ? -inValue : inValue);
	uint32_t result = 0;
	uint32_t bit;
	uint8_t  n;
	
	// Many numbers will be less than 15, so
	// this gives a good balance between time spent
	// in if vs. time spent in the while loop
	// when searching for the starting value.
	if (num & 0xFFF00000)
		bit = (uint32_t)1 << 30;
	else
		bit = (uint32_t)1 << 18;
	
	while (bit > num) bit >>= 2;
	
	// The main part is executed twice, in order to avoid
	// using 64 bit values in computations.
	for (n = 0; n < 2; n++)
	{
		// First we get the top 24 bits of the answer.
		while (bit)
		{
			if (num >= result + bit)
			{
				num -= result + bit;
				result = (result >> 1) + bit;
			}
			else
			{
				result = (result >> 1);
			}
			bit >>= 2;
		}
		
		if (n == 0)
		{
			// Then process it again to get the lowest 8 bits.
			if (num > 65535)
			{
				// The remainder 'num' is too large to be shifted left
				// by 16, so we have to add 1 to result manually and
				// adjust 'num' accordingly.
				// num = a - (result + 0.5)^2
				//	 = num + result^2 - (result + 0.5)^2
				//	 = num - result - 0.5
				num -= result;
				num = (num << 16) - 0x8000;
				result = (result << 16) + 0x8000;
			}
			else
			{
				num <<= 16;
				result <<= 16;
			}
			
			bit = 1 << 14;
		}
	}

#ifndef FIXMATH_NO_ROUNDING
	// Finally, if next bit would have been 1, round the result upwards.
	if (num > result)
	{
		result++;
	}
#endif
	
	return (neg ? -(fix16_t)result : (fix16_t)result);
}

#ifndef FIXMATH_NO_CACHE
static fix16_t _fix16_atan_cache_index[2][4096] = { { 0 }, { 0 } };
static fix16_t _fix16_atan_cache_value[4096] = { 0 };
#endif

#ifdef __GNUC__
// Count leading zeros, using processor-specific instruction if available.
#define clz(x) (__builtin_clzl(x) - (8 * sizeof(long) - 32))
#else
static uint8_t clz(uint32_t x)
{
	uint8_t result = 0;
	if (x == 0) return 32;
	while (!(x & 0xF0000000)) { result += 4; x <<= 4; }
	while (!(x & 0x80000000)) { result += 1; x <<= 1; }
	return result;
}
#endif


fix16_t fix16_div(fix16_t a, fix16_t b)
{
	// This uses a hardware 32/32 bit division multiple times, until we have
	// computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.
	
	if (b == 0)
			return fix16_minimum;
	
	uint32_t remainder = (a >= 0) ? a : (-a);
	uint32_t divider = (b >= 0) ? b : (-b);
	uint32_t quotient = 0;
	int bit_pos = 17;
	
	// Kick-start the division a bit.
	// This improves speed in the worst-case scenarios where N and D are large
	// It gets a lower estimate for the result by N/(D >> 17 + 1).
	if (divider & 0xFFF00000)
	{
		uint32_t shifted_div = ((divider >> 17) + 1);
		quotient = remainder / shifted_div;
		remainder -= ((uint64_t)quotient * divider) >> 17;
	}
	
	// If the divider is divisible by 2^n, take advantage of it.
	while (!(divider & 0xF) && bit_pos >= 4)
	{
		divider >>= 4;
		bit_pos -= 4;
	}
	
	while (remainder && bit_pos >= 0)
	{
		// Shift remainder as much as we can without overflowing
		int shift = clz(remainder);
		if (shift > bit_pos) shift = bit_pos;
		remainder <<= shift;
		bit_pos -= shift;
		
		uint32_t div = remainder / divider;
		remainder = remainder % divider;
		quotient += div << bit_pos;

		#ifndef FIXMATH_NO_OVERFLOW
		if (div & ~(0xFFFFFFFF >> bit_pos))
				return fix16_overflow;
		#endif
		
		remainder <<= 1;
		bit_pos--;
	}
	
	#ifndef FIXMATH_NO_ROUNDING
	// Quotient is always positive so rounding is easy
	quotient++;
	#endif
	
	fix16_t result = quotient >> 1;
	
	// Figure out the sign of the result
	if ((a ^ b) & 0x80000000)
	{
		#ifndef FIXMATH_NO_OVERFLOW
		if (result == fix16_minimum)
				return fix16_overflow;
		#endif
		
		result = -result;
	}
	
	return result;
}

fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
	int64_t product = (int64_t)inArg0 * inArg1;
	
	#ifndef FIXMATH_NO_OVERFLOW
	// The upper 17 bits should all be the same (the sign).
	uint32_t upper = (product >> 47);
	#endif
	
	if (product < 0)
	{
		#ifndef FIXMATH_NO_OVERFLOW
		if (~upper)
				return fix16_overflow;
		#endif
		
		#ifndef FIXMATH_NO_ROUNDING
		// This adjustment is required in order to round -1/2 correctly
		product--;
		#endif
	}
	else
	{
		#ifndef FIXMATH_NO_OVERFLOW
		if (upper)
				return fix16_overflow;
		#endif
	}
	
	#ifdef FIXMATH_NO_ROUNDING
	return product >> 16;
	#else
	fix16_t result = product >> 16;
	result += (product & 0x8000) >> 15;
	
	return result;
	#endif
}


fix16_t fix16_atan2(fix16_t inY , fix16_t inX)
{
	fix16_t abs_inY, mask, angle, r, r_3;

	#ifndef FIXMATH_NO_CACHE
	uint32_t hash = (inX ^ inY);
	hash ^= hash >> 20;
	hash &= 0x0FFF;
	if((_fix16_atan_cache_index[0][hash] == inX) && (_fix16_atan_cache_index[1][hash] == inY))
		return _fix16_atan_cache_value[hash];
	#endif

	/* Absolute inY */
	mask = (inY >> (sizeof(fix16_t)*CHAR_BIT-1));
	abs_inY = (inY + mask) ^ mask;

	if (inX >= 0)
	{
		r = fix16_div( (inX - abs_inY), (inX + abs_inY));
		r_3 = fix16_mul(fix16_mul(r, r),r);
		angle = fix16_mul(0x00003240 , r_3) - fix16_mul(0x0000FB50,r) + PI_DIV_4;
	} else {
		r = fix16_div( (inX + abs_inY), (abs_inY - inX));
		r_3 = fix16_mul(fix16_mul(r, r),r);
		angle = fix16_mul(0x00003240 , r_3)
			- fix16_mul(0x0000FB50,r)
			+ THREE_PI_DIV_4;
	}
	if (inY < 0)
	{
		angle = -angle;
	}

	#ifndef FIXMATH_NO_CACHE
	_fix16_atan_cache_index[0][hash] = inX;
	_fix16_atan_cache_index[1][hash] = inY;
	_fix16_atan_cache_value[hash] = angle;
	#endif

	return angle;
}

void vector_narrow(vector *vs, vector *vr, int k)
{
	vr->x = vs->x/k;
	vr->y = vs->y/k;
	vr->z = vs->z/k;
}

void fix16_euler_angle_int(vec3_int16 *vr, vec3_int16 *vec)
{
	fix16_t atan2x, atan2y;
	fix16_t anglex, angley;

	atan2x = fix16_from_int(vec->x);
	atan2y = fix16_sqrt(fix16_from_int(vec->y*vec->y + vec->z*vec->z));
	log_debug("angle x %d", fix16_to_int(atan2x));
	log_debug("angle y %d", fix16_to_int(fix16_from_int(vec->y*vec->y + vec->z*vec->z)));
	anglex = fix16_atan2(atan2x, atan2y);
	//vr->x = fix16_to_int(anglex)*180/3;
	vr->x = fix16_to_int(fix16_mul(anglex,fix16_from_int(180/3.14)));
	
	atan2x = fix16_from_int(vec->y);
	atan2y = fix16_sqrt(fix16_from_int(vec->x*vec->x + vec->z*vec->z));
	angley = fix16_atan2(atan2x, atan2y);
	//vr->y = fix16_to_int(angley)*180/3;
	vr->y = fix16_to_int(fix16_mul(angley,fix16_from_int(180/3.14)));

	vr->z = 0;
}

void fix16_euler_angle(vector *vr, vector *vec)
{
	vector vs;
	fix16_t atan2x, atan2y;
	fix16_t anglex, angley;

	vector_narrow(vec, &vs, 10000);
	//vector_copy(&vs, vec);

	atan2x = fix16_from_dbl(vs.x);
	atan2y = fix16_sqrt(fix16_from_dbl(vs.y*vs.y + vs.z*vs.z));
	anglex = fix16_atan2(atan2x, atan2y);
	vr->x = fix16_to_dbl(fix16_atan2(atan2x, atan2y))*180/AGCT_PAT;

	atan2x = fix16_from_dbl(vs.y);
	atan2y = fix16_sqrt(fix16_from_dbl(vs.x*vs.x + vs.z*vs.z));
	angley = fix16_atan2(atan2x, atan2y);
	vr->y = fix16_to_dbl(fix16_atan2(atan2x, atan2y))*180/AGCT_PAT;

	log_debug("angle x %d", (int)(vr->x));
	log_debug("angle y %d", (int)(vr->y));

	vr->z = 0;
}

static inline vector *vec_euler_angle(vector *v_angle, vector *vec)
{
	v_angle->x = atan2(vec->x, sqrt(pow(vec->y, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->y = atan2(vec->y, sqrt(pow(vec->x, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->z = atan2(vec->z, sqrt(pow(vec->x, 2) + pow(vec->y, 2)))*180/PAI;
	log_debug("x y z %d %d %d", (int)v_angle->x, (int)v_angle->y, (int)v_angle->z);
	return v_angle;
}

static inline void vec16_euler_angle(vec3_int16 *v_angle, vec3_int16 *vec)
{
	log_debug("angle x %d", vec->x);
	log_debug("angle y %d", (int)(pow(vec->y, 2) + pow(vec->z, 2)));
	v_angle->x = atan2(vec->x, sqrt(pow(vec->y, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->y = atan2(vec->y, sqrt(pow(vec->x, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->z = 0;
	return v_angle;
}


int main(int argc, char **argv)
{
	log_level(LOG_DBG);
	log_mode(LOG_TEST);

	/*
	double src = 12345.67;
	log_debug("src %lf", fix16_to_dbl(fix16_from_dbl(src)));
	log_debug("fixsqrt result %lf", fix16_to_dbl(fix16_sqrt(fix16_from_dbl(src))));
	log_debug("mathsqrt result %lf", sqrt(src));*/

	/*
	double atan2_x = 2345.67;
	double atan2_y = 3456.78;
	double atan2_r = 0;
	
	log_debug("math atan2 x:%lf, y:%lf, r:%lf", atan2_x,
												atan2_y,
												atan2(atan2_x, atan2_y));
	
	log_debug("fix  atan2 x:%lf, y:%lf, r:%lf", fix16_from_dbl(atan2_x),
												fix16_from_dbl(atan2_y),
												fix16_atan2(fix16_from_dbl(atan2_x), 
															fix16_from_dbl(atan2_y)));
	*/

	vector v, vfix, ve;
	//v.x = -22387.000000;
	//v.y = 6893.000000;
	//v.z = 1018334.000000;
	//v.x = -122387.000000;
	//v.y = 96893.000000;
	//v.z = 118334.000000;
	int16_t x = 16;
	int16_t y = 0;
	int16_t z = -16;
	v.x = (double)x;
	v.y = (double)y;
	v.z = (double)z;
	log_debug("vec v (%d, %d, %d)", vector_print_int(v));
	fix16_euler_angle(&vfix, &v);
	vec_euler_angle(&ve, &v);
	log_debug("vec euler (%d, %d, %d)", vector_print_int(ve));
	log_debug("fix euler (%d, %d, %d)", vector_print_int(vfix));
	fix16_t anglex = -5876942;
	fix16_t angley = -5876942;
	

	/*
	vec3_int16 v, vfix, ve;
	v.x = -96;
	v.y = 960;
	v.z = 32;
	fix16_euler_angle_int(&vfix, &v);
	vec16_euler_angle(&ve, &v);
	log_debug("vec euler (%d, %d, %d)", vec16_print(ve));
	log_debug("fix euler (%d, %d, %d)", vec16_print(vfix));
	*/
	return 0;
}

