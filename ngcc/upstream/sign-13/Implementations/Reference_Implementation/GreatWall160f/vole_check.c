#include "vole_check.h"

#include <stdbool.h>
#include "hash.h"
#include "universal_hash.h"
#include "vole_params.h"

typedef struct
{
	poly_secpar_vec matrix[4];

	poly_secpar_vec key_secpar;
	poly64_vec key_64;

	hasher_gfsecpar_key hasher_key_secpar;
	hasher_gf64_key hasher_key_64;
} vole_check_challenge;

static vole_check_challenge load_challenge(const uint8_t* in)
{
	vole_check_challenge out;
	for (size_t i = 0; i < 4; ++i, in += SECURITY_PARAM / 8)
		out.matrix[i] = poly_secpar_load_dup(in);

	out.key_secpar = poly_secpar_load_dup(in);
	out.key_64 = poly64_load_dup(in + SECURITY_PARAM / 8);

	hasher_gfsecpar_init_key(&out.hasher_key_secpar, poly_secpar_exp(out.key_secpar, POLY_VEC_LEN));
	hasher_gf64_init_key(&out.hasher_key_64, poly64_exp(out.key_64, POLY_VEC_LEN));
	return out;
}

static void vole_check_both(
	bool verifier, const vole_block* restrict u, const vole_block* restrict vq,
	const uint8_t* restrict delta_bytes,
	const uint8_t* restrict challenge, uint8_t* restrict proof, uint8_t* restrict check)
{

	vole_check_challenge chal = load_challenge(challenge);

	block_secpar u_hash[2] = {block_secpar_set_zero(), block_secpar_set_zero()};
	if (verifier)
		memcpy(&u_hash[0], proof, VOLE_CHECK_HASH_BYTES);

	hash_state hasher;
	hash_init(&hasher);

	for (int col = -!verifier; col < SECURITY_PARAM; ++col)
	{
		const uint8_t* to_hash = (const uint8_t*) (col == -1 ? u : vq + VOLE_COL_BLOCKS * col);

        GW_STATIC_ASSERT(POLY_VEC_LEN == 1, "160-bit backend uses one lane");
        const size_t input_bytes = QUICKSILVER_ROWS / 8;
        const size_t field_bytes = SECURITY_PARAM / 8;
        const size_t secpar_coeffs = (input_bytes + field_bytes - 1) / field_bytes;
        const size_t gf64_coeffs = (input_bytes + 7) / 8;
        hasher_gfsecpar_state state_secpar;
        hasher_gf64_state state_64;
        hasher_gfsecpar_init_state(&state_secpar, secpar_coeffs);
        hasher_gf64_init_state(&state_64, gf64_coeffs);
        for (size_t offset = 0; offset < input_bytes; offset += field_bytes) {
            uint8_t coefficient[SECURITY_PARAM / 8] = {0};
            size_t take = input_bytes - offset;
            if (take > field_bytes) take = field_bytes;
            memcpy(coefficient, to_hash + offset, take);
            hasher_gfsecpar_update(&chal.hasher_key_secpar, &state_secpar,
                                  poly_secpar_load(coefficient));
        }
        for (size_t offset = 0; offset < input_bytes; offset += 8) {
            uint8_t coefficient[8] = {0};
            size_t take = input_bytes - offset;
            if (take > 8) take = 8;
            memcpy(coefficient, to_hash + offset, take);
            hasher_gf64_update(&chal.hasher_key_64, &state_64, poly64_load(coefficient));
        }

		poly_secpar_vec poly_hashes[2];
		poly_hashes[0] =
			gfsecpar_combine_hashes(chal.key_secpar, hasher_gfsecpar_final(&state_secpar));
		poly_hashes[1] =
			poly_secpar_from_64(gf64_combine_hashes(chal.key_64, hasher_gf64_final(&state_64)));

		poly_secpar_vec mapped_hashes[2];
		for (size_t j = 0; j < 2; ++j)
			mapped_hashes[j] =
				poly_2secpar_reduce_secpar(poly_2secpar_add(
					poly_secpar_mul(chal.matrix[2 * j + 0], poly_hashes[0]),
					poly_secpar_mul(chal.matrix[2 * j + 1], poly_hashes[1])));

		block_secpar hash_output_storage[2];
		block_secpar* hash_output = col == -1 ? u_hash : hash_output_storage;
		for (size_t j = 0; j < 2; ++j)
			poly_secpar_store1(&hash_output[j], mapped_hashes[j]);

		block_secpar mask[2] = { block_secpar_set_zero(), block_secpar_set_zero() };
		memcpy(mask, to_hash + QUICKSILVER_ROWS / 8, VOLE_CHECK_HASH_BYTES);
		for (size_t j = 0; j < 2; ++j)
			hash_output[j] = block_secpar_xor(hash_output[j], mask[j]);

		if (verifier)
			for (size_t j = 0; j < 2; ++j)
				hash_output[j] = block_secpar_xor(hash_output[j],
					block_secpar_and(u_hash[j], block_secpar_set_all_8(delta_bytes[col])));

		if (col >= 0)
			hash_update(&hasher, &hash_output[0], VOLE_CHECK_HASH_BYTES);
	}

	if (!verifier)
		memcpy(proof, &u_hash[0], VOLE_CHECK_HASH_BYTES);

	hash_update_byte(&hasher, 1);
	hash_final(&hasher, check, VOLE_CHECK_CHECK_BYTES);
}

void vole_check_sender(
	const vole_block* restrict u, const vole_block* restrict v,
	const uint8_t* restrict challenge, uint8_t* restrict proof, uint8_t* restrict check)
{
	vole_check_both(false, u, v, NULL, challenge, proof, check);
}

void vole_check_receiver(
	const vole_block* restrict q, const uint8_t* restrict delta_bytes,
	const uint8_t* restrict challenge, const uint8_t* restrict proof, uint8_t* restrict check)
{
	vole_check_both(true, NULL, q, delta_bytes, challenge, (uint8_t*) proof, check);
}
