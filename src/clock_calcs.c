#define NUM_PRODUCER_THREADS 5

// Perform Greatest Common Divisor (GCD) using Euclidean Division
// -> Obtain Suitable Clock Interval for Accurate Timing
int calc_gcd(int p1, int p2) {
	if(p1 == 0)
		return p2;

	return calc_gcd(p2 % p1, p1);
}

// Perform Least Common Multiple (LCM) using GCD Factor
// -> Obtain Suitable Time to Request User Input at Start of Major Cycle
int calc_lcm(int p1, int p2) {
	return (p1 * p2)/calc_gcd(p1, p2);
}

// Get Clock Interval by Calculating GCD Among Producer Thread Periods
int get_clock_interval(int periods[]) {
	// Iterate Over Each Period to Obtain Greatest Common Divisor, Suitable Clock Interval
	int clock_interval = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		clock_interval = calc_gcd(periods[i], clock_interval);
	}

	return clock_interval;
}

// Get Hyperperiod by Calculating LCM AMong Producer Thread Periods
int get_hyperperiod(int periods[]) {
	// Iterate Over Each Period to Obtain Least Common Multiple for Major Cycle
	int major_cycle = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		major_cycle = calc_lcm(periods[i], major_cycle);
	}

	return major_cycle;
}

// Get Consumer Period to Determine Rate at Which Data is Displayed to User
// -> Equivalent to Minimum Producer Period
int get_consumer_period(int periods[]) {
	int min_period = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		if(min_period > periods[i]) {
			min_period = periods[i];
		}
	}

	return min_period;
}
