
struct __TestFrame {
	int total_count;
	int fail_count;
	int failed;
};

struct __TestFrame __test_frame = {0, 0, 0};

#define t_assert(A) \
	if(!(A)) { \
		printf("\n\tAssertion failed at %s, %i\n", __FILE__, __LINE__); \
		__test_frame.failed = 1; \
	}

#define t_test(F) \
	t_setup(); \
	_t_run_test(#F, F); \
	t_teardown();

void _t_run_test(char* name, void test()) {
	printf("%s...", name); \
	test();
	__test_frame.total_count++;
	if(!__test_frame.failed) {
		printf("success\n" );
	} else {
		__test_frame.fail_count++;
	}
	__test_frame.failed = 0;
}

void t_init() {
}

int t_done() {
	printf(
		"tests run: %i, errors: %i\n",
		__test_frame.total_count,
		__test_frame.fail_count
	);
	return __test_frame.fail_count;
}
