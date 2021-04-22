#pragma once
#include <cstdint>
#include <cstring>
#include <array>
#include <type_traits>
#include <vector>
#include <string>

#pragma warning(disable:4984)

/*
// standalone hash
std::array<int, 4> input {322, 2137, 42069, 65536};
xxh::hash_t<32> hash = xxh::xxhash<32>(input); 

// hash streaming
std::array<unsigned char, 512> buffer;
xxh::hash_state_t<64> hash_stream; 
while (fill_buffer(buffer))
{
  hash_stream.update(buffer);
}
xxh::hash_t<64> final_hash = hash_stream.digest();

The template argument specifies whether the algorithm will use the 32 or 64 bit version. 
Other values are not allowed. Typedefs hash32_t, hash64_t, hash_state32_t and 
hash_state64_t are provided.
*/

/*
xxHash - Extremely Fast Hash algorithm
Header File
Copyright (C) 2012-2020, Yann Collet.
Copyright (C) 2017-2020, Red Gavin.
All rights reserved.

BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
You can contact the author at :
- xxHash source repository : https://github.com/Cyan4973/xxHash
- xxHash C++ port repository : https://github.com/RedSpah/xxhash_cpp
*/

/* Intrinsics
* Sadly has to be included in the global namespace or literally everything breaks
*/
#include <immintrin.h>

namespace xxh
{
	/* *************************************
	*  Versioning
	***************************************/

	namespace version
	{
		constexpr int cpp_version_major = 0;
		constexpr int cpp_version_minor = 7;
		constexpr int cpp_version_release = 3;
	}

	constexpr uint32_t version_number() 
	{ 
		return version::cpp_version_major * 10000 + version::cpp_version_minor * 100 + version::cpp_version_release;
	}


	/* *************************************
	*  Basic Types - Predefining uint128_t for intrin
	***************************************/

	namespace typedefs
	{
		struct alignas(16) uint128_t
		{
			uint64_t low64 = 0;
			uint64_t high64 = 0;

			bool operator==(const uint128_t & other)
			{
				return (low64 == other.low64 && high64 == other.high64);
			}

			bool operator>(const uint128_t & other)
			{
				return (high64 > other.high64 || low64 > other.low64);
			}

			bool operator>=(const uint128_t & other)
			{
				return (*this > other || *this == other);
			}

			bool operator<(const uint128_t & other)
			{
				return !(*this >= other);
			}

			bool operator<=(const uint128_t & other)
			{
				return !(*this > other);
			}

			bool operator!=(const uint128_t & other)
			{
				return !(*this == other);
			}

			uint128_t(uint64_t low, uint64_t high) : low64(low), high64(high) {}

			uint128_t() {}
		};

	}

	using uint128_t = typedefs::uint128_t;


	/* *************************************
	*  Compiler / Platform Specific Features
	***************************************/

	namespace intrin
	{
		/*!XXH_CPU_LITTLE_ENDIAN :
		* This is a CPU endian detection macro, will be
		* automatically set to 1 (little endian) if it is left undefined.
		* If compiling for a big endian system (why), XXH_CPU_LITTLE_ENDIAN has to be explicitly defined as 0.
		*/
#ifndef XXH_CPU_LITTLE_ENDIAN
#	define XXH_CPU_LITTLE_ENDIAN 1
#endif


		/* Vectorization Detection
		* NOTE: XXH_NEON and XXH_VSX aren't supported in this C++ port.
		* The primary reason is that I don't have access to an ARM and PowerPC
		* machines to test them, and the secondary reason is that I even doubt anyone writing
		* code for such machines would bother using a C++ port rather than the original C version.
		*/
#ifndef XXH_VECTOR   /* can be predefined on command line */
#	if defined(__AVX2__)
#		define XXH_VECTOR 2 /* AVX2 for Haswell and Bulldozer */
#	elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#		define XXH_VECTOR 1 /* SSE2 for Pentium 4 and all x86_64 */
#	else
#		define XXH_VECTOR 0 /* Portable scalar version */
#	endif
#endif

		constexpr int vector_mode = XXH_VECTOR;

#if XXH_VECTOR == 2		/* AVX2 for Haswell and Bulldozer */
		constexpr int acc_align = 32;
		using avx2_underlying = __m256i;
		using sse2_underlying = __m128i;
#elif XXH_VECTOR == 1	/* SSE2 for Pentium 4 and all x86_64 */
		using avx2_underlying = void; //std::array<__m128i, 2>;
		using sse2_underlying = __m128i;
		constexpr int acc_align = 16;
#else					/* Portable scalar version */
		using avx2_underlying = void; //std::array<uint64_t, 4>;
		using sse2_underlying = void; //std::array<uint64_t, 2>;
		constexpr int acc_align = 8;
#endif


		/* Compiler Specifics
		* Defines inline macros and includes specific compiler's instrinsics.
		* */
#ifdef XXH_FORCE_INLINE /* First undefining the symbols in case they're already defined */
#	undef XXH_FORCE_INLINE
#endif 
#ifdef XXH_NO_INLINE
#	undef XXH_NO_INLINE
#endif

#ifdef _MSC_VER    /* Visual Studio */
#	pragma warning(disable : 4127)    
#	define XXH_FORCE_INLINE static __forceinline
#	define XXH_NO_INLINE static __declspec(noinline)
#	include <intrin.h>
#elif defined(__GNUC__)  /* Clang / GCC */
#	define XXH_FORCE_INLINE static inline __attribute__((always_inline))
#	define XXH_NO_INLINE static __attribute__((noinline))
#	include <mmintrin.h>
#else
#	define XXH_FORCE_INLINE static inline
#	define XXH_NO_INLINE static
#endif


		/* Prefetch
		* Can be disabled by defining XXH_NO_PREFETCH
		*/
#if defined(XXH_NO_PREFETCH)
		XXH_FORCE_INLINE void prefetch(const void* ptr) {}
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))  
		XXH_FORCE_INLINE void prefetch(const void* ptr) { _mm_prefetch((const char*)(ptr), _MM_HINT_T0); }
#elif defined(__GNUC__) 
		XXH_FORCE_INLINE void prefetch(const void* ptr) { __builtin_prefetch((ptr), 0, 3); }
#else
		XXH_FORCE_INLINE void prefetch(const void* ptr) {}
#endif


		/* Restrict
		* Defines macro for restrict, which in C++ is sadly just a compiler extension (for now).
		* Can be disabled by defining XXH_NO_RESTRICT
		*/
#ifdef XXH_RESTRICT
#	undef XXH_RESTRICT
#endif

#if (defined(__GNUC__) || defined(_MSC_VER)) && defined(__cplusplus) && !defined(XXH_NO_RESTRICT)
#	define XXH_RESTRICT  __restrict
#else
#	define XXH_RESTRICT 
#endif


		/* Likely / Unlikely
		* Defines macros for Likely / Unlikely, which are official in C++20, but sadly this library aims the previous standard.
		* Not present on MSVC.
		* Can be disabled by defining XXH_NO_BRANCH_HINTS
		*/
#if ((defined(__GNUC__) && (__GNUC__ >= 3))  || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) || defined(__clang__)) && !defined(XXH_NO_BRANCH_HINTS)
#    define XXH_likely(x) __builtin_expect(x, 1)
#    define XXH_unlikely(x) __builtin_expect(x, 0)
#else
#    define XXH_likely(x) (x)
#    define XXH_unlikely(x) (x)
#endif


		namespace bit_ops
		{
#if defined(_MSC_VER)
			static inline uint32_t rotl32(uint32_t x, int32_t r) { return _rotl(x, r); }
			static inline uint64_t rotl64(uint64_t x, int32_t r) { return _rotl64(x, r); }
			static inline uint32_t rotr32(uint32_t x, int32_t r) { return _rotr(x, r); }
			static inline uint64_t rotr64(uint64_t x, int32_t r) { return _rotr64(x, r); }
#else
			static inline uint32_t rotl32(uint32_t x, int32_t r) { return ((x << r) | (x >> (32 - r))); }
			static inline uint64_t rotl64(uint64_t x, int32_t r) { return ((x << r) | (x >> (64 - r))); }
			static inline uint32_t rotr32(uint32_t x, int32_t r) { return ((x >> r) | (x << (32 - r))); }
			static inline uint64_t rotr64(uint64_t x, int32_t r) { return ((x >> r) | (x << (64 - r))); }
#endif


#if defined(_MSC_VER)     /* Visual Studio */
			static inline uint32_t swap32(uint32_t x) { return _byteswap_ulong(x); }
			static inline uint64_t swap64(uint64_t x) { return _byteswap_uint64(x); }
#elif defined(__GNUC__)
			static inline uint32_t swap32(uint32_t x) { return __builtin_bswap32(x); }
			static inline uint64_t swap64(uint64_t x) { return __builtin_bswap64(x); }
#else
			static inline uint32_t swap32(uint32_t x) { return ((x << 24) & 0xff000000) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff); }
			static inline uint64_t swap64(uint64_t x) { return ((x << 56) & 0xff00000000000000ULL) | ((x << 40) & 0x00ff000000000000ULL) | ((x << 24) & 0x0000ff0000000000ULL) | ((x << 8) & 0x000000ff00000000ULL) | ((x >> 8) & 0x00000000ff000000ULL) | ((x >> 24) & 0x0000000000ff0000ULL) | ((x >> 40) & 0x000000000000ff00ULL) | ((x >> 56) & 0x00000000000000ffULL); }
#endif


#if defined(_MSC_VER) && defined(_M_IX86) // Only for 32-bit MSVC.
			XXH_FORCE_INLINE uint64_t mult32to64(uint32_t x, uint32_t y) { return __emulu(x, y); }
#else
			XXH_FORCE_INLINE uint64_t mult32to64(uint32_t x, uint32_t y) { return (uint64_t)(uint32_t)(x) * (uint64_t)(uint32_t)(y); }
#endif


#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
			__attribute__((__target__("no-sse")))
#endif
			static inline uint128_t mult64to128(uint64_t lhs, uint64_t rhs)
			{

#if defined(__GNUC__) && !defined(__wasm__) \
    && defined(__SIZEOF_INT128__) \
    || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

				__uint128_t product = (__uint128_t)lhs * (__uint128_t)rhs;
				uint128_t const r128 = { (uint64_t)(product), (uint64_t)(product >> 64) };
				return r128;

#elif defined(_M_X64) || defined(_M_IA64)

#ifndef _MSC_VER
#   pragma intrinsic(_umul128)
#endif
				uint64_t product_high;
				uint64_t const product_low = _umul128(lhs, rhs, &product_high);
				return uint128_t{ product_low, product_high };
#else
				uint64_t const lo_lo = bit_ops::mult32to64(lhs & 0xFFFFFFFF, rhs & 0xFFFFFFFF);
				uint64_t const hi_lo = bit_ops::mult32to64(lhs >> 32, rhs & 0xFFFFFFFF);
				uint64_t const lo_hi = bit_ops::mult32to64(lhs & 0xFFFFFFFF, rhs >> 32);
				uint64_t const hi_hi = bit_ops::mult32to64(lhs >> 32, rhs >> 32);

				/* Now add the products together. These will never overflow. */
				uint64_t const cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
				uint64_t const upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
				uint64_t const lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

				uint128_t r128 = { lower, upper };
				return r128;
#endif
			}
		}
	}


	/* *************************************
	*  Basic Types - Everything else
	***************************************/

	namespace typedefs
	{
		/* *************************************
		*  Basic Types - Detail
		***************************************/

		template <size_t N>
		struct hash_type
		{
			using type = void;
		};

		template <>
		struct hash_type<32>
		{
			using type = uint32_t;
		};

		template <>
		struct hash_type<64>
		{
			using type = uint64_t;
		};

		template <>
		struct hash_type<128>
		{
			using type = uint128_t;
		};


		template <size_t N>
		struct vec_type
		{
			using type = void;
		};

		template <>
		struct vec_type<64>
		{
			using type = uint64_t;
		};

		template <>
		struct vec_type<128>
		{
			using type = intrin::sse2_underlying;
		};

		template <>
		struct vec_type<256>
		{
			using type = intrin::avx2_underlying;
		};

		/* Rationale
		* On the surface level uint_type appears to be pointless,
		* as it is just a copy of hash_type. They do use the same types,
		* that is true, but the reasoning for the difference is aimed at humans,
		* not the compiler, as a difference between values that are 'just' numbers,
		* and those that represent actual hash values.
		*/
		template <size_t N>
		struct uint_type
		{
			using type = void;
		};

		template <>
		struct uint_type<32>
		{
			using type = uint32_t;
		};

		template <>
		struct uint_type<64>
		{
			using type = uint64_t;
		};

		template <>
		struct uint_type<128>
		{
			using type = uint128_t;
		};
	}

	template <size_t N>
	using hash_t = typename typedefs::hash_type<N>::type;
	using hash32_t = hash_t<32>;
	using hash64_t = hash_t<64>;
	using hash128_t = hash_t<128>;

	template <size_t N>
	using vec_t = typename typedefs::vec_type<N>::type;
	using vec64_t = vec_t<64>;
	using vec128_t = vec_t<128>;
	using vec256_t = vec_t<256>;

	template <size_t N>
	using uint_t = typename typedefs::uint_type<N>::type;
	


	/* *************************************
	*  Bit Operations
	***************************************/

	namespace bit_ops
	{
		/* ****************************************
		*  Bit Operations
		******************************************/

		template <size_t N>
		static inline uint_t<N> rotl(uint_t<N> n, int32_t r)
		{
			if constexpr (N == 32)
			{
				return intrin::bit_ops::rotl32(n, r);
			}

			if constexpr (N == 64)
			{
				return intrin::bit_ops::rotl64(n, r);
			}
		}

		template <size_t N>
		static inline uint_t<N> rotr(uint_t<N> n, int32_t r)
		{
			if constexpr (N == 32)
			{
				return intrin::bit_ops::rotr32(n, r);
			}

			if constexpr (N == 64)
			{
				return intrin::bit_ops::rotr64(n, r);
			}
		}

		template <size_t N>
		static inline uint_t<N> swap(uint_t<N> n)
		{
			if constexpr (N == 32)
			{
				return intrin::bit_ops::swap32(n);
			}

			if constexpr (N == 64)
			{
				return intrin::bit_ops::swap64(n);
			}
		}

		static inline uint64_t mul32to64(uint32_t x, uint32_t y)
		{ 
			return intrin::bit_ops::mult32to64(x, y); 
		}

		static inline uint128_t mul64to128(uint64_t x, uint64_t y)
		{ 
			return intrin::bit_ops::mult64to128(x, y); 
		}

		static inline uint64_t mul128fold64(uint64_t x, uint64_t y)
		{
			uint128_t product = mul64to128(x, y);

			return (product.low64 ^ product.high64);
		}
	}


	/* *************************************
	*  Memory Functions 
	***************************************/

	namespace mem_ops
	{

		/* *************************************
		* Endianness
		***************************************/

		constexpr bool is_little_endian()
		{
			return (XXH_CPU_LITTLE_ENDIAN == 1);
		}


		/* *************************************
		*  Memory Access
		***************************************/

		template <size_t N>
		static inline uint_t<N> read(const void* memPtr)
		{
			uint_t<N> val;

			memcpy(&val, memPtr, sizeof(val));
			return val;
		}

		template <size_t N>
		static inline uint_t<N> readLE(const void* ptr)
		{
			if constexpr (is_little_endian())
			{
				return read<N>(ptr);
			}
			else
			{
				return bit_ops::swap<N>(read<N>(ptr));
			}
		}

		template <size_t N>
		static inline uint_t<N> readBE(const void* ptr)
		{
			if constexpr (is_little_endian())
			{
				return bit_ops::swap<N>(read<N>(ptr));
			}
			else
			{
				return read<N>(ptr);
			}
		}

		template <size_t N>
		static void writeLE(void* dst, uint_t<N> v)
		{
			if constexpr (!is_little_endian())
			{
				v = bit_ops::swap<N>(v);
			}

			memcpy(dst, &v, sizeof(v));
		}
	}


	/* *************************************
	*  Vector Functions 
	***************************************/

	namespace vec_ops
	{
		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> loadu(const vec_t<N>* input)
		{ 
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid template argument passed to xxh::vec_ops::loadu"); 

			if constexpr (N == 128)
			{
				return _mm_loadu_si128(input);
			}

			if constexpr (N == 256)
			{
				return _mm256_loadu_si256(input);
			}

			if constexpr (N == 64)
			{
				return mem_ops::readLE<64>(input);
			}
		}


		// 'xorv' instead of 'xor' because 'xor' is a weird wacky alternate operator expression thing. 
		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> xorv(vec_t<N> a, vec_t<N> b)
		{ 
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::xorv");
		
			if constexpr (N == 128)
			{
				return _mm_xor_si128(a, b);
			}

			if constexpr (N == 256)
			{
				return _mm256_xor_si256(a, b);
			}

			if constexpr (N == 64)
			{
				return a ^ b;
			}
		}
		

		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> mul(vec_t<N> a, vec_t<N> b)
		{
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::mul");

			if constexpr (N == 128)
			{
				return _mm_mul_epu32(a, b);
			}

			if constexpr (N == 256)
			{
				return _mm256_mul_epu32(a, b);
			}

			if constexpr (N == 64)
			{
				return a * b;
			}
		}


		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> add(vec_t<N> a, vec_t<N> b)
		{
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::add");

			if constexpr (N == 128)
			{
				return _mm_add_epi64(a, b);
			}

			if constexpr (N == 256)
			{
				return _mm256_add_epi64(a, b);
			}

			if constexpr (N == 64)
			{
				return a + b;
			}
		}


		template <size_t N, uint8_t S1, uint8_t S2, uint8_t S3, uint8_t S4>
		XXH_FORCE_INLINE vec_t<N> shuffle(vec_t<N> a)
		{ 
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::shuffle");

			if constexpr (N == 128)
			{
				return _mm_shuffle_epi32(a, _MM_SHUFFLE(S1, S2, S3, S4));
			}

			if constexpr (N == 256)
			{
				return _mm256_shuffle_epi32(a, _MM_SHUFFLE(S1, S2, S3, S4));
			}

			if constexpr (N == 64)
			{
				return a;
			}
		}


		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> set1(int a)
		{
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::set1");

			if constexpr (N == 128)
			{
				return _mm_set1_epi32(a);
			}

			if constexpr (N == 256)
			{
				return _mm256_set1_epi32(a);
			}

			if constexpr (N == 64)
			{
				return a;
			}
		}


		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> srli(vec_t<N> n, int a)
		{
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::srli");

			if constexpr (N == 128)
			{
				return _mm_srli_epi64(n, a);
			}

			if constexpr (N == 256)
			{
				return _mm256_srli_epi64(n, a);
			}

			if constexpr (N == 64)
			{
				return n >> a;
			}
		}


		template <size_t N>
		XXH_FORCE_INLINE vec_t<N> slli(vec_t<N> n, int a)
		{
			static_assert(!(N != 128 && N != 256 && N != 64), "Invalid argument passed to xxh::vec_ops::slli");

			if constexpr (N == 128)
			{
				return _mm_slli_epi64(n, a);
			}

			if constexpr (N == 256)
			{
				return _mm256_slli_epi64(n, a);
			}

			if constexpr (N == 64)
			{
				return n << a;
			}
		}
	}


	/* *************************************
	*  Algorithm Implementation - xxhash
	***************************************/

	namespace detail
	{
		using namespace mem_ops;
		using namespace bit_ops;


		/* *************************************
		*  Constants
		***************************************/

		constexpr static std::array<uint32_t, 5> primes32 = { 2654435761U, 2246822519U, 3266489917U, 668265263U, 374761393U };
		constexpr static std::array<uint64_t, 5> primes64 = { 11400714785074694791ULL, 14029467366897019727ULL, 1609587929392839161ULL, 9650029242287828579ULL, 2870177450012600261ULL };

		template <size_t N>
		constexpr uint_t<N> PRIME(uint64_t n) 
		{
			if constexpr (N == 32)
			{
				return primes32[n - 1];
			}
			else
			{
				return primes64[n - 1];
			}
		}


		/* *************************************
		*  Functions
		***************************************/

		template <size_t N>
		static inline uint_t<N> round(uint_t<N> seed, uint_t<N> input)
		{
			seed += input * PRIME<N>(2);

			if constexpr (N == 32)
			{
				seed = rotl<N>(seed, 13);
			}
			else
			{
				seed = rotl<N>(seed, 31);
			}

			seed *= PRIME<N>(1);
			return seed;
		}

		static inline uint64_t mergeRound64(hash64_t acc, uint64_t val)
		{
			val = round<64>(0, val);
			acc ^= val;
			acc = acc * PRIME<64>(1) + PRIME<64>(4);
			return acc;
		}

		static inline void endian_align_sub_mergeround(hash64_t& hash_ret, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t v4)
		{
			hash_ret = mergeRound64(hash_ret, v1);
			hash_ret = mergeRound64(hash_ret, v2);
			hash_ret = mergeRound64(hash_ret, v3);
			hash_ret = mergeRound64(hash_ret, v4);
		}

		template <size_t N>
		static inline hash_t<N> endian_align_sub_ending(hash_t<N> hash_ret, const uint8_t* p, const uint8_t* bEnd)
		{
			if constexpr (N == 32)
			{
				while ((p + 4) <= bEnd)
				{
					hash_ret += readLE<32>(p) * PRIME<32>(3);
					hash_ret = rotl<32>(hash_ret, 17) * PRIME<32>(4);
					p += 4;
				}

				while (p < bEnd)
				{
					hash_ret += (*p) * PRIME<32>(5);
					hash_ret = rotl<32>(hash_ret, 11) * PRIME<32>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 15;
				hash_ret *= PRIME<32>(2);
				hash_ret ^= hash_ret >> 13;
				hash_ret *= PRIME<32>(3);
				hash_ret ^= hash_ret >> 16;

				return hash_ret;
			}
			else
			{
				while (p + 8 <= bEnd)
				{
					const uint64_t k1 = round<64>(0, readLE<64>(p));

					hash_ret ^= k1;
					hash_ret = rotl<64>(hash_ret, 27) * PRIME<64>(1) + PRIME<64>(4);
					p += 8;
				}

				if (p + 4 <= bEnd)
				{
					hash_ret ^= static_cast<uint64_t>(readLE<32>(p))* PRIME<64>(1);
					hash_ret = rotl<64>(hash_ret, 23) * PRIME<64>(2) + PRIME<64>(3);
					p += 4;
				}

				while (p < bEnd)
				{
					hash_ret ^= (*p) * PRIME<64>(5);
					hash_ret = rotl<64>(hash_ret, 11) * PRIME<64>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 33;
				hash_ret *= PRIME<64>(2);
				hash_ret ^= hash_ret >> 29;
				hash_ret *= PRIME<64>(3);
				hash_ret ^= hash_ret >> 32;

				return hash_ret;
			}
		}

		template <size_t N>
		static inline hash_t<N> endian_align(const void* input, size_t len, uint_t<N> seed)
		{
			static_assert(!(N != 32 && N != 64), "You can only call endian_align in 32 or 64 bit mode.");

			const uint8_t* p = static_cast<const uint8_t*>(input);
			const uint8_t* bEnd = p + len;
			hash_t<N> hash_ret;

			if (len >= (N / 2))
			{
				const uint8_t* const limit = bEnd - (N / 2);
				uint_t<N> v1 = seed + PRIME<N>(1) + PRIME<N>(2);
				uint_t<N> v2 = seed + PRIME<N>(2);
				uint_t<N> v3 = seed + 0;
				uint_t<N> v4 = seed - PRIME<N>(1);

				do
				{
					v1 = round<N>(v1, readLE<N>(p)); 
					p += (N / 8);
					v2 = round<N>(v2, readLE<N>(p)); 
					p += (N / 8);
					v3 = round<N>(v3, readLE<N>(p)); 
					p += (N / 8);
					v4 = round<N>(v4, readLE<N>(p)); 
					p += (N / 8);
				} 
				while (p <= limit);

				hash_ret = rotl<N>(v1, 1) + rotl<N>(v2, 7) + rotl<N>(v3, 12) + rotl<N>(v4, 18);

				if constexpr (N == 64)
				{
					endian_align_sub_mergeround(hash_ret, v1, v2, v3, v4);
				}
			}
			else 
			{ 
				hash_ret = seed + PRIME<N>(5); 
			}

			hash_ret += static_cast<hash_t<N>>(len);

			return endian_align_sub_ending<N>(hash_ret, p, bEnd);
		}
	}


	/* *************************************
	*  Algorithm Implementation - xxhash3
	***************************************/

	namespace detail3
	{
		using namespace vec_ops;
		using namespace detail;
		using namespace mem_ops;
		using namespace bit_ops;


		/* *************************************
		*  Enums
		***************************************/

		enum class acc_width : uint8_t { acc_64bits, acc_128bits };
		enum class vec_mode : uint8_t { scalar = 0, sse2 = 1, avx2 = 2 };


		/* *************************************
		*  Constants
		***************************************/

		constexpr uint64_t secret_default_size = 192;
		constexpr uint64_t secret_size_min = 136;
		constexpr uint64_t secret_consume_rate = 8;
		constexpr uint64_t stripe_len = 64;
		constexpr uint64_t acc_nb = 8;
		constexpr uint64_t prefetch_distance = 384;
		constexpr uint64_t secret_lastacc_start = 7;
		constexpr uint64_t secret_mergeaccs_start = 11;
		constexpr uint64_t midsize_max = 240;
		constexpr uint64_t midsize_startoffset = 3;
		constexpr uint64_t midsize_lastoffset = 17;

		constexpr vec_mode vector_mode = static_cast<vec_mode>(intrin::vector_mode);
		constexpr uint64_t acc_align = intrin::acc_align;
		constexpr std::array<uint64_t, 3> vector_bit_width { 64, 128, 256 };

		
		/* *************************************
		*  Defaults
		***************************************/
		
		alignas(64) constexpr uint8_t default_secret[secret_default_size] = {
			0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
			0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
			0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
			0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
			0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
			0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
			0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
			0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
			0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
			0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
			0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
			0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
		};

		constexpr std::array<uint64_t, 8> init_acc = { PRIME<32>(3), PRIME<64>(1), PRIME<64>(2), PRIME<64>(3), PRIME<64>(4), PRIME<32>(2), PRIME<64>(5), PRIME<32>(1) };
	

		/* *************************************
		*  Functions
		***************************************/
	
		static hash_t<64> avalanche(hash_t<64> h64)
		{
			constexpr uint64_t avalanche_mul_prime = 0x165667919E3779F9ULL;

			h64 ^= h64 >> 37;
			h64 *= avalanche_mul_prime;
			h64 ^= h64 >> 32;
			return h64;
		}

		template <vec_mode V>
		XXH_FORCE_INLINE void accumulate_512(void* XXH_RESTRICT acc, const void* XXH_RESTRICT input, const void* XXH_RESTRICT secret, acc_width width)
		{
			constexpr uint64_t bits = vector_bit_width[static_cast<uint8_t>(V)];

			using vec_t = vec_t<bits>;
			
			alignas(sizeof(vec_t)) vec_t* const xacc = static_cast<vec_t*>(acc);
			const vec_t* const xinput = static_cast<const vec_t*>(input);
			const vec_t* const xsecret = static_cast<const vec_t*>(secret);

			for (size_t i = 0; i < stripe_len / sizeof(vec_t); i++)
			{
				vec_t const data_vec = loadu<bits>(xinput + i);
				vec_t const key_vec = loadu<bits>(xsecret + i);
				vec_t const data_key = xorv<bits>(data_vec, key_vec);
				vec_t product = set1<bits>(0);

				if constexpr (V != vec_mode::scalar)
				{
					vec_t const data_key_lo = shuffle<bits, 0, 3, 0, 1>(data_key);

					product = mul<bits>(data_key, data_key_lo);

					if (width == acc_width::acc_128bits)
					{
						vec_t const data_swap = shuffle<bits, 1, 0, 3, 2>(data_vec);
						vec_t const sum = add<bits>(xacc[i], data_swap);

						xacc[i] = add<bits>(sum, product);
					}
					else
					{
						vec_t const sum = add<bits>(xacc[i], data_vec);

						xacc[i] = add<bits>(sum, product);
					}
				}
				else
				{
					product = mul32to64(data_key & 0xFFFFFFFF, data_key >> 32);

					if (width == acc_width::acc_128bits)
					{
						xacc[i ^ 1] = add<bits>(xacc[i ^ 1], data_vec);				
					}
					else
					{
						xacc[i] = add<bits>(xacc[i], data_vec);
					}

					xacc[i] = add<bits>(xacc[i], product);
				}				
			}
		}

		template <vec_mode V>
		XXH_FORCE_INLINE void scramble_acc(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
		{
			constexpr uint64_t bits = vector_bit_width[static_cast<uint8_t>(V)];;

			using vec_t = vec_t<bits>;

			alignas(sizeof(vec_t)) vec_t* const xacc = (vec_t*)acc;
			const vec_t* const xsecret = (const vec_t*)secret;  

			for (size_t i = 0; i < stripe_len / sizeof(vec_t); i++)
			{
				vec_t const acc_vec = xacc[i];
				vec_t const shifted = srli<bits>(acc_vec, 47);
				vec_t const data_vec = xorv<bits>(acc_vec, shifted);
				vec_t const key_vec = loadu<bits>(xsecret + i);
				vec_t const data_key = xorv<bits>(data_vec, key_vec);
				
				if constexpr (V != vec_mode::scalar)
				{
					vec_t const prime32 = set1<bits>(PRIME<32>(1));
					vec_t const data_key_hi = shuffle<bits, 0, 3, 0, 1>(data_key);
					vec_t const prod_lo = mul<bits>(data_key, prime32);
					vec_t const prod_hi = mul<bits>(data_key_hi, prime32);

					xacc[i] = add<bits>(prod_lo, vec_ops::slli<bits>(prod_hi, 32)); 
				}
				else 
				{
					xacc[i] = mul<bits>(data_key, PRIME<32>(1));
				}
			}
		}

		XXH_FORCE_INLINE void accumulate(uint64_t* XXH_RESTRICT acc, const uint8_t* XXH_RESTRICT input, const uint8_t* XXH_RESTRICT secret, size_t nbStripes, acc_width accWidth)
		{
			for (size_t n = 0; n < nbStripes; n++) 
			{
				const uint8_t* const in = input + n * stripe_len;

				intrin::prefetch(in + prefetch_distance);
				accumulate_512<vector_mode>(acc, in, secret + n * secret_consume_rate, accWidth);
			}
		}

		XXH_FORCE_INLINE void hash_long_internal_loop(uint64_t* XXH_RESTRICT acc, const uint8_t* XXH_RESTRICT input, size_t len, const uint8_t* XXH_RESTRICT secret, size_t secretSize, acc_width accWidth)
		{
			size_t const nb_rounds = (secretSize - stripe_len) / secret_consume_rate;
			size_t const block_len = stripe_len * nb_rounds;
			size_t const nb_blocks = len / block_len;

			for (size_t n = 0; n < nb_blocks; n++) 
			{
				accumulate(acc, input + n * block_len, secret, nb_rounds, accWidth);
				scramble_acc<vector_mode>(acc, secret + secretSize - stripe_len);
			}

			/* last partial block */
			size_t const nbStripes = (len - (block_len * nb_blocks)) / stripe_len;

			accumulate(acc, input + nb_blocks * block_len, secret, nbStripes, accWidth);

			/* last stripe */
			if (len & (stripe_len - 1)) 
			{
				const uint8_t* const p = input + len - stripe_len;

				accumulate_512<vector_mode>(acc, p, secret + secretSize - stripe_len - secret_lastacc_start, accWidth);
			}
		}

		XXH_FORCE_INLINE uint64_t mix_2_accs(const uint64_t* XXH_RESTRICT acc, const uint8_t* XXH_RESTRICT secret)
		{
			return mul128fold64(acc[0] ^ readLE<64>(secret), acc[1] ^ readLE<64>(secret + 8));
		}

		XXH_FORCE_INLINE uint64_t merge_accs(const uint64_t* XXH_RESTRICT acc, const uint8_t* XXH_RESTRICT secret, uint64_t start)
		{
			uint64_t result64 = start;

			result64 += mix_2_accs(acc + 0, secret + 0);
			result64 += mix_2_accs(acc + 2, secret + 16);
			result64 += mix_2_accs(acc + 4, secret + 32);
			result64 += mix_2_accs(acc + 6, secret + 48);

			return avalanche(result64);
		}

		XXH_FORCE_INLINE void init_custom_secret(uint8_t* customSecret, uint64_t seed)
		{
			for (uint64_t i = 0; i < secret_default_size / 16; i++) 
			{
				writeLE<64>(customSecret + i * 16, readLE<64>(default_secret + i * 16) + seed);
				writeLE<64>(customSecret + i * 16 + 8, readLE<64>(default_secret + i * 16 + 8) - seed);
			}
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> len_1to3(const uint8_t* input, size_t len, const uint8_t* secret, uint64_t seed)
		{
			if constexpr (N == 64)
			{
				uint8_t const c1 = input[0];
				uint8_t const c2 = input[len >> 1];
				uint8_t const c3 = input[len - 1];
				uint32_t const combined = ((uint32_t)c1 << 16) | (((uint32_t)c2) << 24) | (((uint32_t)c3) << 0) | (((uint32_t)len) << 8);
				uint64_t const bitflip = (readLE<32>(secret) ^ readLE<32>(secret + 4)) + seed;
				uint64_t const keyed = (uint64_t)combined ^ bitflip;
				uint64_t const mixed = keyed * PRIME<64>(1);

				return avalanche(mixed);
			}
			else
			{
				uint8_t const c1 = input[0];
				uint8_t const c2 = input[len >> 1];
				uint8_t const c3 = input[len - 1];
				uint32_t const combinedl = ((uint32_t)c1 << 16) + (((uint32_t)c2) << 24) + (((uint32_t)c3) << 0) + (((uint32_t)len) << 8);
				uint32_t const combinedh = rotl<32>(swap<32>(combinedl), 13);
				uint64_t const bitflipl = (readLE<32>(secret) ^ readLE<32>(secret + 4)) + seed;
				uint64_t const bitfliph = (readLE<32>(secret + 8) ^ readLE<32>(secret + 12)) - seed;
				uint64_t const keyed_lo = (uint64_t)combinedl ^ bitflipl;
				uint64_t const keyed_hi = (uint64_t)combinedh ^ bitfliph;
				uint64_t const mixedl = keyed_lo * PRIME<64>(1);
				uint64_t const mixedh = keyed_hi * PRIME<64>(5);
				hash128_t const h128 = {avalanche(mixedl), avalanche(mixedh)};

				return h128;
			}
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> len_4to8(const uint8_t* input, size_t len, const uint8_t* secret, uint64_t seed)
		{
			constexpr uint64_t mix_constant = 0x9FB21C651E98DF25ULL;

			seed ^= (uint64_t)swap<32>((uint32_t)seed) << 32;

			if constexpr (N == 64)
			{		
				uint32_t const input1 = readLE<32>(input);
				uint32_t const input2 = readLE<32>(input + len - 4);
				uint64_t const bitflip = (readLE<64>(secret + 8) ^ readLE<64>(secret + 16)) - seed;
				uint64_t const input64 = input2 + ((uint64_t)input1 << 32);
				uint64_t x = input64 ^ bitflip;

				x ^= rotl<64>(x, 49) ^ rotl<64>(x, 24);
				x *= mix_constant;
				x ^= (x >> 35) + len;
				x *= mix_constant;

				return (x ^ (x >> 28));
			}
			else
			{
				uint32_t const input_lo = readLE<32>(input);
				uint32_t const input_hi = readLE<32>(input + len - 4);
				uint64_t const input_64 = input_lo + ((uint64_t)input_hi << 32);
				uint64_t const bitflip = (readLE<64>(secret + 16) ^ readLE<64>(secret + 24)) + seed;
				uint64_t const keyed = input_64 ^ bitflip;
				uint128_t m128 = mul64to128(keyed, PRIME<64>(1) + (len << 2));

				m128.high64 += (m128.low64 << 1);
				m128.low64 ^= (m128.high64 >> 3);
				m128.low64 ^= (m128.low64 >> 35);
				m128.low64 *= mix_constant;
				m128.low64 ^= (m128.low64 >> 28);
				m128.high64 = avalanche(m128.high64);

				return m128;		
			}
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> len_9to16(const uint8_t* input, size_t len, const uint8_t* secret, uint64_t seed)
		{
			if constexpr (N == 64)
			{
				uint64_t const bitflip1 = (readLE<64>(secret + 24) ^ readLE<64>(secret + 32)) + seed;
				uint64_t const bitflip2 = (readLE<64>(secret + 40) ^ readLE<64>(secret + 48)) - seed;
				uint64_t const input_lo = readLE<64>(input) ^ bitflip1;
				uint64_t const input_hi = readLE<64>(input + len - 8) ^ bitflip2;
				uint64_t const acc = len + swap<64>(input_lo) + input_hi + mul128fold64(input_lo, input_hi);

				return avalanche(acc);
			}
			else
			{
				uint64_t const bitflipl = (readLE<64>(secret + 32) ^ readLE<64>(secret + 40)) - seed;
				uint64_t const bitfliph = (readLE<64>(secret + 48) ^ readLE<64>(secret + 56)) + seed;
				uint64_t const input_lo = readLE<64>(input);
				uint64_t input_hi = readLE<64>(input + len - 8);
				uint128_t m128 = mul64to128(input_lo ^ input_hi ^ bitflipl, PRIME<64>(1));

				m128.low64 += (uint64_t)(len - 1) << 54;
				input_hi ^= bitfliph;

				if constexpr (sizeof(void*) < sizeof(uint64_t)) // 32-bit version
				{
					m128.high64 += (input_hi & 0xFFFFFFFF00000000) + mul32to64((uint32_t)input_hi, PRIME<32>(2));
				}
				else
				{
					m128.high64 += input_hi + mul32to64((uint32_t)input_hi, PRIME<32>(2) - 1);
				}

				m128.low64 ^= swap<64>(m128.high64);

				hash128_t h128 = mul64to128(m128.low64, PRIME<64>(2));

				h128.high64 += m128.high64 * PRIME<64>(2);
				h128.low64 = avalanche(h128.low64);
				h128.high64 = avalanche(h128.high64);

				return h128;
			}
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> len_0to16(const uint8_t* input, size_t len, const uint8_t* secret, uint64_t seed)
		{
			if (XXH_likely(len > 8))
			{
				return len_9to16<N>(input, len, secret, seed);
			}
			else if (XXH_likely(len >= 4))
			{
				return len_4to8<N>(input, len, secret, seed);
			}
			else if (len)
			{
				return len_1to3<N>(input, len, secret, seed);
			}
			else
			{
				if constexpr (N == 64)
				{
					return avalanche((PRIME<64>(1) + seed) ^ (readLE<64>(secret + 56) ^ readLE<64>(secret + 64)));
				}
				else
				{
					uint64_t const bitflipl = readLE<64>(secret + 64) ^ readLE<64>(secret + 72);
					uint64_t const bitfliph = readLE<64>(secret + 80) ^ readLE<64>(secret + 88);

					return hash128_t(avalanche((PRIME<64>(1) + seed) ^ bitflipl), avalanche((PRIME<64>(2) - seed) ^ bitfliph));
				}			
			}
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> hash_long_internal(const uint8_t* XXH_RESTRICT input, size_t len, const uint8_t* XXH_RESTRICT secret = default_secret, size_t secretSize = sizeof(default_secret))
		{
			alignas(acc_align) std::array<uint64_t, acc_nb> acc = init_acc;

			if constexpr (N == 64)
			{				
				hash_long_internal_loop(acc.data(), input, len, secret, secretSize, acc_width::acc_64bits);

				/* converge into final hash */
				return merge_accs(acc.data(), secret + secret_mergeaccs_start, (uint64_t)len * PRIME<64>(1));
			}
			else
			{
				hash_long_internal_loop(acc.data(), input, len, secret, secretSize, acc_width::acc_128bits);

				/* converge into final hash */
				uint64_t const low64 = merge_accs(acc.data(), secret + secret_mergeaccs_start, (uint64_t)len * PRIME<64>(1));
				uint64_t const high64 = merge_accs(acc.data(), secret + secretSize - sizeof(acc) - secret_mergeaccs_start, ~((uint64_t)len * PRIME<64>(2)));

				return hash128_t(low64, high64);
			}
		}

		XXH_FORCE_INLINE uint64_t mix_16b(const uint8_t* XXH_RESTRICT input, const uint8_t* XXH_RESTRICT secret, uint64_t seed)
		{
			uint64_t const input_lo = readLE<64>(input);
			uint64_t const input_hi = readLE<64>(input + 8);

			return mul128fold64(input_lo ^ (readLE<64>(secret) + seed), input_hi ^ (readLE<64>(secret + 8) - seed));
		}

		XXH_FORCE_INLINE uint128_t mix_32b(uint128_t acc, const uint8_t* input1, const uint8_t* input2, const uint8_t* secret, uint64_t seed)
		{
			acc.low64 += mix_16b(input1, secret + 0, seed);
			acc.low64 ^= readLE<64>(input2) + readLE<64>(input2 + 8);
			acc.high64 += mix_16b(input2, secret + 16, seed);
			acc.high64 ^= readLE<64>(input1) + readLE<64>(input1 + 8);

			return acc;	
		}

		template <size_t N>
		XXH_FORCE_INLINE hash_t<N> len_17to128(const uint8_t* XXH_RESTRICT input, size_t len, const uint8_t* XXH_RESTRICT secret, uint64_t seed)
		{
			if constexpr (N == 64)
			{
				hash64_t acc = len * PRIME<64>(1);

				if (len > 32) 
				{
					if (len > 64) 
					{
						if (len > 96) 
						{
							acc += mix_16b(input + 48, secret + 96, seed);
							acc += mix_16b(input + len - 64, secret + 112, seed);
						}

						acc += mix_16b(input + 32, secret + 64, seed);
						acc += mix_16b(input + len - 48, secret + 80, seed);
					}

					acc += mix_16b(input + 16, secret + 32, seed);
					acc += mix_16b(input + len - 32, secret + 48, seed);
				}

				acc += mix_16b(input + 0, secret + 0, seed);
				acc += mix_16b(input + len - 16, secret + 16, seed);

				return avalanche(acc);
			}
			else
			{
				hash128_t acc = { len * PRIME<64>(1), 0 };

				if (len > 32) 
				{
					if (len > 64) 
					{
						if (len > 96) 
						{
							acc = mix_32b(acc, input + 48, input + len - 64, secret + 96, seed);
						}

						acc = mix_32b(acc, input + 32, input + len - 48, secret + 64, seed);
					}

					acc = mix_32b(acc, input + 16, input + len - 32, secret + 32, seed);
				}

				acc = mix_32b(acc, input, input + len - 16, secret, seed);

				uint64_t const low64 = acc.low64 + acc.high64;
				uint64_t const high64 = (acc.low64 * PRIME<64>(1)) + (acc.high64 * PRIME<64>(4)) + ((len - seed) * PRIME<64>(2));

				return { avalanche(low64), (uint64_t)0 - avalanche(high64) };
			}
		}

		template <size_t N>
		XXH_NO_INLINE hash_t<N> len_129to240(const uint8_t* XXH_RESTRICT input, size_t len, const uint8_t* XXH_RESTRICT secret, uint64_t seed)
		{
			if constexpr (N == 64)
			{
				uint64_t acc = len * PRIME<64>(1);
				size_t const nbRounds = len / 16;

				for (size_t i = 0; i < 8; i++) 
				{
					acc += mix_16b(input + (i * 16), secret + (i * 16), seed);
				}

				acc = avalanche(acc);

				for (size_t i = 8; i < nbRounds; i++) 
				{
					acc += mix_16b(input + (i * 16), secret + ((i - 8) * 16) + midsize_startoffset, seed);
				}

				/* last bytes */
				acc += mix_16b(input + len - 16, secret + secret_size_min - midsize_lastoffset, seed);

				return avalanche(acc);
			}
			else
			{
				hash128_t acc;
				uint64_t const nbRounds = len / 32;

				acc.low64 = len * PRIME<64>(1);
				acc.high64 = 0;

				for (size_t i = 0; i < 4; i++) 
				{
					acc = mix_32b(acc, input + (i * 32), input + (i * 32) + 16, secret + (i * 32), seed);
				}

				acc.low64 = avalanche(acc.low64);
				acc.high64 = avalanche(acc.high64);

				for (size_t i = 4; i < nbRounds; i++) 
				{
					acc = mix_32b(acc, input + (i * 32), input + (i * 32) + 16, secret + midsize_startoffset + ((i - 4) * 32), seed);
				}

				/* last bytes */
				acc = mix_32b(acc, input + len - 16, input + len - 32, secret + secret_size_min - midsize_lastoffset - 16, 0ULL - seed);

				uint64_t const low64 = acc.low64 + acc.high64;
				uint64_t const high64 = (acc.low64 * PRIME<64>(1)) + (acc.high64 * PRIME<64>(4)) + ((len - seed) * PRIME<64>(2));

				return { avalanche(low64), (uint64_t)0 - avalanche(high64) };
			}

		}

		template <size_t N>
		XXH_NO_INLINE hash_t<N> xxhash3_impl(const void* XXH_RESTRICT input, size_t len, hash64_t seed, const void* XXH_RESTRICT secret = default_secret, size_t secretSize = secret_default_size)
		{
			alignas(8) uint8_t custom_secret[secret_default_size];
			const void* short_secret = secret;
		
			if (seed != 0)
			{
				init_custom_secret(custom_secret, seed);
				secret = custom_secret;
				secretSize = secret_default_size;
				short_secret = default_secret;
			}

			if (len <= 16)
			{
				return len_0to16<N>(static_cast<const uint8_t*>(input), len, static_cast<const uint8_t*>(short_secret), seed);
			}
			else if (len <= 128)
			{
				return len_17to128<N>(static_cast<const uint8_t*>(input), len, static_cast<const uint8_t*>(short_secret), seed);
			}
			else if (len <= midsize_max)
			{
				return len_129to240<N>(static_cast<const uint8_t*>(input), len, static_cast<const uint8_t*>(short_secret), seed);
			}
			else
			{
				return hash_long_internal<N>(static_cast<const uint8_t*>(input), len, static_cast<const uint8_t*>(secret), secretSize);
			}
		}
	}


	/* *************************************
	*  Public Access Point - xxhash
	***************************************/

	template <size_t bit_mode>
	inline hash_t<bit_mode> xxhash(const void* input, size_t len, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		return detail::endian_align<bit_mode>(input, len, seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash(const std::basic_string<T>& input, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		return detail::endian_align<bit_mode>(static_cast<const void*>(input.data()), input.length() * sizeof(T), seed);
	}

	template <size_t bit_mode, typename ContiguousIterator>
	inline hash_t<bit_mode> xxhash(ContiguousIterator begin, ContiguousIterator end, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		using T = typename std::decay_t<decltype(*end)>;
		return detail::endian_align<bit_mode>(static_cast<const void*>(&*begin), (end - begin) * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash(const std::vector<T>& input, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		return detail::endian_align<bit_mode>(static_cast<const void*>(input.data()), input.size() * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T, size_t AN>
	inline hash_t<bit_mode> xxhash(const std::array<T, AN>& input, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		return detail::endian_align<bit_mode>(static_cast<const void*>(input.data()), AN * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash(const std::initializer_list<T>& input, uint_t<bit_mode> seed = 0)
	{
		static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash can only be used in 32 and 64 bit modes.");
		return detail::endian_align<bit_mode>(static_cast<const void*>(input.begin()), input.size() * sizeof(T), seed);
	}


	/* *************************************
	*  Public Access Point - xxhash3
	***************************************/

	template <size_t bit_mode>
	inline hash_t<bit_mode> xxhash3(const void* input, size_t len, uint64_t seed = 0)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(input, len, seed);
	}

	template <size_t bit_mode>
	inline hash_t<bit_mode> xxhash3(const void* input, size_t len, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(input, len, 0, secret, secretSize);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::basic_string<T>& input, uint64_t seed = 0)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), input.length() * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::basic_string<T>& input, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), input.length() * sizeof(T), 0, secret, secretSize);
	}

	template <size_t N, typename ContiguousIterator>
	inline hash_t<N> xxhash3(ContiguousIterator begin, ContiguousIterator end, uint64_t seed = 0)
	{
		static_assert(!(N != 128 && N != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		using T = typename std::decay_t<decltype(*end)>;
		return detail3::xxhash3_impl<N>(static_cast<const void*>(&*begin), (end - begin) * sizeof(T), seed);
	}

	template <size_t bit_mode, typename ContiguousIterator>
	inline hash_t<bit_mode> xxhash3(ContiguousIterator begin, ContiguousIterator end, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		using T = typename std::decay_t<decltype(*end)>;
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(&*begin), (end - begin) * sizeof(T), 0, secret, secretSize);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::vector<T>& input, uint64_t seed = 0)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), input.size() * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::vector<T>& input, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), input.size() * sizeof(T), 0, secret, secretSize);
	}

	template <size_t bit_mode, typename T, size_t AN>
	inline hash_t<bit_mode> xxhash3(const std::array<T, AN>& input, uint64_t seed = 0)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), AN * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T, size_t AN>
	inline hash_t<bit_mode> xxhash3(const std::array<T, AN>& input, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.data()), AN * sizeof(T), 0, secret, secretSize);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::initializer_list<T>& input, uint64_t seed = 0)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.begin()), input.size() * sizeof(T), seed);
	}

	template <size_t bit_mode, typename T>
	inline hash_t<bit_mode> xxhash3(const std::initializer_list<T>& input, const void* secret, size_t secretSize)
	{
		static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 can only be used in 64 and 128 bit modes.");
		return detail3::xxhash3_impl<bit_mode>(static_cast<const void*>(input.begin()), input.size() * sizeof(T), 0, secret, secretSize);
	}


	/* *************************************
	*  Hash streaming - xxhash
	***************************************/

	template <size_t bit_mode>
	class hash_state_t 
	{
		uint64_t total_len = 0;
		uint_t<bit_mode> v1 = 0, v2 = 0, v3 = 0, v4 = 0;
		std::array<hash_t<bit_mode>, 4> mem = {0, 0, 0, 0};
		uint32_t memsize = 0;

		inline void update_impl(const void* input, size_t length)
		{
			const uint8_t* p = reinterpret_cast<const uint8_t*>(input);
			const uint8_t* const bEnd = p + length;

			total_len += length;

			if (memsize + length < (bit_mode / 2))
			{   /* fill in tmp buffer */
				memcpy(reinterpret_cast<uint8_t*>(mem.data()) + memsize, input, length);
				memsize += static_cast<uint32_t>(length);
				return;
			}

			if (memsize > 0)
			{   /* some data left from previous update */
				memcpy(reinterpret_cast<uint8_t*>(mem.data()) + memsize, input, (bit_mode / 2) - memsize);

				const uint_t<bit_mode>* ptr = mem.data();

				v1 = detail::round<bit_mode>(v1, mem_ops::readLE<bit_mode>(ptr)); 
				ptr++;
				v2 = detail::round<bit_mode>(v2, mem_ops::readLE<bit_mode>(ptr)); 
				ptr++;
				v3 = detail::round<bit_mode>(v3, mem_ops::readLE<bit_mode>(ptr)); 
				ptr++;
				v4 = detail::round<bit_mode>(v4, mem_ops::readLE<bit_mode>(ptr));

				p += (bit_mode / 2) - memsize;
				memsize = 0;
			}

			if (p <= bEnd - (bit_mode / 2))
			{
				const uint8_t* const limit = bEnd - (bit_mode / 2);

				do
				{
					v1 = detail::round<bit_mode>(v1, mem_ops::readLE<bit_mode>(p)); 
					p += (bit_mode / 8);
					v2 = detail::round<bit_mode>(v2, mem_ops::readLE<bit_mode>(p)); 
					p += (bit_mode / 8);
					v3 = detail::round<bit_mode>(v3, mem_ops::readLE<bit_mode>(p)); 
					p += (bit_mode / 8);
					v4 = detail::round<bit_mode>(v4, mem_ops::readLE<bit_mode>(p)); 
					p += (bit_mode / 8);
				} 
				while (p <= limit);
			}

			if (p < bEnd)
			{
				memcpy(mem.data(), p, static_cast<size_t>(bEnd - p));
				memsize = static_cast<uint32_t>(bEnd - p);
			}
		}

		inline hash_t<bit_mode> digest_impl() const
		{
			const uint8_t* p = reinterpret_cast<const uint8_t*>(mem.data());
			const uint8_t* const bEnd = reinterpret_cast<const uint8_t*>(mem.data()) + memsize;
			hash_t<bit_mode> hash_ret;

			if (total_len >= (bit_mode / 2))
			{
				hash_ret = bit_ops::rotl<bit_mode>(v1, 1) + bit_ops::rotl<bit_mode>(v2, 7) + bit_ops::rotl<bit_mode>(v3, 12) + bit_ops::rotl<bit_mode>(v4, 18);

				if constexpr (bit_mode == 64)
				{
					detail::endian_align_sub_mergeround(hash_ret, v1, v2, v3, v4);
				}
			}
			else 
			{ 
				hash_ret = v3 + detail::PRIME<bit_mode>(5); 
			}

			hash_ret += static_cast<hash_t<bit_mode>>(total_len);

			return detail::endian_align_sub_ending<bit_mode>(hash_ret, p, bEnd);
		}

	public:

		hash_state_t(uint_t<bit_mode> seed = 0)
		{
			static_assert(!(bit_mode != 32 && bit_mode != 64), "xxhash streaming can only be used in 32 and 64 bit modes.");
			v1 = seed + detail::PRIME<bit_mode>(1) + detail::PRIME<bit_mode>(2);
			v2 = seed + detail::PRIME<bit_mode>(2);
			v3 = seed + 0;
			v4 = seed - detail::PRIME<bit_mode>(1);
		};

		hash_state_t operator=(hash_state_t<bit_mode>& other)
		{
			memcpy(this, &other, sizeof(hash_state_t<bit_mode>));
		}

		void reset(uint_t<bit_mode> seed = 0)
		{
			memset(this, 0, sizeof(hash_state_t<bit_mode>));
			v1 = seed + detail::PRIME<bit_mode>(1) + detail::PRIME<bit_mode>(2);
			v2 = seed + detail::PRIME<bit_mode>(2);
			v3 = seed + 0;
			v4 = seed - detail::PRIME<bit_mode>(1);
		}

		void update(const void* input, size_t length)
		{
			return update_impl(input, length);
		}

		template <typename T>
		void update(const std::basic_string<T>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), input.length() * sizeof(T));
		}

		template <typename ContiguousIterator>
		void update(ContiguousIterator begin, ContiguousIterator end)
		{
			using T = typename std::decay_t<decltype(*end)>;
			return update_impl(static_cast<const void*>(&*begin), (end - begin) * sizeof(T));
		}

		template <typename T>
		void update(const std::vector<T>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), input.size() * sizeof(T));
		}

		template <typename T, size_t AN>
		void update(const std::array<T, AN>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), AN * sizeof(T));
		}

		template <typename T>
		void update(const std::initializer_list<T>& input)
		{
			return update_impl(static_cast<const void*>(input.begin()), input.size() * sizeof(T));
		}

		hash_t<bit_mode> digest() const
		{
			return digest_impl();
		}
	};

	using hash_state32_t = hash_state_t<32>;
	using hash_state64_t = hash_state_t<64>;


	/* *************************************
	*  Hash streaming - xxhash3
	***************************************/

	template <size_t bit_mode>
	class alignas(64) hash3_state_t 
	{   
		constexpr static int internal_buffer_size = 256;
		constexpr static int internal_buffer_stripes = (internal_buffer_size / detail3::stripe_len);
		constexpr static detail3::acc_width accWidth = (bit_mode == 64) ? detail3::acc_width::acc_64bits : detail3::acc_width::acc_128bits;
	
		alignas(64) uint64_t acc[8];
		alignas(64) uint8_t customSecret[detail3::secret_default_size];  /* used to store a custom secret generated from the seed. Makes state larger. Design might change */
		alignas(64) uint8_t buffer[internal_buffer_size];
		uint32_t bufferedSize = 0;
		uint32_t nbStripesPerBlock = 0;
		uint32_t nbStripesSoFar = 0;
		uint32_t secretLimit = 0;
		uint32_t reserved32 = 0;
		uint32_t reserved32_2 = 0;
		uint64_t totalLen = 0;
		uint64_t seed = 0;
		uint64_t reserved64 = 0;
		const uint8_t* secret = nullptr;    /* note : there is some padding after, due to alignment on 64 bytes */


		void consume_stripes(uint64_t* acc, uint32_t& nbStripesSoFar, size_t totalStripes, const uint8_t* input, detail3::acc_width accWidth)
		{
			if (nbStripesPerBlock - nbStripesSoFar <= totalStripes) /* need a scrambling operation */
			{			
				size_t const nbStripes = nbStripesPerBlock - nbStripesSoFar;

				detail3::accumulate(acc, input, secret + (nbStripesSoFar * detail3::secret_consume_rate), nbStripes, accWidth);
				detail3::scramble_acc<detail3::vector_mode>(acc, secret + secretLimit);
				detail3::accumulate(acc, input + nbStripes * detail3::stripe_len, secret, totalStripes - nbStripes, accWidth);
				nbStripesSoFar = (uint32_t)(totalStripes - nbStripes);
			}
			else 
			{
				detail3::accumulate(acc, input, secret + (nbStripesSoFar * detail3::secret_consume_rate), totalStripes, accWidth);
				nbStripesSoFar += (uint32_t)totalStripes;
			}
		}

		void update_impl(const void* input_, size_t len)
		{
			const uint8_t* input = static_cast<const uint8_t*>(input_);
			const uint8_t* const bEnd = input + len;

			totalLen += len;

			if (bufferedSize + len <= internal_buffer_size) 
			{  /* fill in tmp buffer */
				memcpy(buffer + bufferedSize, input, len);
				bufferedSize += (uint32_t)len;
				return;
			}
			/* input now > XXH3_INTERNALBUFFER_SIZE */

			if (bufferedSize > 0) 
			{   /* some input within internal buffer: fill then consume it */
				size_t const loadSize = internal_buffer_size - bufferedSize;

				memcpy(buffer + bufferedSize, input, loadSize);
				input += loadSize;
				consume_stripes(acc, nbStripesSoFar, internal_buffer_stripes, buffer, accWidth);
				bufferedSize = 0;
			}

			/* consume input by full buffer quantities */
			if (input + internal_buffer_size <= bEnd) 
			{
				const uint8_t* const limit = bEnd - internal_buffer_size;

				do 
				{
					consume_stripes(acc, nbStripesSoFar, internal_buffer_stripes, input, accWidth);
					input += internal_buffer_size;
				} 
				while (input <= limit);
			}

			if (input < bEnd) 
			{ /* some remaining input input : buffer it */
				memcpy(buffer, input, (size_t)(bEnd - input));
				bufferedSize = (uint32_t)(bEnd - input);
			}
		}

		void digest_long(uint64_t* acc_, detail3::acc_width accWidth)
		{
			memcpy(acc_, acc, sizeof(acc));  /* digest locally, state remains unaltered, and can continue ingesting more input afterwards */

			if (bufferedSize >= detail3::stripe_len) 
			{
				size_t const totalNbStripes = bufferedSize / detail3::stripe_len;
				uint32_t nbStripesSoFar = this->nbStripesSoFar;

				consume_stripes(acc_, nbStripesSoFar, totalNbStripes, buffer, accWidth);

				if (bufferedSize % detail3::stripe_len) 
				{  /* one last partial stripe */
					detail3::accumulate_512<detail3::vector_mode>(acc_, buffer + bufferedSize - detail3::stripe_len, secret + secretLimit - detail3::secret_lastacc_start, accWidth);
				}
			}
			else 
			{  /* bufferedSize < STRIPE_LEN */
				if (bufferedSize > 0) 
				{ /* one last stripe */
					uint8_t lastStripe[detail3::stripe_len];
					size_t const catchupSize = detail3::stripe_len - bufferedSize;
					memcpy(lastStripe, buffer + sizeof(buffer) - catchupSize, catchupSize);
					memcpy(lastStripe + catchupSize, buffer, bufferedSize);
					detail3::accumulate_512<detail3::vector_mode>(acc_, lastStripe, secret + secretLimit - detail3::secret_lastacc_start, accWidth);
				}
			}
		}

	public:

		hash3_state_t operator=(hash3_state_t& other)
		{
			memcpy(this, &other, sizeof(hash3_state_t));
		}

		hash3_state_t(uint64_t seed = 0)
		{
			static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 streaming can only be used in 64 and 128 bit modes.");
			reset(seed);
		}

		hash3_state_t(const void* secret, size_t secretSize)
		{
			static_assert(!(bit_mode != 128 && bit_mode != 64), "xxhash3 streaming can only be used in 64 and 128 bit modes.");
			reset(secret, secretSize);
		}

		void reset(uint64_t seed = 0)
		{ 
			memset(this, 0, sizeof(*this));
			memcpy(acc, detail3::init_acc.data(), sizeof(detail3::init_acc));
			(*this).seed = seed;

			if (seed == 0)
			{
				secret = detail3::default_secret;
			}
			else
			{
				detail3::init_custom_secret(customSecret, seed);
				secret = customSecret;
			}

			secretLimit = (uint32_t)(detail3::secret_default_size - detail3::stripe_len);
			nbStripesPerBlock = secretLimit / detail3::secret_consume_rate;
		}

		void reset(const void* secret, size_t secretSize)
		{
			memset(this, 0, sizeof(*this));
			memcpy(acc, detail3::init_acc.data(), sizeof(detail3::init_acc));
			seed = 0;

			(*this).secret = (const uint8_t*)secret;
			secretLimit = (uint32_t)(secretSize - detail3::stripe_len);
			nbStripesPerBlock = secretLimit / detail3::secret_consume_rate;
		}

		void update(const void* input, size_t len)
		{
			return update_impl(static_cast<const void*>(input), len);
		}

		template <typename T>
		void update(const std::basic_string<T>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), input.length() * sizeof(T));
		}

		template <typename ContiguousIterator>
		void update(ContiguousIterator begin, ContiguousIterator end)
		{
			using T = typename std::decay_t<decltype(*end)>;
			return update_impl(static_cast<const void*>(&*begin), (end - begin) * sizeof(T));
		}

		template <typename T>
		void update(const std::vector<T>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), input.size() * sizeof(T));
		}

		template <typename T, size_t AN>
		void update(const std::array<T, AN>& input)
		{
			return update_impl(static_cast<const void*>(input.data()), AN * sizeof(T));
		}

		template <typename T>
		void update(const std::initializer_list<T>& input)
		{
			return update_impl(static_cast<const void*>(input.begin()), input.size() * sizeof(T));
		}

		hash_t<bit_mode> digest()
		{	
			if (totalLen > detail3::midsize_max) 
			{
				alignas(detail3::acc_align) hash64_t acc[detail3::acc_nb];
				
				digest_long(acc, accWidth);

				if constexpr (bit_mode == 64)
				{
					return detail3::merge_accs(acc, secret + detail3::secret_mergeaccs_start, (uint64_t)totalLen * detail::PRIME<64>(1));
				}
				else
				{
					uint64_t const low64 = detail3::merge_accs(acc, secret + detail3::secret_mergeaccs_start, (uint64_t)totalLen * detail::PRIME<64>(1));
					uint64_t const high64 = detail3::merge_accs(acc, secret + secretLimit + detail3::stripe_len - sizeof(acc) - detail3::secret_mergeaccs_start, ~((uint64_t)totalLen * detail::PRIME<64>(2)));

					return { low64, high64 };
				}
			}
			else
			{
				return detail3::xxhash3_impl<bit_mode>(buffer, totalLen, seed, secret, secretLimit + detail3::stripe_len);
			}
		}
	};

	using hash3_state64_t = hash3_state_t<64>;
	using hash3_state128_t = hash3_state_t<128>;


	/* *************************************
	*  Canonical represenation
	***************************************/

	template <size_t bit_mode>
	struct canonical_t
	{
		std::array<uint8_t, bit_mode / 8> digest {0};

		canonical_t(hash_t<bit_mode> hash)
		{
			if constexpr (bit_mode < 128)
			{
				if (mem_ops::is_little_endian()) 
				{ 
					hash = bit_ops::swap<bit_mode>(hash); 
				}

				memcpy(digest.data(), &hash, sizeof(canonical_t<bit_mode>));
			}
			else
			{
				if (mem_ops::is_little_endian()) 
				{ 
					hash.low64 = bit_ops::swap<64>(hash.low64); 
					hash.high64 = bit_ops::swap<64>(hash.high64);
				}

				memcpy(digest.data(), &hash.high64, sizeof(hash.high64));
				memcpy(digest.data() + sizeof(hash.high64), &hash.low64, sizeof(hash.low64));
			}
		}

		hash_t<bit_mode> get_hash() const
		{
			if constexpr (bit_mode < 128)
			{
				return mem_ops::readBE<bit_mode>(&digest);
			}
			else
			{
				return { mem_ops::readBE<64>(&digest[8]), mem_ops::readBE<64>(&digest) };
			}
		}
	};

	using canonical32_t = canonical_t<32>;
	using canonical64_t = canonical_t<64>;
	using canonical128_t = canonical_t<128>;
}
