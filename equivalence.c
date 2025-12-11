
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
uint8_t nondet_uint8(void);
/*
 * NB: This file can only be run by CBMC. Clang/GCC will not know what is going on here.
*/

#define MASK 0x0F
typedef uint8_t my_uint;     
typedef uint16_t my_uint_wide; // To hold products (max 15*15 = 225, fits in 8 bits)

/* FIRST IMPLEMENTATION
 * Assumes 'n' is odd.
 * 'r' is typically a power of 2 equal to the word size (e.g., 2^64).
 * 'n_prime' is the modular inverse of -n mod r, precomputed using the Extended Euclidean Algorithm.
 * Inputs a_bar and b_bar are in Montgomery form (a*R mod n, b*R mod n).
 */
my_uint montgomery_multiplication(my_uint a_bar, my_uint b_bar, my_uint n, my_uint n_prime) {
    // Product fits in 8 bits, T = a_bar * b_bar
    my_uint_wide T = (my_uint_wide)a_bar * b_bar;
    
    // m fits in 4 bits (by definition modulo R), m = (T mod r) * n_prime mod r
    my_uint m = (my_uint)T * n_prime;
    m = m & MASK;
    
    // m*n fits in 16 bits
    my_uint_wide m_times_n = (my_uint_wide)m * n;
    
    // Sum can exceed 16 bits (max ~131070). 
    // We use a container > 16 bits to catch the carry.
    my_uint_wide sum = (uint32_t)T + m_times_n;
    
    // Shift right by 8. 
    // Result max value is ~131070 / 256 = 511, this fits in 9 bits, so my_uint_wide is plenty.
    my_uint_wide u = (sum >> 4); 

    // Final reduction: if u >= n, return u - n, else return u
    // Now check against n using the wide value
    if (u >= n) {
        return (my_uint)(u - n);
    } else {
        return (my_uint)u;
    }
}

/* SECOND IMPLEMENTATION
 * Computes (a * b) % m using a larger type to prevent overflow
 */
my_uint regular_mul_mod(my_uint a, my_uint b, my_uint m) {
    my_uint_wide res = (my_uint_wide)a * b;
    return (my_uint_wide)(res % m);
}

/*
 * For converting affine coordinates to Montgomery. 
 */
my_uint to_mont(my_uint a, my_uint n) {
    // We verify a < n typically, though not strictly required by math
    // R is 2^8. We perform (a * 2^8) by shifting a 16-bit value.
    my_uint_wide res = (my_uint_wide)a << 4; 
    return (my_uint)(res % n);
}

/*
 * For converting Montgomery coordinates to affine.
 */
my_uint from_mont(my_uint a, my_uint r_inv, my_uint n) {
    // r_inv must be precomputed such that (2^8 * r_inv) % n == 1
    my_uint_wide res = (my_uint_wide)a * r_inv;
    return (my_uint)(res % n);
}

/*
 * https://www.carstensinz.de/papers/ICST-2009.pdf. This produces a "miter harness"
 * which CBMC will accept and produce UNSAT proof for.
 */
void miter() {
    my_uint n = nondet_uint8() & MASK;
    my_uint r_inv = nondet_uint8() & MASK;
    my_uint n_prime = nondet_uint8() & MASK;
    my_uint a = nondet_uint8() & MASK;
    my_uint b = nondet_uint8() & MASK;

    // 2. Constraints (Assumptions)
    __CPROVER_assume(n == 13); // A concrete prime
    __CPROVER_assume(n % 2 != 0); // Montgomery requires odd modulus
    __CPROVER_assume(a < n);
    __CPROVER_assume(b < n);
    
    // Constrain r_inv to be the correct mathematical inverse of 2^8 mod n
    my_uint_wide R = 16; // This is 2^8

    // R * r_inv == 1 mod n
    __CPROVER_assume(((my_uint_wide)r_inv * (R % n)) % n == 1 );

     // n * n_prime == -1 mod R (mod 2^8)
    __CPROVER_assume( ((my_uint)(n * n_prime) & MASK) == 15 );

    // execute functions sequentially
    my_uint expected = regular_mul_mod(a, b, n);

    //for montgomery
    my_uint a_mont = to_mont(a, n); 
    my_uint b_mont = to_mont(b, n);
    
    // Run optimized Montgomery Multiplication
    my_uint res_mont = montgomery_multiplication(a_mont, b_mont, n, n_prime);
    
    // Convert back using reduction
    my_uint actual = from_mont(res_mont, r_inv, n);

    assert(actual == expected);
}

int main() {
    miter();
    return 0;
}