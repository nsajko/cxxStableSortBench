#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // printf

#include <sys/resource.h> // setrlimit
#include <time.h>         // clock_gettime

#include <algorithm> // stable_sort

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
	int64_t a;
	int64_t b;
} intPair;
intPair data[(int64_t)1e8];

void
prFew(const intPair arr[]) {
	printf("[");
	int i;
	for (i = 0; i < 5; i++) {
		printf("[%7ld %7ld] ", arr[i].a, arr[i].b);
	}
	printf("]\n");
}

int
now(struct timespec *t) {
	return clock_gettime(CLOCK_MONOTONIC, t);
}

// Returns seconds since ref, or less than zero in case of an error.
int64_t
since(const struct timespec *ref) {
	struct timespec current;
	int status = now(&current);
	if (status < 0) {
		return status;
	}
	int64_t secs = (int64_t)(current.tv_sec) - (int64_t)(ref->tv_sec);
	if (secs < 0) {
		return secs;
	}
	if (long(5e8) < current.tv_nsec - ref->tv_nsec) {
		return secs + 1;
	}
	return secs;
}

typedef struct {
	int64_t secsDuration;
	struct timespec started;
} stopwatch;

bool
lessNotInlined(const intPair *x, const intPair *y) {
	return x->a < y->a;
}
bool
less(const intPair x, const intPair y) {
	// Simulate Go method call overhead.
	return lessNotInlined(&x, &y);
}

int
bench(void) {
	stopwatch sw = {0};

	// A replica of func bench from $GOROOT/src/sort/sort_test.go
	uint32_t x = ~uint32_t(0);
	size_t n;
	for (n = ARRAY_LEN(data) - 3; n <= ARRAY_LEN(data) + 3; n++) {
		size_t i;
		for (i = 0; i < ARRAY_LEN(data); i++) {
			x += x;
			x ^= 1;
			if ((int32_t)x < 0) {
				x ^= 0x88888eef;
			}
			data[i].a = (int64_t)(x % (uint32_t)(n / 5));
			data[i].b = i;
		}
		prFew(data);

		// Start the stopwatch.
		int status = now(&sw.started);
		if (status < 0) {
			return status;
		}

		std::stable_sort(data, data + n, less);

		// Stop the stopwatch.
		sw.secsDuration += since(&sw.started);

		prFew(data);
	}
	// Report stopwatch duration value.
	printf("%ld s\n", sw.secsDuration);
	return 0;
}

int
main(void) {
	// Limit the amount of memory available to the process so stable_sort
	// would choose the algorithm that uses a sublinear amount of
	// space/memory.
	struct rlimit rlp;
	rlp.rlim_cur = sizeof(data) + 2048*2;
	rlp.rlim_max = rlp.rlim_cur;
	setrlimit(RLIMIT_AS, &rlp);

	return bench();
}
