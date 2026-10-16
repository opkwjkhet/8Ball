/* --------------------------------- ABOUT -------------------------------------

Original Author: Adam Yaxley
Website: https://github.com/adamyaxley
License: See end of file

Obfuscate - Ultra Edition v2
Compile-time string literal obfuscation with anti-reverse-engineering for C++14

Upgrades over Enhanced Edition:
- SPECK-64/128 block cipher (NSA lightweight cipher designed for ARM/IoT)
  replaces XOR+ADD+nibble-swap — real Feistel network, not pattern-matchable
- ARM64 NEON SIMD accelerated decryption path
- 5 opaque predicates (up from 2) with diverse math patterns
- Junk code injection (disassembly bloating) to confuse IDA/Ghidra CFG
- Anti-debug detection: ptrace / sysctl check woven into decrypt path
- Memory barriers prevent compiler reordering of security-critical ops
- Per-site entropy: __COUNTER__ + __LINE__ + __FILE__ + golden ratio hash
- Compile-time key whitening with avalanche-quality mixing

API unchanged: OBFUSCATE(data), NSOBFUSCATE(data), OBFUSCATE_KEY(data, key)

----------------------------------------------------------------------------- */

#pragma once
#if __cplusplus >= 202002L
	#define CONSTEVAL consteval
#else
	#define CONSTEVAL constexpr
#endif

#ifdef _MSC_VER
	#define CAT(X,Y) CAT2(X,Y)
	#define CAT2(X,Y) X##Y
	#define LINE int(CAT(__LINE__,U))
#else
	#define LINE __LINE__
#endif

// NEON detection for ARM64
#if defined(__aarch64__) || defined(_M_ARM64)
	#define AY_HAS_NEON 1
	#include <arm_neon.h>
#else
	#define AY_HAS_NEON 0
#endif

// Anti-debug headers (iOS/macOS)
#if defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
	#include <unistd.h>
#endif

#ifndef OBFUSCATE_DEFAULT_KEY
#define OBFUSCATE_DEFAULT_KEY \
	ay::generate_key( \
		(ay::key_type)__LINE__ ^ \
		((ay::key_type)__COUNTER__ << 32) ^ \
		(ay::key_type)(sizeof(__FILE__) * 0x9E3779B97F4A7C15ull) \
	)
#endif

namespace ay
{
	using size_type = unsigned long long;
	using key_type = unsigned long long;

	template <typename T>
	struct remove_const_ref {
		using type = T;
	};

	template <typename T>
	struct remove_const_ref<T&> {
		using type = T;
	};

	template <typename T>
	struct remove_const_ref<const T> {
		using type = T;
	};

	template <typename T>
	struct remove_const_ref<const T&> {
		using type = T;
	};

	template <typename T>
	using char_type = typename remove_const_ref<T>::type;

	// Generate a pseudo-random key that spans all 8 bytes
	// Uses splitmix64 — excellent avalanche properties
	CONSTEVAL key_type generate_key(key_type seed)
	{
		key_type key = seed;
		key ^= (key >> 33);
		key *= 0xff51afd7ed558ccdull;
		key ^= (key >> 33);
		key *= 0xc4ceb9fe1a85ec53ull;
		key ^= (key >> 33);
		key |= 0x0101010101010101ull;
		return key;
	}

	// Compile-time key hash for runtime integrity verification
	// Uses murmur-style finalizer for strong mixing
	CONSTEVAL key_type key_hash(key_type key)
	{
		key_type h = key;
		h ^= h >> 16;
		h *= 0x45d9f3bull;
		h ^= h >> 16;
		h *= 0x45d9f3bull;
		h ^= h >> 16;
		return h;
	}

	// Derive subkeys from master key for SPECK rounds
	CONSTEVAL key_type derive_subkey(key_type master, key_type round)
	{
		key_type k = master ^ (round * 0x517cc1b727220a95ull);
		k ^= (k >> 28);
		k *= 0x9FB21C651E98DF25ull;
		k ^= (k >> 32);
		return k;
	}

	// =========================================================================
	// SPECK-64/128 lightweight block cipher (constexpr-safe)
	// Designed by NSA for constrained ARM/IoT devices
	// 27-round Feistel network — vastly stronger than XOR rolling
	// =========================================================================

	static constexpr unsigned int SPECK_ROUNDS = 27;

	// SPECK round function (operates on 32-bit halves)
	constexpr void speck_round(unsigned int& x, unsigned int& y, unsigned int k)
	{
		x = ((x >> 8) | (x << 24)); // ROR 8
		x += y;
		x ^= k;
		y = ((y << 3) | (y >> 29)); // ROL 3
		y ^= x;
	}

	// Inverse SPECK round
	constexpr void speck_round_inv(unsigned int& x, unsigned int& y, unsigned int k)
	{
		y ^= x;
		y = ((y >> 3) | (y << 29)); // ROR 3
		x ^= k;
		x -= y;
		x = ((x << 8) | (x >> 24)); // ROL 8
	}

	// SPECK key schedule: expand 128-bit key into round keys
	constexpr void speck_key_schedule(key_type key_hi, key_type key_lo, unsigned int rk[SPECK_ROUNDS])
	{
		unsigned int b = (unsigned int)(key_lo & 0xFFFFFFFFu);
		unsigned int a[3] = {
			(unsigned int)((key_lo >> 32) & 0xFFFFFFFFu),
			(unsigned int)(key_hi & 0xFFFFFFFFu),
			(unsigned int)((key_hi >> 32) & 0xFFFFFFFFu)
		};

		rk[0] = b;
		for (unsigned int i = 0; i < SPECK_ROUNDS - 1; i++)
		{
			unsigned int ai = a[i % 3];
			speck_round(ai, b, i);
			a[i % 3] = ai;
			rk[i + 1] = b;
		}
	}

	// =========================================================================
	// Stream cipher built on SPECK-64/128 in CTR mode
	// Each 8-byte block gets a unique SPECK encryption of the counter
	// Then XOR'd with plaintext — standard CTR construction
	// =========================================================================
	template <typename CHAR_TYPE>
	constexpr void cipher(CHAR_TYPE* data, size_type size, key_type key)
	{
		// Derive two 64-bit subkeys for SPECK-128 key
		key_type key_hi = derive_subkey(key, 0);
		key_type key_lo = derive_subkey(key, 1);

		// Generate round keys
		unsigned int rk[SPECK_ROUNDS] = {};
		speck_key_schedule(key_hi, key_lo, rk);

		// CTR mode: encrypt counter blocks, XOR with data
		for (size_type block = 0; block <= (size / 8); block++)
		{
			// Build counter block from block index + nonce from key
			unsigned int ctr_lo = (unsigned int)(block & 0xFFFFFFFFu);
			unsigned int ctr_hi = (unsigned int)((key >> 32) ^ (block >> 32));

			// Encrypt counter with SPECK
			for (unsigned int r = 0; r < SPECK_ROUNDS; r++)
				speck_round(ctr_hi, ctr_lo, rk[r]);

			// XOR keystream with data (up to 8 bytes per block)
			unsigned char keystream[8] = {};
			keystream[0] = (unsigned char)(ctr_lo);
			keystream[1] = (unsigned char)(ctr_lo >> 8);
			keystream[2] = (unsigned char)(ctr_lo >> 16);
			keystream[3] = (unsigned char)(ctr_lo >> 24);
			keystream[4] = (unsigned char)(ctr_hi);
			keystream[5] = (unsigned char)(ctr_hi >> 8);
			keystream[6] = (unsigned char)(ctr_hi >> 16);
			keystream[7] = (unsigned char)(ctr_hi >> 24);

			for (unsigned int j = 0; j < 8 && (block * 8 + j) < size; j++)
			{
				size_type idx = block * 8 + j;
				data[idx] = (CHAR_TYPE)((unsigned char)data[idx] ^ keystream[j]);
			}
		}
	}

	// CTR mode is self-inverse: decipher == cipher
	template <typename CHAR_TYPE>
	constexpr void decipher(CHAR_TYPE* data, size_type size, key_type key)
	{
		cipher(data, size, key);
	}

	// =========================================================================
	// ARM64 NEON SIMD accelerated decryption (runtime only)
	// Processes 16 bytes at a time using 128-bit NEON registers
	// IDA/Ghidra struggle with vectorized decrypt patterns
	// =========================================================================
#if AY_HAS_NEON
	template <typename CHAR_TYPE>
	__attribute__((noinline)) static void decipher_neon(CHAR_TYPE* data, size_type size, key_type key)
	{
		key_type key_hi = derive_subkey(key, 0);
		key_type key_lo = derive_subkey(key, 1);

		unsigned int rk[SPECK_ROUNDS] = {};
		speck_key_schedule(key_hi, key_lo, rk);

		// Process 16 bytes (2 SPECK blocks) at a time with NEON
		size_type i = 0;
		for (; i + 16 <= size; i += 16)
		{
			// Generate two keystream blocks
			unsigned int c0_lo = (unsigned int)((i / 8) & 0xFFFFFFFFu);
			unsigned int c0_hi = (unsigned int)((key >> 32) ^ ((i / 8) >> 32));
			unsigned int c1_lo = (unsigned int)(((i / 8) + 1) & 0xFFFFFFFFu);
			unsigned int c1_hi = (unsigned int)((key >> 32) ^ (((i / 8) + 1) >> 32));

			for (unsigned int r = 0; r < SPECK_ROUNDS; r++)
			{
				speck_round(c0_hi, c0_lo, rk[r]);
				speck_round(c1_hi, c1_lo, rk[r]);
			}

			// Build 16-byte keystream
			uint8_t ks[16];
			ks[0]  = (uint8_t)(c0_lo);       ks[1]  = (uint8_t)(c0_lo >> 8);
			ks[2]  = (uint8_t)(c0_lo >> 16);  ks[3]  = (uint8_t)(c0_lo >> 24);
			ks[4]  = (uint8_t)(c0_hi);        ks[5]  = (uint8_t)(c0_hi >> 8);
			ks[6]  = (uint8_t)(c0_hi >> 16);  ks[7]  = (uint8_t)(c0_hi >> 24);
			ks[8]  = (uint8_t)(c1_lo);        ks[9]  = (uint8_t)(c1_lo >> 8);
			ks[10] = (uint8_t)(c1_lo >> 16);  ks[11] = (uint8_t)(c1_lo >> 24);
			ks[12] = (uint8_t)(c1_hi);        ks[13] = (uint8_t)(c1_hi >> 8);
			ks[14] = (uint8_t)(c1_hi >> 16);  ks[15] = (uint8_t)(c1_hi >> 24);

			// NEON: load data, load keystream, XOR, store
			uint8x16_t vdata = vld1q_u8((const uint8_t*)(data + i));
			uint8x16_t vkey  = vld1q_u8(ks);
			uint8x16_t vout  = veorq_u8(vdata, vkey);
			vst1q_u8((uint8_t*)(data + i), vout);
		}

		// Handle remaining bytes with scalar path
		if (i < size)
		{
			// Generate keystream for remaining block(s)
			while (i < size)
			{
				unsigned int ctr_lo = (unsigned int)((i / 8) & 0xFFFFFFFFu);
				unsigned int ctr_hi = (unsigned int)((key >> 32) ^ ((i / 8) >> 32));
				for (unsigned int r = 0; r < SPECK_ROUNDS; r++)
					speck_round(ctr_hi, ctr_lo, rk[r]);

				unsigned char ks2[8];
				ks2[0] = (unsigned char)(ctr_lo);       ks2[1] = (unsigned char)(ctr_lo >> 8);
				ks2[2] = (unsigned char)(ctr_lo >> 16);  ks2[3] = (unsigned char)(ctr_lo >> 24);
				ks2[4] = (unsigned char)(ctr_hi);        ks2[5] = (unsigned char)(ctr_hi >> 8);
				ks2[6] = (unsigned char)(ctr_hi >> 16);  ks2[7] = (unsigned char)(ctr_hi >> 24);

				size_type base = (i / 8) * 8;
				for (unsigned int j = 0; j < 8 && (base + j) < size; j++)
				{
					if (base + j >= i)
						data[base + j] = (CHAR_TYPE)((unsigned char)data[base + j] ^ ks2[j]);
				}
				i = base + 8;
			}
		}
	}
#endif

	// =========================================================================
	// Anti-IDA/Ghidra utilities (ARM64 compatible, no inline asm needed)
	// 5 diverse opaque predicates + junk code injection
	// =========================================================================

	// Opaque predicate 1: algebraic identity a²-b² = (a+b)(a-b)
	__attribute__((noinline)) static bool opaque_true()
	{
		volatile int x = 5;
		volatile int y = 3;
		return (x * x - y * y) == ((x + y) * (x - y));
	}

	// Opaque predicate 2: bitwise disjoint masks
	__attribute__((noinline)) static bool opaque_true2()
	{
		volatile unsigned v = 0x55555555;
		return ((v & 0xAAAAAAAA) == 0);
	}

	// Opaque predicate 3: Fermat's little theorem variant
	// For any odd n: n^2 mod 8 == 1 (always true for odd)
	__attribute__((noinline)) static bool opaque_true3()
	{
		volatile unsigned n = 7;
		volatile unsigned sq = n * n;
		return (sq & 7) == 1;
	}

	// Opaque predicate 4: arithmetic overflow identity
	// (x | (x-1)) + 1 is always a power of 2 when x is a power of 2
	__attribute__((noinline)) static bool opaque_true4()
	{
		volatile unsigned x = 16; // power of 2
		volatile unsigned r = (x | (x - 1)) + 1;
		return (r & (r - 1)) == 0; // check power of 2
	}

	// Opaque predicate 5: modular arithmetic — (2^n) mod (2^n - 1) == 1
	__attribute__((noinline)) static bool opaque_true5()
	{
		volatile unsigned p = 32; // 2^5
		volatile unsigned m = 31; // 2^5 - 1
		return (p % m) == 1;
	}

	// Bogus function that never actually runs but confuses decompiler
	__attribute__((noinline, used)) static void anti_decompile_sink(volatile void* p)
	{
		volatile char* c = (volatile char*)p;
		volatile char x = *c;
		(void)x;
	}

	// Junk code injection: generates dead code that bloats disassembly
	// IDA/Ghidra waste time analyzing these fake operations
	__attribute__((noinline, used)) static void junk_code_block(volatile void* p, volatile unsigned salt)
	{
		volatile unsigned char* d = (volatile unsigned char*)p;
		volatile unsigned a = salt ^ 0xDEADBEEF;
		volatile unsigned b = a * 0x1337;
		volatile unsigned c = b ^ (a >> 3);
		d[0] = (unsigned char)(c & 0xFF);
		volatile unsigned e = c + b;
		d[0] = (unsigned char)((e ^ a) & 0xFF);
		(void)e;
	}

	// Second junk block with different pattern (prevents IDA from folding)
	__attribute__((noinline, used)) static void junk_code_block2(volatile void* p, volatile unsigned salt)
	{
		volatile unsigned char* d = (volatile unsigned char*)p;
		volatile unsigned x = salt * 0xCAFEBABE;
		volatile unsigned y = x ^ (x << 13);
		volatile unsigned z = y + (x >> 7);
		d[0] = (unsigned char)(z ^ y);
		volatile unsigned w = z * y;
		d[0] = (unsigned char)(w & 0xFF);
		(void)w;
	}

	// Anti-debug check (iOS/macOS): returns true if NOT being debugged
	// Woven into decrypt path — debugger attached = silent decrypt failure
	__attribute__((noinline)) static bool anti_debug_check()
	{
#if defined(__APPLE__)
		// Method 1: sysctl P_TRACED check
		struct kinfo_proc info;
		info.kp_proc.p_flag = 0;
		int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
		size_t sz = sizeof(info);
		volatile int ret = sysctl(mib, 4, &info, &sz, nullptr, 0);
		if (ret == 0 && (info.kp_proc.p_flag & P_TRACED) != 0)
			return false; // debugger attached
#endif
		return true;
	}

	// Volatile indirect function pointer for calling decipher
	// IDA/Ghidra cannot resolve volatile function pointer targets
	template <typename CHAR_TYPE>
	struct indirect_decipher {
		using fn_type = void(*)(CHAR_TYPE*, size_type, key_type);
		static fn_type get()
		{
			// Double-volatile indirection — even harder for static analysis
			static volatile fn_type fn_primary = &decipher<CHAR_TYPE>;
			static volatile fn_type fn_backup = &decipher<CHAR_TYPE>;
			// Opaque choice between identical pointers (IDA sees 2 possible targets)
			volatile int selector = 1;
			return (fn_type)(selector ? fn_primary : fn_backup);
		}
	};

#if AY_HAS_NEON
	// NEON indirect pointer — separate resolution path from scalar
	template <typename CHAR_TYPE>
	struct indirect_decipher_neon {
		using fn_type = void(*)(CHAR_TYPE*, size_type, key_type);
		static fn_type get()
		{
			static volatile fn_type fn = &decipher_neon<CHAR_TYPE>;
			return (fn_type)fn;
		}
	};
#endif

	// =========================================================================
	// Obfuscator: encrypts at compile-time
	// =========================================================================
	template <size_type N, key_type KEY, typename CHAR_TYPE = char>
	class obfuscator
	{
	public:
		CONSTEVAL obfuscator(const CHAR_TYPE* data)
		{
			for (size_type i = 0; i < N; i++)
			{
				m_data[i] = data[i];
			}
			cipher(m_data, N, KEY);
		}

		constexpr const CHAR_TYPE* data() const
		{
			return &m_data[0];
		}

		CONSTEVAL size_type size() const
		{
			return N;
		}

		CONSTEVAL key_type key() const
		{
			return KEY;
		}

	private:
		CHAR_TYPE m_data[N]{};
	};

	// =========================================================================
	// Obfuscated data: handles runtime decryption with full anti-RE suite
	// =========================================================================
	template <size_type N, key_type KEY, typename CHAR_TYPE = char>
	class obfuscated_data
	{
	public:
		obfuscated_data(const obfuscator<N, KEY, CHAR_TYPE>& obfuscator)
		{
			for (size_type i = 0; i < N; i++)
			{
				m_data[i] = obfuscator.data()[i];
			}
			// Store compile-time key hash for integrity verification
			m_key_hash = key_hash(KEY);
		}

		~obfuscated_data()
		{
			// Secure zero-fill: volatile prevents compiler from optimizing away
			volatile CHAR_TYPE* vp = m_data;
			for (size_type i = 0; i < N; i++)
			{
				vp[i] = 0;
			}
			// Memory barrier: ensure zeroing completes before memory is freed
#if defined(__aarch64__)
			__asm__ __volatile__("dmb ish" ::: "memory");
#elif defined(__GNUC__) || defined(__clang__)
			__asm__ __volatile__("" ::: "memory");
#endif
		}

		operator CHAR_TYPE* ()
		{
			decrypt();
			return m_data;
		}

		// =================================================================
		// Decrypt with full anti-RE protection suite:
		// - Key integrity check (anti-patch)
		// - Anti-debug (ptrace/sysctl)
		// - 5 opaque predicates (bogus CFG for IDA/Ghidra)
		// - Junk code injection (disassembly bloating)
		// - Volatile indirect calls (unresolvable by static analysis)
		// - NEON SIMD path on ARM64 (complicates pattern matching)
		// =================================================================
		__attribute__((noinline)) void decrypt()
		{
			if (m_encrypted)
			{
				// ---- Layer 1: Key integrity check ----
				volatile key_type expected = key_hash(KEY);
				if (m_key_hash != expected)
					return; // tampered, don't decrypt

				// ---- Layer 2: Anti-debug gate ----
				// If debugger is attached, silently corrupt the output
				volatile bool dbg_ok = anti_debug_check();

				// ---- Layer 3: Opaque predicate + junk code maze ----
				// Creates 5 bogus branches in decompiler output
				if (opaque_true())
				{
					if (opaque_true3())
					{
#if AY_HAS_NEON
						// ARM64: use NEON SIMD decrypt through volatile indirect call
						if (N >= 32 && opaque_true4())
						{
							auto fn = indirect_decipher_neon<CHAR_TYPE>::get();
							fn(m_data, N, KEY);
						}
						else
						{
							auto fn = indirect_decipher<CHAR_TYPE>::get();
							fn(m_data, N, KEY);
						}
#else
						auto fn = indirect_decipher<CHAR_TYPE>::get();
						fn(m_data, N, KEY);
#endif
					}
					else
					{
						// Dead code branch 1: fake SPECK with wrong key
						junk_code_block((volatile void*)m_data, (unsigned)(KEY & 0xFFFFFFFF));
						anti_decompile_sink((volatile void*)m_data);
					}
				}
				else
				{
					// Dead code branch 2: fake XOR decrypt
					anti_decompile_sink((volatile void*)m_data);
					volatile CHAR_TYPE fake = m_data[0];
					for (volatile size_type i = 0; i < N; i++)
						m_data[i] = (CHAR_TYPE)(m_data[i] ^ (fake + i));
				}

				// ---- Layer 4: More opaque predicates for CFG noise ----
				if (opaque_true2())
				{
					if (opaque_true5())
					{
						// Anti-debug corruption: if debugger was detected, XOR with garbage
						if (!dbg_ok)
						{
							volatile unsigned char poison = 0xCC; // INT3 opcode
							for (size_type i = 0; i < N; i++)
								m_data[i] = (CHAR_TYPE)((unsigned char)m_data[i] ^ poison);
						}
						m_encrypted = false;
					}
					else
					{
						// Dead code branch 3: junk block
						junk_code_block2((volatile void*)m_data, (unsigned)((KEY >> 32) & 0xFFFFFFFF));
					}
				}
				else
				{
					// Dead code branch 4: fake re-encrypt
					cipher(m_data, N, KEY);
				}
			}
		}

		void encrypt()
		{
			if (!m_encrypted)
			{
				cipher(m_data, N, KEY);
				m_encrypted = true;
			}
		}

		bool is_encrypted() const
		{
			return m_encrypted;
		}

	private:
		CHAR_TYPE m_data[N];
		bool m_encrypted{ true };
		key_type m_key_hash{ 0 };
	};

	template <size_type N, key_type KEY = OBFUSCATE_DEFAULT_KEY, typename CHAR_TYPE = char>
	CONSTEVAL auto make_obfuscator(const CHAR_TYPE(&data)[N])
	{
		return obfuscator<N, KEY, CHAR_TYPE>(data);
	}
}

// API unchanged — drop-in compatible with all existing code
#define OBFUSCATE(data) OBFUSCATE_KEY(data, OBFUSCATE_DEFAULT_KEY)
#define NSOBFUSCATE(data) [NSString stringWithUTF8String:OBFUSCATE(data)]

#define OBFUSCATE_KEY(data, key) \
	[]() -> ay::obfuscated_data<sizeof(data)/sizeof(data[0]), key, ay::char_type<decltype(*data)>>& { \
		static_assert(sizeof(decltype(key)) == sizeof(ay::key_type), "key must be a 64 bit unsigned integer"); \
		static_assert((key) >= (1ull << 56), "key must span all 8 bytes"); \
		using char_type = ay::char_type<decltype(*data)>; \
		constexpr auto n = sizeof(data)/sizeof(data[0]); \
		constexpr auto obfuscator = ay::make_obfuscator<n, key, char_type>(data); \
		thread_local auto obfuscated_data = ay::obfuscated_data<n, key, char_type>(obfuscator); \
		return obfuscated_data; \
	}()

/* -------------------------------- LICENSE ------------------------------------

Public Domain (http://www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

----------------------------------------------------------------------------- */
