/*
 * report.cc
 *
 *  Created on: 2013-08-29
 *      Author: akshayk
 */

#include "clients/watchpoints/clients/profiler/instrument.h"
#include "clients/watchpoints/clients/profiler/descriptor.h"


using namespace granary;

extern unsigned long app_access_counter[120000];
extern unsigned long host_access_counter[120000];


namespace client {

//extern wp::profiler_descriptor *DESCRIPTORS[client::wp::MAX_NUM_WATCHPOINTS];



/// Dump the watchpoints shadow information.
    void report(void) throw() {
        unsigned long index;
        //unsigned long counter[12][20][20][20][20] = {{{{{0,},},},},};
       // unsigned int j = 0;
        //unsigned int i = 0;

        printf("\nWatchpoint profile dump\n");

        for(index = 0; index < client::wp::MAX_NUM_WATCHPOINTS; index++){
            wp::profiler_descriptor *desc;
            desc = wp::profiler_descriptor::access(index);

            if(is_valid_address(desc)){
                if(desc->object_source) {
                    app_access_counter[desc->object_source*10000+desc->size] += desc->module_access_counter;
                    host_access_counter[desc->object_source*10000+desc->size] += desc->kernel_access_counter;
                   // counter[desc->object_source][0][0][0][0] = desc->object_source;
                    granary::printf(" report : source (%llx) app_count(%llx), host_count(%llx), size(%llx)\n",
                            desc->object_source, desc->module_access_counter, desc->kernel_access_counter, desc->size);
                }
            }
        }
        for(index=0; index < 120000; index++){
            if(app_access_counter[index] != 0){
                printf("source (%d), size(%d), app_access (%d)\n", index/10000, (index%10000), app_access_counter[index]);
            }

            if(host_access_counter[index] != 0){
                printf("source (%d), size(%d), host_access (%d)\n", index/10000, (index%10000), host_access_counter[index]);
            }
        }
    }
}


