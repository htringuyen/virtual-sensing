/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <dummy/dummy.h>


ZTEST_SUITE(dummy_test, NULL, NULL, NULL, NULL, NULL);


ZTEST(dummy_test, test_assert)
{
	zassert_true(get_dummy_ok(), "Dummy is not ok!");
}
