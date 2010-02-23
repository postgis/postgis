#define PG_TEST(test_func) { #test_func, test_func }
#define MAX_CUNIT_ERROR_LENGTH 512
char cu_error_msg[MAX_CUNIT_ERROR_LENGTH+1];
void cu_error_msg_reset();

