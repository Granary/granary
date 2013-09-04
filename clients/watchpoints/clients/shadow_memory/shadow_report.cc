/*
 * shadow_report.cc
 *
 *  Created on: 2013-08-06
 *      Author: akshayk
 */

#include "granary/types.h"
#include "clients/watchpoints/clients/shadow_memory/descriptor.h"
#include "clients/watchpoints/clients/shadow_memory/instrument.h"



using namespace granary;


namespace client {

    extern wp::shadow_policy_descriptor *DESCRIPTORS[client::wp::MAX_NUM_WATCHPOINTS];

    /// Dump the watchpoints shadow information.
    void report(void) throw() {

        unsigned long index;
        uint16_t type;

        printf("\nWatchpoint shadow dumps\n");
        type = client::wp::get_inode_type_id();
        for(index = 0; index < client::wp::MAX_NUM_WATCHPOINTS; index++){
            wp::shadow_policy_descriptor *desc;
            desc = wp::shadow_policy_descriptor::access(index);

            if(is_valid_address(desc)){
                if(desc->state.is_active && desc->state.accessed_in_last_epoch){;
                    desc->state.accessed_in_last_epoch = false;
                    granary::printf("index (%llx) shadow_read(%llx), shadow_write(%llx)\t", index, *(desc->read_shadow), *(desc->write_shadow));
                }
            }
        }
    }
}



