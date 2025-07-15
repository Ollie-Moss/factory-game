#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/string_cast.hpp>

#define maxPrimeIndex 10

// The primes array used for noise generation
static int primes[maxPrimeIndex][3] = {
	{995615039, 600173719, 701464987},
	{831731269, 162318869, 136250887},
	{174329291, 946737083, 245679977},
	{362489573, 795918041, 350777237},
	{457025711, 880830799, 909678923},
	{787070341, 177340217, 593320781},
	{405493717, 291031019, 391950901},
	{458904767, 676625681, 424452397},
	{531736441, 939683957, 810651871},
	{997169939, 842027887, 423882827}};

static int primeIndex = 0;
class PerlinNoise {
  public:
	static double Noise(double x, double y, int numOctaves, double persistence) {
		double total = 0.0;
		double frequency = pow(2.0, numOctaves);
		double amplitude = 1.0;

		for (int i = 0; i < numOctaves; ++i) {
			frequency /= 2.0;
			amplitude *= persistence;
			total += InterpolatedNoise((primeIndex + i) % maxPrimeIndex, x / frequency, y / frequency) * amplitude;
		}
		return total / frequency;
	}

  private:
	// --- Noise generation functions (with dynamic parameters) ---
	static double BasicNoise(int i, int x, int y) {
		int n = x + y * 57;
		n = (n << 13) ^ n;
		int a = primes[i][0], b = primes[i][1], c = primes[i][2];
		int t = (n * (n * n * a + b) + c) & 0x7fffffff;
		return 1.0 - static_cast<double>(t) / 1073741824.0;
	}

	static double SmoothedNoise(int i, int x, int y) {
		double corners = (BasicNoise(i, x - 1, y - 1) + BasicNoise(i + 1, x + 1, y - 1) +
						  BasicNoise(i + 2, x - 1, y + 1) + BasicNoise(i + 3, x + 1, y + 1)) /
						 16.0;
		double sides = (BasicNoise(i + 4, x - 1, y) + BasicNoise(i + 5, x + 1, y) +
						BasicNoise(i + 6, x, y - 1) + BasicNoise(i + 7, x, y + 1)) /
					   8.0;
		double center = BasicNoise(i + 8, x, y) / 4.0;
		return corners + sides + center;
	}

	static double Interpolate(double a, double b, double x) {
		double ft = x * 3.1415927;
		double f = (1.0 - cos(ft)) * 0.5;
		return a * (1.0 - f) + b * f;
	}

	static double InterpolatedNoise(int i, double x, double y) {
		int intX = static_cast<int>(floor(x));
		double fracX = x - intX;
		int intY = static_cast<int>(floor(y));
		double fracY = y - intY;

		double v1 = SmoothedNoise(i, intX, intY);
		double v2 = SmoothedNoise(i, intX + 1, intY);
		double v3 = SmoothedNoise(i, intX, intY + 1);
		double v4 = SmoothedNoise(i, intX + 1, intY + 1);

		double i1 = Interpolate(v1, v2, fracX);
		double i2 = Interpolate(v3, v4, fracX);
		return Interpolate(i1, i2, fracY);
	}
};

#endif
