#ifndef __FIX_MATH_H__
#define __FIX_MATH_H__

#define FIXMATH_NO_ROUNDING

#define CHAR_BIT 8

typedef int32_t fix16_t;

#define AGCT_PAT	3.14

static const fix16_t fix16_one = 0x00010000; /*!< fix16_t value of 1 */
static const fix16_t PI_DIV_4 = 0x0000C90F;             /*!< Fix16 value of PI/4 */
static const fix16_t fix16_minimum  = 0x80000000; /*!< the minimum value of fix16_t */
static const fix16_t fix16_overflow = 0x80000000; /*!< the value used to indicate overflows when FIXMATH_NO_OVERFLOW is not specified */
static const fix16_t THREE_PI_DIV_4 = 0x00025B2F;       /*!< Fix16 value of 3PI/4 */

static inline double  fix16_to_dbl(fix16_t a)   { return (double)a / fix16_one; }

static inline fix16_t fix16_from_int(int a)     { return a * fix16_one; }

static inline fix16_t fix16_from_dbl(double a)
{
	double temp = a * fix16_one;
#ifndef FIXMATH_NO_ROUNDING
	temp += (temp >= 0) ? 0.5f : -0.5f;
#endif
	return (fix16_t)temp;
}

static inline int fix16_to_int(fix16_t a)
{
#ifdef FIXMATH_NO_ROUNDING
	printf("FIXMATH_NO_ROUNDING\n");
    return (a >> 16);
#else
	if (a >= 0)
		return (a + (fix16_one >> 1)) / fix16_one;
	return (a - (fix16_one >> 1)) / fix16_one;
#endif
}

typedef struct _vec3_int16 {
	int16_t x;
	int16_t y;
	int16_t z;
}vec3_int16;

#define vec16_print(vec) \
	vec.x, vec.y, vec.z

#endif /* __FIX_MATH_H__ */
