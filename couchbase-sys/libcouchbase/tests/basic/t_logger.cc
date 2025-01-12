/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2011-2019 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "config.h"
#include <gtest/gtest.h>
#include <libcouchbase/couchbase.h>
#include "logging.h"
#include "internal.h"
#include <list>

using namespace std;

class Logger : public ::testing::Test
{
};

struct MyLogprocs {
    MyLogprocs() : base(NULL) {}
    lcb_LOGGER *base;
    set< string > messages;
};

extern "C" {
static void fallback_logger(lcb_LOGGER *logger, uint64_t, const char *, lcb_LOG_SEVERITY, const char *, int,
                            const char *fmt, va_list ap)
{
    char buf[2048];
    vsprintf(buf, fmt, ap);
    EXPECT_FALSE(logger == NULL);
    MyLogprocs *myprocs;
    lcb_logger_cookie(logger, reinterpret_cast< void ** >(&myprocs));
    myprocs->messages.insert(buf);
}
}

TEST_F(Logger, testLogger)
{
    lcb_INSTANCE *instance;
    lcb_STATUS err;

    MyLogprocs procs;
    lcb_logger_create(&procs.base, &procs);
    lcb_logger_callback(procs.base, fallback_logger);

    lcb_create(&instance, NULL);

    err = lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_LOGGER, procs.base);
    ASSERT_EQ(LCB_SUCCESS, err);

    LCB_LOG_BASIC(instance->getSettings(), "foo");
    LCB_LOG_BASIC(instance->getSettings(), "bar");
    LCB_LOG_BASIC(instance->getSettings(), "baz");

    set< string > &msgs = procs.messages;
    ASSERT_FALSE(msgs.find("foo") == msgs.end());
    ASSERT_FALSE(msgs.find("bar") == msgs.end());
    ASSERT_FALSE(msgs.find("baz") == msgs.end());
    msgs.clear();

    // Try without a logger
    err = lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_LOGGER, NULL);
    ASSERT_EQ(LCB_SUCCESS, err);
    LCB_LOG_BASIC(instance->getSettings(), "this should not appear");
    ASSERT_TRUE(msgs.empty());

    lcb_destroy(instance);

    lcb_logger_destroy(procs.base);
}
