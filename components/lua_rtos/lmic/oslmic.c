/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

#include "luartos.h"

#if LUA_USE_LORA
#if USE_LMIC

#include "lmic.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_attr.h"

// LMIC run loop, as a FreeRTOS task
void os_runloop(void *pvParameters);

// Task handle for LMIC run loop
TaskHandle_t xRunLoop = NULL;

// RUNTIME STATE
static struct {
    osjob_t* scheduledjobs;
    osjob_t* runnablejobs;
} OS;

int os_init () {
    memset(&OS, 0x00, sizeof(OS));
    hal_init();
    if (radio_init() == 0) {
        LMIC_init();

    	// Run os_runloop in a FreeRTOS task
    	xTaskCreate(os_runloop, "lmic", tskDEFStack, NULL, tskDEF_PRIORITY, &xRunLoop);
    } else {
    	return -1;
    }

    return 0;
}

static u1_t unlinkjob (osjob_t** pnext, osjob_t* job) {
    for( ; *pnext; pnext = &((*pnext)->next)) {
        if(*pnext == job) { // unlink
            *pnext = job->next;
            return 1;
        }
    }
    return 0;
}

// clear scheduled job
void IRAM_ATTR os_clearCallback (osjob_t* job) {
    hal_disableIRQs();
    unlinkjob(&OS.scheduledjobs, job);
	unlinkjob(&OS.runnablejobs, job);
    hal_enableIRQs();
}

// schedule immediately runnable job
void IRAM_ATTR os_setCallback (osjob_t* job, osjobcb_t cb) {
    osjob_t** pnext;
    hal_disableIRQs();
    // remove if job was already queued
    os_clearCallback(job);
    // fill-in job
    job->func = cb;
    job->next = NULL;
    // add to end of run queue
    for(pnext=&OS.runnablejobs; *pnext; pnext=&((*pnext)->next));
    *pnext = job;
    hal_enableIRQs();

    hal_resume();
}

// schedule timed job
void os_setTimedCallback (osjob_t* job, ostime_t time, osjobcb_t cb) {
    osjob_t** pnext;
    hal_disableIRQs();
    // remove if job was already queued
    os_clearCallback(job);
    // fill-in job
    job->deadline = time;
    job->func = cb;
    job->next = NULL;
    // insert into schedule
    for(pnext=&OS.scheduledjobs; *pnext; pnext=&((*pnext)->next)) {
        if((*pnext)->deadline - time > 0) { // (cmp diff, not abs!)
            // enqueue before next element and stop
            job->next = *pnext;
            break;
        }
    }
    *pnext = job;
    hal_enableIRQs();

	hal_resume();
}

// LMIC run loop, as a FreeRTOS task
void os_runloop(void *pvParameters) {
	for(;;) {
	    osjob_t *j = NULL;

	    hal_disableIRQs();

	    // check for runnable jobs
		j = NULL;
	    if(OS.runnablejobs) {
	        j = OS.runnablejobs;
	        OS.runnablejobs = j->next;
	    } else if(OS.scheduledjobs && hal_checkTimer(OS.scheduledjobs->deadline)) { // check for expired timed jobs
	        j = OS.scheduledjobs;
	        OS.scheduledjobs = j->next;
	    }

	    hal_enableIRQs();

	    if (j) { // run job callback
	        j->func(j);
	    } else {
	    	if (!OS.scheduledjobs) {
	    		hal_sleep();
	    	}
	    }
	}
}

#endif
#endif
