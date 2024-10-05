#ifndef SPECIAL_FUNCTIONS_H
#define SPECIAL_FUNCTIONS_H

/////////////////////////////////  MIT LICENSE  ////////////////////////////////

//  Copyright (C) 2014 TroggleMonkey
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.


///////////////////////////  GAUSSIAN ERROR FUNCTION  //////////////////////////

vec4 erf6(vec4 x)
{
    //  Requires:   x is the standard parameter to erf().
    //  Returns:    Return an Abramowitz/Stegun approximation of erf(), where:
    //                  erf(x) = 2/sqrt(pi) * integral(e**(-x**2))
    //              This approximation has a max absolute error of 2.5*10**-5
    //              with solid numerical robustness and efficiency.  See:
	//                  https://en.wikipedia.org/wiki/Error_function#Approximation_with_elementary_functions
	vec4 one = vec4(1.0);
	vec4 sign_x = sign(x);
	vec4 t = one/(one + 0.47047*abs(x));
	vec4 result = one - t*(0.3480242 + t*(-0.0958798 + t*0.7478556))*
		exp(-(x*x));
	return result * sign_x;
}

vec3 erf6(vec3 x)
{
    //  Float3 version:
	vec3 one = vec3(1.0);
	vec3 sign_x = sign(x);
	vec3 t = one/(one + 0.47047*abs(x));
	vec3 result = one - t*(0.3480242 + t*(-0.0958798 + t*0.7478556))*
		exp(-(x*x));
	return result * sign_x;
}

vec2 erf6(vec2 x)
{
    //  Float2 version:
	vec2 one = vec2(1.0);
	vec2 sign_x = sign(x);
	vec2 t = one/(one + 0.47047*abs(x));
	vec2 result = one - t*(0.3480242 + t*(-0.0958798 + t*0.7478556))*
		exp(-(x*x));
	return result * sign_x;
}

float erf6(float x)
{
    //  Float version:
	float sign_x = sign(x);
	float t = 1.0/(1.0 + 0.47047*abs(x));
	float result = 1.0 - t*(0.3480242 + t*(-0.0958798 + t*0.7478556))*
		exp(-(x*x));
	return result * sign_x;
}

vec4 erft(vec4 x)
{
    //  Requires:   x is the standard parameter to erf().
    //  Returns:    Approximate erf() with the hyperbolic tangent.  The error is
    //              visually noticeable, but it's blazing fast and perceptually
    //              close...at least on ATI hardware.  See:
    //                  http://www.maplesoft.com/applications/view.aspx?SID=5525&view=html
    //  Warning:    Only use this if your hardware drivers correctly implement
    //              tanh(): My nVidia 8800GTS returns garbage output.
	return tanh(1.202760580 * x);
}

vec3 erft(vec3 x)
{
    //  Float3 version:
	return tanh(1.202760580 * x);
}

vec2 erft(vec2 x)
{
    //  Float2 version:
	return tanh(1.202760580 * x);
}

float erft(float x)
{
    //  Float version:
	return tanh(1.202760580 * x);
}

vec4 erf(vec4 x)
{
    //  Requires:   x is the standard parameter to erf().
    //  Returns:    Some approximation of erf(x), depending on user settings.
	#ifdef ERF_FAST_APPROXIMATION
		return erft(x);
	#else
		return erf6(x);
	#endif
}

vec3 erf(vec3 x)
{
    //  Float3 version:
	#ifdef ERF_FAST_APPROXIMATION
		return erft(x);
	#else
		return erf6(x);
	#endif
}

vec2 erf(vec2 x)
{
    //  Float2 version:
	#ifdef ERF_FAST_APPROXIMATION
		return erft(x);
	#else
		return erf6(x);
	#endif
}

float erf(float x)
{
    //  Float version:
	#ifdef ERF_FAST_APPROXIMATION
		return erft(x);
	#else
		return erf6(x);
	#endif
}


///////////////////////////  COMPLETE GAMMA FUNCTION  //////////////////////////

vec4 gamma_impl(vec4 s, vec4 s_inv)
{
    //  Requires:   1.) s is the standard parameter to the gamma function, and
    //                  it should lie in the [0, 36] range.
    //              2.) s_inv = 1.0/s.  This implementation function requires
    //                  the caller to precompute this value, giving users the
    //                  opportunity to reuse it.
    //  Returns:    Return approximate gamma function (real-numbered factorial)
    //              output using the Lanczos approximation with two coefficients
    //              calculated using Paul Godfrey's method here:
    //                  http://my.fit.edu/~gabdo/gamma.txt
    //              An optimal g value for s in [0, 36] is ~1.12906830989, with
    //              a maximum relative error of 0.000463 for 2**16 equally
    //              evals.  We could use three coeffs (0.0000346 error) without
    //              hurting latency, but this allows more parallelism with
    //              outside instructions.
	vec4 g = vec4(1.12906830989);
	vec4 c0 = vec4(0.8109119309638332633713423362694399653724431);
	vec4 c1 = vec4(0.4808354605142681877121661197951496120000040);
	vec4 e = vec4(2.71828182845904523536028747135266249775724709);
	vec4 sph = s + vec4(0.5);
	vec4 lanczos_sum = c0 + c1/(s + vec4(1.0));
	vec4 base = (sph + g)/e;  //  or (s + g + vec4(0.5))/e
	//  gamma(s + 1) = base**sph * lanczos_sum; divide by s for gamma(s).
	//  This has less error for small s's than (s -= 1.0) at the beginning.
	return (pow(base, sph) * lanczos_sum) * s_inv;
}

vec3 gamma_impl(vec3 s, vec3 s_inv)
{
    //  Float3 version:
	vec3 g = vec3(1.12906830989);
	vec3 c0 = vec3(0.8109119309638332633713423362694399653724431);
	vec3 c1 = vec3(0.4808354605142681877121661197951496120000040);
	vec3 e = vec3(2.71828182845904523536028747135266249775724709);
	vec3 sph = s + vec3(0.5);
	vec3 lanczos_sum = c0 + c1/(s + vec3(1.0));
	vec3 base = (sph + g)/e;
	return (pow(base, sph) * lanczos_sum) * s_inv;
}

vec2 gamma_impl(vec2 s, vec2 s_inv)
{
    //  Float2 version:
	vec2 g = vec2(1.12906830989);
	vec2 c0 = vec2(0.8109119309638332633713423362694399653724431);
	vec2 c1 = vec2(0.4808354605142681877121661197951496120000040);
	vec2 e = vec2(2.71828182845904523536028747135266249775724709);
	vec2 sph = s + vec2(0.5);
	vec2 lanczos_sum = c0 + c1/(s + vec2(1.0));
	vec2 base = (sph + g)/e;
	return (pow(base, sph) * lanczos_sum) * s_inv;
}

float gamma_impl(float s, float s_inv)
{
    //  Float version:
	float g = 1.12906830989;
	float c0 = 0.8109119309638332633713423362694399653724431;
	float c1 = 0.4808354605142681877121661197951496120000040;
	float e = 2.71828182845904523536028747135266249775724709;
	float sph = s + 0.5;
	float lanczos_sum = c0 + c1/(s + 1.0);
	float base = (sph + g)/e;
	return (pow(base, sph) * lanczos_sum) * s_inv;
}

vec4 gamma(vec4 s)
{
    //  Requires:   s is the standard parameter to the gamma function, and it
    //              should lie in the [0, 36] range.
    //  Returns:    Return approximate gamma function output with a maximum
    //              relative error of 0.000463.  See gamma_impl for details.
	return gamma_impl(s, vec4(1.0)/s);
}

vec3 gamma(vec3 s)
{
    //  Float3 version:
	return gamma_impl(s, vec3(1.0)/s);
}

vec2 gamma(vec2 s)
{
    //  Float2 version:
	return gamma_impl(s, vec2(1.0)/s);
}

float gamma(float s)
{
    //  Float version:
	return gamma_impl(s, 1.0/s);
}


////////////////  INCOMPLETE GAMMA FUNCTIONS (RESTRICTED INPUT)  ///////////////

//  Lower incomplete gamma function for small s and z (implementation):
vec4 ligamma_small_z_impl(vec4 s, vec4 z, vec4 s_inv)
{
    //  Requires:   1.) s < ~0.5
    //              2.) z <= ~0.775075
    //              3.) s_inv = 1.0/s (precomputed for outside reuse)
    //  Returns:    A series representation for the lower incomplete gamma
    //              function for small s and small z (4 terms).
    //  The actual "rolled up" summation looks like:
	//      last_sign = 1.0; last_pow = 1.0; last_factorial = 1.0;
	//      sum = last_sign * last_pow / ((s + k) * last_factorial)
	//      for(int i = 0; i < 4; ++i)
	//      {
	//          last_sign *= -1.0; last_pow *= z; last_factorial *= i;
	//          sum += last_sign * last_pow / ((s + k) * last_factorial);
	//      }
	//  Unrolled, constant-unfolded and arranged for madds and parallelism:
	vec4 scale = pow(z, s);
	vec4 sum = s_inv;  //  Summation iteration 0 result
	//  Summation iterations 1, 2, and 3:
	vec4 z_sq = z*z;
	vec4 denom1 = s + vec4(1.0);
	vec4 denom2 = 2.0*s + vec4(4.0);
	vec4 denom3 = 6.0*s + vec4(18.0);
	//vec4 denom4 = 24.0*s + vec4(96.0);
	sum -= z/denom1;
	sum += z_sq/denom2;
	sum -= z * z_sq/denom3;
	//sum += z_sq * z_sq / denom4;
	//  Scale and return:
	return scale * sum;
}

vec3 ligamma_small_z_impl(vec3 s, vec3 z, vec3 s_inv)
{
    //  Float3 version:
	vec3 scale = pow(z, s);
	vec3 sum = s_inv;
	vec3 z_sq = z*z;
	vec3 denom1 = s + vec3(1.0);
	vec3 denom2 = 2.0*s + vec3(4.0);
	vec3 denom3 = 6.0*s + vec3(18.0);
	sum -= z/denom1;
	sum += z_sq/denom2;
	sum -= z * z_sq/denom3;
	return scale * sum;
}

vec2 ligamma_small_z_impl(vec2 s, vec2 z, vec2 s_inv)
{
    //  Float2 version:
	vec2 scale = pow(z, s);
	vec2 sum = s_inv;
	vec2 z_sq = z*z;
	vec2 denom1 = s + vec2(1.0);
	vec2 denom2 = 2.0*s + vec2(4.0);
	vec2 denom3 = 6.0*s + vec2(18.0);
	sum -= z/denom1;
	sum += z_sq/denom2;
	sum -= z * z_sq/denom3;
	return scale * sum;
}

float ligamma_small_z_impl(float s, float z, float s_inv)
{
    //  Float version:
	float scale = pow(z, s);
	float sum = s_inv;
	float z_sq = z*z;
	float denom1 = s + 1.0;
	float denom2 = 2.0*s + 4.0;
	float denom3 = 6.0*s + 18.0;
	sum -= z/denom1;
	sum += z_sq/denom2;
	sum -= z * z_sq/denom3;
	return scale * sum;
}

//  Upper incomplete gamma function for small s and large z (implementation):
vec4 uigamma_large_z_impl(vec4 s, vec4 z)
{
    //  Requires:   1.) s < ~0.5
    //              2.) z > ~0.775075
    //  Returns:    Gauss's continued fraction representation for the upper
    //              incomplete gamma function (4 terms).
	//  The "rolled up" continued fraction looks like this.  The denominator
    //  is truncated, and it's calculated "from the bottom up:"
	//      denom = vec4('inf');
	//      vec4 one = vec4(1.0);
	//      for(int i = 4; i > 0; --i)
	//      {
	//          denom = ((i * 2.0) - one) + z - s + (i * (s - i))/denom;
	//      }
	//  Unrolled and constant-unfolded for madds and parallelism:
	vec4 numerator = pow(z, s) * exp(-z);
	vec4 denom = vec4(7.0) + z - s;
	denom = vec4(5.0) + z - s + (3.0*s - vec4(9.0))/denom;
	denom = vec4(3.0) + z - s + (2.0*s - vec4(4.0))/denom;
	denom = vec4(1.0) + z - s + (s - vec4(1.0))/denom;
	return numerator / denom;
}

vec3 uigamma_large_z_impl(vec3 s, vec3 z)
{
    //  Float3 version:
	vec3 numerator = pow(z, s) * exp(-z);
	vec3 denom = vec3(7.0) + z - s;
	denom = vec3(5.0) + z - s + (3.0*s - vec3(9.0))/denom;
	denom = vec3(3.0) + z - s + (2.0*s - vec3(4.0))/denom;
	denom = vec3(1.0) + z - s + (s - vec3(1.0))/denom;
	return numerator / denom;
}

vec2 uigamma_large_z_impl(vec2 s, vec2 z)
{
    //  Float2 version:
	vec2 numerator = pow(z, s) * exp(-z);
	vec2 denom = vec2(7.0) + z - s;
	denom = vec2(5.0) + z - s + (3.0*s - vec2(9.0))/denom;
	denom = vec2(3.0) + z - s + (2.0*s - vec2(4.0))/denom;
	denom = vec2(1.0) + z - s + (s - vec2(1.0))/denom;
	return numerator / denom;
}

float uigamma_large_z_impl(float s, float z)
{
    //  Float version:
	float numerator = pow(z, s) * exp(-z);
	float denom = 7.0 + z - s;
	denom = 5.0 + z - s + (3.0*s - 9.0)/denom;
	denom = 3.0 + z - s + (2.0*s - 4.0)/denom;
	denom = 1.0 + z - s + (s - 1.0)/denom;
	return numerator / denom;
}

//  Normalized lower incomplete gamma function for small s (implementation):
vec4 normalized_ligamma_impl(vec4 s, vec4 z,
    vec4 s_inv, vec4 gamma_s_inv)
{
    //  Requires:   1.) s < ~0.5
    //              2.) s_inv = 1/s (precomputed for outside reuse)
    //              3.) gamma_s_inv = 1/gamma(s) (precomputed for outside reuse)
    //  Returns:    Approximate the normalized lower incomplete gamma function
    //              for s < 0.5.  Since we only care about s < 0.5, we only need
    //              to evaluate two branches (not four) based on z.  Each branch
    //              uses four terms, with a max relative error of ~0.00182.  The
    //              branch threshold and specifics were adapted for fewer terms
    //              from Gil/Segura/Temme's paper here:
    //                  http://oai.cwi.nl/oai/asset/20433/20433B.pdf
	//  Evaluate both branches: Real branches test slower even when available.
	vec4 thresh = vec4(0.775075);
	bvec4 z_is_large;
	z_is_large.x = z.x > thresh.x;
	z_is_large.y = z.y > thresh.y;
	z_is_large.z = z.z > thresh.z;
	z_is_large.w = z.w > thresh.w;
	vec4 large_z = vec4(1.0) - uigamma_large_z_impl(s, z) * gamma_s_inv;
	vec4 small_z = ligamma_small_z_impl(s, z, s_inv) * gamma_s_inv;
	//  Combine the results from both branches:
	bvec4 inverse_z_is_large = not(z_is_large);
	return large_z * vec4(z_is_large) + small_z * vec4(inverse_z_is_large);
}

vec3 normalized_ligamma_impl(vec3 s, vec3 z,
    vec3 s_inv, vec3 gamma_s_inv)
{
    //  Float3 version:
	vec3 thresh = vec3(0.775075);
	bvec3 z_is_large;
	z_is_large.x = z.x > thresh.x;
	z_is_large.y = z.y > thresh.y;
	z_is_large.z = z.z > thresh.z;
	vec3 large_z = vec3(1.0) - uigamma_large_z_impl(s, z) * gamma_s_inv;
	vec3 small_z = ligamma_small_z_impl(s, z, s_inv) * gamma_s_inv;
	bvec3 inverse_z_is_large = not(z_is_large);
	return large_z * vec3(z_is_large) + small_z * vec3(inverse_z_is_large);
}

vec2 normalized_ligamma_impl(vec2 s, vec2 z,
    vec2 s_inv, vec2 gamma_s_inv)
{
    //  Float2 version:
	vec2 thresh = vec2(0.775075);
	bvec2 z_is_large;
	z_is_large.x = z.x > thresh.x;
	z_is_large.y = z.y > thresh.y;
	vec2 large_z = vec2(1.0) - uigamma_large_z_impl(s, z) * gamma_s_inv;
	vec2 small_z = ligamma_small_z_impl(s, z, s_inv) * gamma_s_inv;
	bvec2 inverse_z_is_large = not(z_is_large);
	return large_z * vec2(z_is_large) + small_z * vec2(inverse_z_is_large);
}

float normalized_ligamma_impl(float s, float z,
    float s_inv, float gamma_s_inv)
{
    //  Float version:
	float thresh = 0.775075;
	bool z_is_large = z > thresh;
	float large_z = 1.0 - uigamma_large_z_impl(s, z) * gamma_s_inv;
	float small_z = ligamma_small_z_impl(s, z, s_inv) * gamma_s_inv;
	return large_z * float(z_is_large) + small_z * float(!z_is_large);
}

//  Normalized lower incomplete gamma function for small s:
vec4 normalized_ligamma(vec4 s, vec4 z)
{
    //  Requires:   s < ~0.5
    //  Returns:    Approximate the normalized lower incomplete gamma function
    //              for s < 0.5.  See normalized_ligamma_impl() for details.
	vec4 s_inv = vec4(1.0)/s;
	vec4 gamma_s_inv = vec4(1.0)/gamma_impl(s, s_inv);
	return normalized_ligamma_impl(s, z, s_inv, gamma_s_inv);
}

vec3 normalized_ligamma(vec3 s, vec3 z)
{
    //  Float3 version:
	vec3 s_inv = vec3(1.0)/s;
	vec3 gamma_s_inv = vec3(1.0)/gamma_impl(s, s_inv);
	return normalized_ligamma_impl(s, z, s_inv, gamma_s_inv);
}

vec2 normalized_ligamma(vec2 s, vec2 z)
{
    //  Float2 version:
	vec2 s_inv = vec2(1.0)/s;
	vec2 gamma_s_inv = vec2(1.0)/gamma_impl(s, s_inv);
	return normalized_ligamma_impl(s, z, s_inv, gamma_s_inv);
}

float normalized_ligamma(float s, float z)
{
    //  Float version:
	float s_inv = 1.0/s;
	float gamma_s_inv = 1.0/gamma_impl(s, s_inv);
	return normalized_ligamma_impl(s, z, s_inv, gamma_s_inv);
}


#endif  //  SPECIAL_FUNCTIONS_H


