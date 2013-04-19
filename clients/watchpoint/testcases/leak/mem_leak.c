
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

MODULE_LICENSE("Dual BSD/GPL");

#define MEMALLOC_SIZE 8

static int n_readtask = 1;
static int n_writetask = 1;
static DEFINE_SPINLOCK(sync_lock);

void *buffer;

//module_param(n_readtask, int, 0444);
//MODULE_PARAM_DESC(n_readtask, "Number of read task");

//module_param(n_writetask, int, 0444);
//MODULE_PARAM_DESC(n_writetask, "Number of write task");

static struct task_struct **read_task;
static struct task_struct **write_task;

struct memleak_ops {
	void (*init)(void*);
	void (*read)(void*);
	void (*write)(void*);
	void (*sync)(void*);
};

static void
func_init(void *arg){
	memset(arg, 0x1, MEMALLOC_SIZE);
}

static int
func_read(int *arg){
	return (*arg); 
}

static void
func_write(void *arg){
	memcpy(arg, buffer, MEMALLOC_SIZE);
}

static void
func_sync(int *arg){
	int i;
	spin_lock(&sync_lock);
	i = *arg;
	spin_unlock(&sync_lock);
	
}

static struct memleak_ops ops = {
	.init = func_init,
	.read = func_read,
	.write = func_write,
	.sync = func_sync
};


static void memset_function(void *arg){
	memset(arg, 0x0, 8);
}

static void *memalloc_function(unsigned int size){
	return kmalloc(size, GFP_KERNEL);
}

static void memfree_function(void *arg){
	kfree(arg);
}

static int
readtask_func(void *arg){
	void *ptr = NULL;
	while(1){
		if(ptr){
			printk("memleak 8 byte\n");
		}
		ptr = memalloc_function(MEMALLOC_SIZE);
		memset_function(ptr);
		ops.init(ptr);
		ops.read((int*)ptr);
		ops.sync((int)ptr);
		ops.write(ptr);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(10*HZ);
		memfree_function(ptr);
		ptr = NULL;
	}
	return 1;
}

static int
writetask_func(void *arg){
	void *ptr = NULL;
	while(1){
                if(ptr){
                        printk("memleak 8 byte\n");
                }
                ptr = memalloc_function(MEMALLOC_SIZE);
                ops.init(ptr);
                ops.read((int*)ptr);
                ops.sync((int)ptr);
                ops.write(ptr);
		if(ptr != NULL){
			printk("%lx\n", ptr);
		}
                set_current_state(TASK_INTERRUPTIBLE);
                schedule_timeout(100*HZ);

                if(((unsigned long)ptr)%2 == 0){
                        memfree_function(ptr);
                        ptr = NULL;
                }

	}
	return 1;
}

int
memleak_init(void){
	int result, i=0;
	buffer = kmalloc(MEMALLOC_SIZE, GFP_KERNEL);
	printk("buffer address : %lx\n", buffer);
	memset(buffer, 0x0, MEMALLOC_SIZE);
	read_task = kzalloc(n_readtask*sizeof(read_task[0]), GFP_KERNEL);
	for(i=0; i<n_readtask; i++){
		read_task[i] = kthread_run(readtask_func, NULL, "readtask");
	}

	write_task = kzalloc(n_writetask*sizeof(write_task[0]), GFP_KERNEL);

	for(i=0; i<n_writetask; i++){
		write_task[i] = kthread_run(writetask_func, NULL, "writetask");
	}

	return result;
}

void
memleak_exit(void){
	int i=0;
	if(read_task){
		for(i=0; i<n_readtask; i++){
			if(read_task[i]){
				kthread_stop(read_task[i]);
			}
		}
	}
	
	if(write_task){
		for(i=0; i<n_writetask; i++){
			if(write_task[i]){
				kthread_stop(write_task[i]);
			}
		}
	}

}

module_init(memleak_init);
module_exit(memleak_exit)

