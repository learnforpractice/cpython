#ifndef softfloat_h
#define softfloat_h 1

typedef struct { uint16_t v; } float16_t;
typedef struct { uint32_t v; } float32_t;
typedef struct { uint64_t v; } float64_t;
typedef struct { uint64_t v[2]; } float128_t;

#define bool char

float64_t f64_add( float64_t, float64_t );
float64_t f64_sub( float64_t, float64_t );
float64_t f64_mul( float64_t, float64_t );
float64_t f64_mulAdd( float64_t, float64_t, float64_t );
float64_t f64_div( float64_t, float64_t );
float64_t f64_rem( float64_t, float64_t );
float64_t f64_sqrt( float64_t );
bool f64_eq( float64_t, float64_t );
bool f64_le( float64_t, float64_t );
bool f64_lt( float64_t, float64_t );

/*----------------------------------------------------------------------------
| Ported from OpenCV and fdlibm and added for usability
*----------------------------------------------------------------------------*/

float32_t f32_powi( float32_t x, int y);
float64_t f64_powi( float64_t x, int y);
float64_t f64_pow( float64_t x, float64_t y);

#endif
