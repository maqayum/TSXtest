/*
 * Copyright 2007 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed
 * in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <immintrin.h>
//#include <libitm.h>

#define LOOP_COUNT 100
#define NUM_THREADS 2	/* should be an even number */

using namespace std;

/* global counter */
int global_a;

/* global variable */
int global_var;

/* a function that can be called inside & outside transaction */
/*__attribute__((tm_callable))*/

//__attribute__((transaction_pure))
void thread_job1(void)
{
    int i;
    for (i = 0; i < LOOP_COUNT; ++i)
	global_var += i;
}

/* a function that can only be called outside transaction */
void thread_local_job2(void)
{
    int i, sum = 0;

    for (i = 0; i < LOOP_COUNT; ++i)
	sum += i;
}

void* thread_body(void* arg)
{
    int i, local_a;
    int tid = *((int*)arg);

    for (i = 0; i < LOOP_COUNT; ++i)
    {
      /* beginning of atomic section */
	/*__tm_atomic {*/
//	__transaction_atomic {
if (_xbegin() == _XBEGIN_STARTED) {
            /* read lock */
            /* transaction */

	    local_a = global_a;

	    /* this function can be called both inside
	     * and outside a transaction since it is
	     * annotated as tm_callable
	     */
	    thread_job1();

	    if (tid % 2 == 0)
	    {
		global_a = local_a + 1;
	    }
	    else
	    {
		global_a = local_a - 1;
	    }
            _xend();
	}
	/* end of atomic section */

	thread_local_job2();	/* can only be called outside transaction */
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    int	i;
    int tids[NUM_THREADS];
    pthread_t threads[NUM_THREADS];

    /* initialize global counter */
    global_a = 0;

    /* spawn threads */
    for (i = 0; i < NUM_THREADS; ++i)
    {
	tids[i] = i;
	pthread_create(&threads[i], NULL, thread_body, (void*)&tids[i]);
    }

    for (i = 0; i < NUM_THREADS; ++i)
    {
	pthread_join(threads[i], NULL);
    }

    /* print out global counter */
    cout << "global_a:"<< global_a << endl;
}

// vim: ts=8 sts=4 smarttab smartindent
