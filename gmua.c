
/*GMUA Scheduler Module for ChronOS
 
 */

#include <linux/module.h>
#include <linux/chronos_types.h>
#include <linux/chronos_sched.h>
#include <linux/list.h>
#include <linux/chronos_global.h>


struct rt_info * sched_gmua(struct list_head *head, struct global_sched_domain *g)
{
        struct rt_info *it, *next, *cpu_head, *temp, *head_lvd;
	struct rt_info *head_final[NR_CPUS];        
	struct cpu_info *cpu;
        int processor, cpus, i;
//	printk(KERN_EMERG"nr_cpus, %d \n",NR_CPUS);	
	cpus = count_global_cpus(g);
//	printk(KERN_EMERG"count_global_cpu, %d \n",cpus);

	temp = NULL, cpu=NULL;
	for(i=0;i<NR_CPUS;i++)
		head_final[i]=NULL;
	
        list_for_each_entry_safe(it, next, head, task_list[GLOBAL_LIST]) {
            
                if(check_task_failure(it, SCHED_FLAG_NONE))
                	{
				_remove_task_global(it, g);
				continue;
			}
		livd(it, 0, SCHED_FLAG_NONE);
		INIT_LIST_HEAD(&it->task_list[SCHED_LIST1]);//DEADLINE ORDERED GLOBAL LIST	
		INIT_LIST_HEAD(&it->task_list[SCHED_LIST2]);//each cpu has its own schedule
		INIT_LIST_HEAD(&it->task_list[SCHED_LIST3]); //sort with lowest value density
		INIT_LIST_HEAD(&it->task_list[SCHED_LIST4]);//final list
		if(!temp)//initialize head very first time
			temp=it;
       		if(temp!=it)
		{
			if(insert_on_list(it, temp, SCHED_LIST4, SORT_KEY_DEADLINE, 0))//sched list 1 is deadline ordered list
				temp=it; //if the task is inserted on head..update the head
		}
	 }
	if(temp)
	{
		it = temp;
		initialize_cpu_state();

		do
		{
			processor=find_processor(cpus);			
			insert_cpu_task(it, processor);	
			update_cpu_exec_times(processor, it, 1);
			it=task_list_entry(it->task_list[SCHED_LIST4].next, SCHED_LIST4);
		}while(it!=temp);
//printk(KERN_EMERG"before for");
		for( i=0; i<cpus; i++)
		{
			cpu= get_cpu_state(i);
			cpu_head = cpu->head;
			head_lvd=cpu_head;
			it=cpu_head;
		//	if(i==0)
				// head_final=cpu_head;
//printk(KERN_EMERG"just before the feasibility check");
			 if(cpu_head)
			{
				do{
					if(it!=head_lvd)
					{
						if(insert_on_list(it, head_lvd, SCHED_LIST3, SORT_KEY_LVD, 0))
							head_lvd=it;
					}
				 	it=task_list_entry(it->task_list[SCHED_LIST2].next, SCHED_LIST2);
				}while(it!=head_lvd);//head_lvd=head of lvd ordered list

				
				while(!list_is_feasible(cpu_head, SCHED_LIST2) && !list_empty(&cpu_head->task_list[SCHED_LIST2]))
				{	
					
						list_remove(head_lvd, SCHED_LIST2);
						if(cpu_head==head_lvd)
                                                {
							cpu_head=task_list_entry(cpu_head->task_list[SCHED_LIST2].next, SCHED_LIST2);
                                                }
					temp=task_list_entry(head_lvd->task_list[SCHED_LIST3].next, SCHED_LIST3);//head_lvd is the task to be removed from lvd list
					//list_remove(head_lvd, SCHED_LIST3);
					//list_remove(head_lvd, SCHED_LIST2);  
					head_lvd = temp;
				}	
//printk(KERN_EMERG"lists are made feasible");
//			INIT_LIST_HEAD(&cpu_head->task_list[SCHED_LIST4]);
		//	cpu->best_deadline=cpu_head;	
		//	if(i!=0 && !list_empty(&cpu_head->task_list[SCHED_LIST2])) 
				//insert_on_list(cpu_head, head_final, SCHED_LIST4, SORT_KEY_NONE, 0);
			//else 
			//	break;	
//printk(KERN_EMERG"end of sched_list4");
			}	
			//else
			//	break;
			cpu->best_dead=cpu_head;
			head_final[i]=cpu_head;		
		}
	 build_list_array(head_final, cpus);	
 

	}
     return  head_final[0];
}

struct rt_sched_global gmua = {
        .base.name = "GMUA",
        .base.id = SCHED_RT_GMUA,
        .schedule = sched_gmua,
        .preschedule = presched_stw_generic,
        .arch = &rt_sched_arch_stw,
	.local = SCHED_RT_FIFO,
        .base.sort_key = SORT_KEY_DEADLINE,
        .base.list = LIST_HEAD_INIT(gmua.base.list)
};

static int __init gmua_init(void)
{
        return add_global_scheduler(&gmua);
}
module_init(gmua_init);

static void __exit gmua_exit(void)
{
        remove_global_scheduler(&gmua);
}
module_exit(gmua_exit);


                                                 
