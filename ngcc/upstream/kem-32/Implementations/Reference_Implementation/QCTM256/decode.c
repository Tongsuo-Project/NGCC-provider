#include <stdlib.h>
#include <string.h>
#include "goppa.h"
#include "scheme_api.h"

static int scheme_decode_twist_row(int t)
{
    return t > 0 ? t - 1 : 0;
}


void poly_syndrome_patterson(poly_t f, gf_t b, poly_t g) {
    int i, t;
    gf_t a, c;

    t = poly_deg(g);
	if (gf_is_zero(b)) {
		gf_inv(a, poly_coeff(g, 0));
		for (i = 0; i < t; i++) {
			gf_mul_fast(c, a, poly_coeff(g, i + 1));
			poly_set_coeff(f, i, c);
		}
	}
	else {
		poly_set_coeff_to_unit(f, t - 1);
		for (i = t - 2; i >= 0; i--) {
			gf_mul_fast(a, b, poly_coeff(f, i + 1));
			gf_add(a, a, poly_coeff(g, i + 1));
			poly_set_coeff(f, i, a);
		}
		gf_mul_fast(a, b, poly_coeff(f, 0));
		gf_add(c, a, poly_coeff(g, 0));

		gf_inv(a, c);
		for (i = 0; i < t; i++) {
			gf_mul_fast(c, a, poly_coeff(f, i));
			poly_set_coeff(f, i, c);
		}
	}

}

static void gf_pow_small(gf_t out, gf_t x, int e)
{
    int i;

    gf_set_to_unit(out);
    for (i = 0; i < e; i++) {
        gf_mul(out, out, x);
    }
}

static void poly_free_if(poly_t p)
{
    if (p != NULL) {
        poly_free(p);
    }
}

static void poly_eval_safe(gf_t res, poly_t p, gf_t a)
{
    if (p == NULL || poly_deg(p) < 0) {
        gf_set_to_zero(res);
        return;
    }
    poly_eval(res, p, a);
}

void poly_syndrome_twisted(poly_t f, gf_t b, poly_t g, gf_t eta)
{
    int t;
    gf_t gb, inv_gb, b_pow, twist;

    /*
     * Sui--Yue Definition 2.1 column:
     *   (x-b)^(-1) - eta*b^t/g(b) mod g(x).
     * The coefficient matrix built from these columns is row-equivalent to
     * their Proposition 2.2 parity-check matrix, whose last row is
     * (b^(t-1) + eta*b^t)/g(b).
     */
    poly_syndrome_patterson(f, b, g);
    t = poly_deg(g);
    poly_eval(gb, g, b);
    if (gf_is_zero(gb)) {
        poly_set_to_zero(f);
        return;
    }
    gf_inv(inv_gb, gb);
    gf_pow_small(b_pow, b, t);
    gf_mul(twist, b_pow, inv_gb);
    gf_mul(twist, twist, eta);
    poly_addto_coeff(f, 0, twist);
    poly_calcule_deg(f);
}

void poly_syndrome_twisted_explicit(poly_t f, gf_t b, poly_t g, gf_t eta)
{
    poly_syndrome_twisted_explicit_row(f, b, g, eta,
                                       scheme_decode_twist_row(poly_deg(g)));
}

void poly_syndrome_twisted_explicit_row(poly_t f, gf_t b, poly_t g,
                                        gf_t eta, int twist_row)
{
    int i;
    int t;
    gf_t gb;
    gf_t inv_gb;
    gf_t b_pow;
    gf_t b_t;
    gf_t value;

    t = poly_deg(g);
    poly_set_to_zero(f);
    if (twist_row < 0 || twist_row >= t) {
        twist_row = t - 1;
    }
    poly_eval(gb, g, b);
    if (gf_is_zero(gb)) {
        return;
    }

    gf_inv(inv_gb, gb);
    gf_set_to_unit(b_pow);
    for (i = 0; i < t; i++) {
        gf_mul(value, b_pow, inv_gb);
        poly_set_coeff(f, i, value);
        gf_mul(b_pow, b_pow, b);
    }

    gf_set_to_unit(b_t);
    for (i = 0; i < t; i++) {
        gf_mul(b_t, b_t, b);
    }
    gf_mul(value, b_t, eta);
    gf_mul(value, value, inv_gb);
    poly_addto_coeff(f, twist_row, value);
    poly_calcule_deg(f);
}

static void poly_add_scaled(poly_t dst, poly_t src, gf_t scale)
{
    int i;
    gf_t a;

    if (gf_is_zero(scale) || poly_deg(src) < 0) {
        return;
    }
    for (i = 0; i <= poly_deg(src) && i < dst->size; i++) {
        gf_mul(a, poly_coeff(src, i), scale);
        poly_addto_coeff(dst, i, a);
    }
    poly_calcule_deg(dst);
}

static poly_t poly_sum(poly_t a, poly_t b)
{
    int size = a->size > b->size ? a->size : b->size;
    gf_t one;
    poly_t out = poly_alloc(size - 1);

    if (out == NULL) {
        return NULL;
    }
    gf_set_to_unit(one);
    poly_set(out, a);
    poly_add_scaled(out, b, one);
    return out;
}

static poly_t poly_linear_comb(poly_t a, poly_t b, gf_t scale)
{
    poly_t out = poly_alloc((a->size > b->size ? a->size : b->size) - 1);

    if (out == NULL) {
        return NULL;
    }
    poly_set(out, a);
    poly_add_scaled(out, b, scale);
    return out;
}

static poly_t poly_derivative(poly_t p)
{
    int i;
    poly_t d = poly_alloc(p->size > 1 ? p->size - 2 : 0);

    if (d == NULL) {
        return NULL;
    }
    for (i = 1; i <= poly_deg(p); i += 2) {
        poly_set_coeff(d, i - 1, poly_coeff(p, i));
    }
    poly_calcule_deg(d);
    return d;
}

static void poly_scale(poly_t p, gf_t scale)
{
    int i;

    if (gf_is_unit(scale)) {
        return;
    }
    if (gf_is_zero(scale)) {
        poly_set_to_zero(p);
        return;
    }
    for (i = 0; i <= poly_deg(p); i++) {
        gf_mul(poly_coeff(p, i), poly_coeff(p, i), scale);
    }
    poly_calcule_deg(p);
}

static int poly_divrem(poly_t q, poly_t rem, poly_t num, poly_t den)
{
    if (poly_deg(den) < 0 || poly_deg(num) < poly_deg(den)) {
        poly_set_to_zero(q);
        poly_set(rem, num);
        return poly_deg(den) >= 0;
    }
    poly_set(rem, num);
    poly_quo_aux(q, rem, den);
    poly_calcule_deg(q);
    poly_calcule_deg(rem);
    return 1;
}

static int twisted_euclidean(poly_t g, poly_t S, int target,
                             poly_t *r_prev_out, poly_t *r_cur_out,
                             poly_t *w_prev_out, poly_t *w_cur_out)
{
    poly_t r_prev = NULL;
    poly_t r_cur = NULL;
    poly_t w_prev = NULL;
    poly_t w_cur = NULL;

    r_prev = poly_copy(g);
    r_cur = poly_copy(S);
    w_prev = poly_alloc(0);
    w_cur = poly_alloc(0);
    if (r_prev == NULL || r_cur == NULL || w_prev == NULL || w_cur == NULL) {
        goto fail;
    }
    poly_set_coeff_to_unit(w_cur, 0);
    poly_set_deg(w_cur, 0);

    while (poly_deg(r_cur) > target) {
        int q_deg;
        poly_t q = NULL;
        poly_t rem = NULL;
        poly_t prod = NULL;
        poly_t w_next = NULL;

        if (poly_deg(r_cur) < 0) {
            goto fail;
        }
        q_deg = poly_deg(r_prev) - poly_deg(r_cur);
        q = poly_alloc(q_deg > 0 ? q_deg : 0);
        rem = poly_alloc(poly_deg(r_prev) >= 0 ? poly_deg(r_prev) : 0);
        if (q == NULL || rem == NULL ||
            !poly_divrem(q, rem, r_prev, r_cur)) {
            poly_free_if(q);
            poly_free_if(rem);
            goto fail;
        }
        prod = poly_mul(q, w_cur);
        if (prod == NULL) {
            poly_free_if(q);
            poly_free_if(rem);
            goto fail;
        }
        w_next = poly_sum(w_prev, prod);
        poly_free(q);
        poly_free(prod);
        if (w_next == NULL) {
            poly_free_if(rem);
            goto fail;
        }

        poly_free(r_prev);
        poly_free(w_prev);
        r_prev = r_cur;
        r_cur = rem;
        w_prev = w_cur;
        w_cur = w_next;
    }

    *r_prev_out = r_prev;
    *r_cur_out = r_cur;
    *w_prev_out = w_prev;
    *w_cur_out = w_cur;
    return 1;

fail:
    poly_free_if(r_prev);
    poly_free_if(r_cur);
    poly_free_if(w_prev);
    poly_free_if(w_cur);
    return 0;
}

int partition(int * tableau, int gauche, int droite, int pivot) {
    int i, temp;
    for (i = gauche; i < droite; i++) {
        if (tableau[i] <= pivot) {
            temp = tableau[i];
            tableau[i] = tableau[gauche];
            tableau[gauche] = temp;
            ++gauche;
        }
	}
    return gauche;
}

void quickSort(int * tableau, int gauche, int droite, int min, int max) {
  if (gauche < droite - 1) {
    int milieu = partition(tableau, gauche, droite, (max + min) / 2);
    quickSort(tableau, gauche, milieu, min, (max + min) / 2);
    quickSort(tableau, milieu, droite, (max + min) / 2, max);
  }
}

poly_t goppa_keyequation_patterson(poly_t R, goppa_t gamma) {
    int i, j;
    poly_t u, v, h, sigma, S, aux;
    gf_t a, b;





    if (poly_calcule_deg(R) < 0) {
	    sigma = poly_alloc(gamma->degree);
		poly_set_coeff_to_unit(sigma, 0);
		poly_set_deg(sigma, 0);
		return sigma;
	}
	
    poly_eeaux(&h ,&aux, R, gamma->g, 1);
    gf_inv(a, poly_coeff(aux,0));
    for (i = 0; i <= poly_deg(h); ++i) {
	    gf_mul_fast(b, a, poly_coeff(h, i));
        poly_set_coeff(h, i, b);
	}
    poly_free(aux);


	gf_set_to_unit(a);
    poly_addto_coeff(h, 1, a);


  S = poly_alloc(gamma->degree - 1);
  for (i = 0; i < gamma->degree; i++) {
		gf_sqrt(a, poly_coeff(h, i));
		if (i & 1) {
#ifndef SQRT_NO_PRECOMP
			aux = gamma->sqrtmod[i / 2];
#else
			if (i == 1) {
				aux = poly_copy(gamma->sqrtzmod);
			}
			else {
				poly_shiftmod(aux, gamma->g);
			}
#endif
			if (!gf_is_zero(a)) {
				for (j = 0; j < gamma->degree; j++) {
					gf_mul_fast(b, a, poly_coeff(aux, j));
					poly_addto_coeff(S, j, b);
				}
			}
		}
		else {
			poly_addto_coeff(S, i / 2, a);
		}
  }
  poly_calcule_deg(S);
  poly_free(h);

#ifdef SQRT_NO_PRECOMP
	poly_free(aux);
#endif


  poly_eeaux(&v, &u, S, gamma->g, gamma->degree / 2 + 1);
  poly_free(S);


  sigma = poly_alloc(gamma->degree);
  for (i = 0; i <= poly_deg(u); ++i) {
		gf_square(b, poly_coeff(u, i));
        poly_set_coeff(sigma, 2 * i, b);
  }
  for (i = 0; i <= poly_deg(v); ++i) {
		gf_square(b, poly_coeff(v, i));
        poly_set_coeff(sigma, 2 * i + 1, b);
    }
    poly_free(u);
    poly_free(v);

    poly_calcule_deg(sigma);

  return sigma;
}

void poly_to_bin_addto(poly_t p, unsigned long * pt, int degree) {
	int i, j, k, l;
	unsigned long c;
	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		k = i / __WORDSIZE;
		j = i % __WORDSIZE;
		c = gf_to_index(poly_coeff(p, l));
		pt[k] ^=  c << j;
		if (__WORDSIZE - j < gf_extd()) {
			pt[k + 1] ^= c >> (__WORDSIZE - j);
		}
	}
}

void bin_to_poly(unsigned long * pt, poly_t p, int degree) {
	int i, j, k, l;
	unsigned long u;
	gf_t a;

	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		k = i / __WORDSIZE;
		j = i % __WORDSIZE;
		u = pt[k] >> j;
		if (__WORDSIZE - j < gf_extd()) {
			u ^= pt[k + 1] << (__WORDSIZE - j);
		}
    u &= ((1UL << gf_extd()) - 1);
		gf_from_index(a, (gfindex_t) u);
    poly_set_coeff(p, l, a);
	}
}

static void bytes_to_poly(const unsigned char *pt, poly_t p, int degree)
{
	int i, j, l;
	gfindex_t u;
	gf_t a;

	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		u = 0;
		for (j = 0; j < gf_extd(); j++) {
			if ((pt[(i + j) / 8] >> ((i + j) % 8)) & 1U) {
				u ^= (gfindex_t)(1U << j);
			}
		}
		gf_from_index(a, u);
		poly_set_coeff(p, l, a);
	}
	poly_calcule_deg(p);
}

static poly_t base_explicit_to_definition(poly_t S, poly_t g)
{
	int i, j, t;
	poly_t R;
	gf_t term, coeff;

	t = poly_deg(g);
	R = poly_alloc(t - 1);
	if (R == NULL) {
		return NULL;
	}

	for (i = 0; i < t; i++) {
		gf_set_to_zero(coeff);
		for (j = i + 1; j <= t; j++) {
			gf_mul(term, poly_coeff(g, j), poly_coeff(S, j - 1 - i));
			gf_add(coeff, coeff, term);
		}
		poly_set_coeff(R, i, coeff);
	}
	poly_calcule_deg(R);
	return R;
}

static poly_t twisted_explicit_to_definition(poly_t S, poly_t g, gf_t eta,
                                             int twist_row, int error_weight)
{
	int j, t;
	poly_t B;
	poly_t R;
	gf_t c, denom, inv_denom, one, term, sum;

	t = poly_deg(g);
	if (twist_row < 0 || twist_row >= t) {
		twist_row = t - 1;
	}

	gf_set_to_zero(sum);
	if ((error_weight & 1) != 0) {
		gf_set_to_unit(sum);
	}
	for (j = 0; j < t; j++) {
		gf_mul(term, poly_coeff(g, j), poly_coeff(S, j));
		gf_add(sum, sum, term);
	}

	gf_mul(c, eta, sum);
	gf_mul(denom, eta, poly_coeff(g, twist_row));
	gf_set_to_unit(one);
	gf_add(denom, denom, one);
	if (gf_is_zero(denom)) {
		return NULL;
	}
	gf_inv(inv_denom, denom);
	gf_mul(c, c, inv_denom);

	B = poly_copy(S);
	if (B == NULL) {
		return NULL;
	}
	poly_addto_coeff(B, twist_row, c);
	poly_calcule_deg(B);

	R = base_explicit_to_definition(B, g);
	poly_free(B);
	if (R == NULL) {
		return NULL;
	}
	poly_addto_coeff(R, 0, c);
	poly_calcule_deg(R);
	return R;
}


void goppa_decode_init(goppa_t gamma) {
    (void)gamma;
#ifndef PARITY_NO_PRECOMP
	if (gamma->parity == NULL) {
		if (gamma->parity) {
			free(gamma->parity[0]);
			free(gamma->parity);
		}

		int i, nb_synd, alloc_size, degree;
		unsigned long * synd;
		poly_t p;
		gfelt_t * L;

		degree = gamma->degree;
		alloc_size = 1 + (degree * gf_extd() - 1) / __WORDSIZE;

		L = gamma->L + (gamma->length - gamma->degree * gf_extd());
		nb_synd = gamma->degree * gf_extd();		

		gamma->parity = (unsigned long **) malloc(nb_synd * sizeof (unsigned long *));

		synd = (unsigned long *) calloc(nb_synd * alloc_size, sizeof (unsigned long));
		p = poly_alloc(degree - 1);
		for (i = 0; i < nb_synd; i++, synd += alloc_size) {
			poly_syndrome_twisted_explicit(p, L + i, gamma->g, gamma->eta);

			poly_to_bin_addto(p, synd, degree);
			gamma->parity[i] = synd;
		}
		poly_free(p);
	}
#endif
}

poly_t goppa_syndrome(const unsigned char * s, goppa_t gamma) {
	int i, alloc_size, nb_synd, degree;
	poly_t R;
	unsigned long * synd;

#ifdef PARITY_NO_PRECOMP
	gfelt_t * L;
#endif

	goppa_decode_init(gamma);

	degree = gamma->degree;
	alloc_size = 1 + (degree * gf_extd() - 1) / __WORDSIZE;

	if (gamma->explicit_syndrome_input == 2) {
		R = poly_alloc(degree);
		if (R == NULL) {
			return NULL;
		}
		bytes_to_poly(s, R, degree);
		return R;
	}

	if (gamma->explicit_syndrome_input) {
		poly_t converted;

		R = poly_alloc(degree);
		if (R == NULL) {
			return NULL;
		}
		bytes_to_poly(s, R, degree);
		converted = twisted_explicit_to_definition(R, gamma->g, gamma->eta,
		                                           scheme_decode_twist_row(degree),
		                                           ERROR_WEIGHT);
		poly_free(R);
		return converted;
	}



#ifdef PARITY_NO_PRECOMP
	L = gamma->L + (gamma->length - gamma->degree * gf_extd());
#endif
	nb_synd = gamma->degree * gf_extd();


	synd = (unsigned long *) calloc(alloc_size, sizeof (unsigned long));
	R = poly_alloc(degree );
	for (i = 0; i < nb_synd; i++) {
		if ((s[i / 8] >> (i % 8)) & 1) {
#ifndef PARITY_NO_PRECOMP
			xor_long(synd, gamma->parity[i], alloc_size);
#else
		poly_syndrome_twisted_explicit(R, L + i, gamma->g, gamma->eta);
			poly_to_bin_addto(R, synd, degree);
#endif
		}
	}
	bin_to_poly(synd, R, degree);
	poly_calcule_deg(R);
	free(synd);

	{
		poly_t converted = twisted_explicit_to_definition(R, gamma->g,
		                                                  gamma->eta,
		                                                  scheme_decode_twist_row(degree),
		                                                  ERROR_WEIGHT);
		poly_free(R);
		R = converted;
	}

  return R;
}


int goppa_locate_error(poly_t sigma, int * e, goppa_t gamma) {
	int i, d;
	gfelt_t * roots;

	roots = (gfelt_t *) malloc(gamma->degree * sizeof (gfelt_t));
	d = roots_berl(sigma, roots);

  for (i = 0; i < d; ++i) {
    e[i] = gamma->Linv[gf_to_index(roots + i)];
	}
	free(roots);


	quickSort(e, 0, d, 0, gamma->length);
  return d;
}



int goppa_decode(const unsigned char * b, int * e, goppa_t gamma) {
    int j;
    int weight = 0;
    int target = ERROR_WEIGHT;
    gf_t eval_sigma, eval_tau, eval_der, value;
    poly_t R = NULL;
    poly_t r_prev = NULL;
    poly_t r_cur = NULL;
    poly_t w_prev = NULL;
    poly_t w_cur = NULL;
    poly_t sigma_der = NULL;

    if (target <= 0) {
        return -1;
    }

    R = goppa_syndrome(b, gamma);
    if (R == NULL || poly_deg(R) < 0) {
        poly_free_if(R);
        return -1;
    }

    if (!twisted_euclidean(gamma->g, R, target,
                           &r_prev, &r_cur, &w_prev, &w_cur)) {
        goto cleanup;
    }
    if (poly_deg(w_cur) < 0 || poly_deg(w_cur) > target) {
        goto cleanup;
    }

    sigma_der = poly_derivative(w_cur);
    if (sigma_der == NULL) {
        goto cleanup;
    }

    for (j = 0; j < gamma->length; j++) {
        poly_eval_safe(eval_sigma, w_cur, gamma->L + j);
        if (!gf_is_zero(eval_sigma)) {
            continue;
        }
        if (weight >= target) {
            goto cleanup;
        }

        poly_eval_safe(eval_der, sigma_der, gamma->L + j);
        if (gf_is_zero(eval_der)) {
            goto cleanup;
        }
        poly_eval_safe(eval_tau, r_cur, gamma->L + j);
        gf_div(value, eval_tau, eval_der);

        if (!gf_is_unit(value)) {
            goto cleanup;
        }
        e[weight++] = j;
    }

cleanup:
    if (weight == target) {
        quickSort(e, 0, weight, 0, gamma->length);
    } else {
        weight = -1;
    }
    poly_free_if(R);
    poly_free_if(r_prev);
    poly_free_if(r_cur);
    poly_free_if(w_prev);
    poly_free_if(w_cur);
    poly_free_if(sigma_der);
    return weight;
}
