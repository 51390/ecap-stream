#include <check.h>
#include <string.h>
#include "../src/service.h"

START_TEST (test_service_uri) {
    EcapStream::Service service;
    std::string uri = service.uri();
    ck_assert_str_eq(uri.c_str(), "ecap://github.com/51390/ecap-stream");
}
END_TEST

Suite* service_suite() {
    Suite* suite;
    TCase* service_test_case;

    suite = suite_create("service");
    service_test_case = tcase_create("service");
    tcase_add_test(service_test_case, test_service_uri);
    suite_add_tcase(suite, service_test_case);

    return suite;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = service_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
