/* Radius of an arc given width and height */
#define radius(w,h) (h / 2 + (w * w) / (8 * h))

/* Angle given number of sides */
#define angle(s) (360 / s)

/* Miter angle given number of sides */
#define miter(s) (360 / (s * 2))

/* Convert radians to degrees */
#define degrees(r) (r * (180 / M_PI))

/* Convert degrees to radians */
#define radians(a) (a * (M_PI / 180))

/* Length of chord given diameter and number of sides */
#define chord(d,s) ((2 * d) * sin(radians(360 / s) / 2))

/* Arithmetic sum from 1 to n: n(n-1)/2 */
#define sum1ToN(n) (((n) * ((n) + 1)) / 2)

/* Sum of powers from 1 to n: (x^n-1)/(x-1)  The equation is undefined for x=1,
       so just return n (1 + 1^1 + ... + 1^(n-1) = n). */
#define sumOfConsecutivePowers(x, n) (x == 1 ? n : (pow(x, n) - 1) / (x - 1))

/* Round a fractional number to the nearest unit of precision */
#define roundFraction(f,p) (round(f * p) / p)

/* Find the nth number in the Fibonacci sequence */
#define fibonacci(n) ((pow((1 + sqrt(5)) / 2, n) - pow((1 - sqrt(5)) / 2, n) / sqrt(5)))

/* Find the sum of the first n Fibonacci numbers */
#define sumOfFibonacciNumbers(n) (fibonacci(n + 2) - 1)
