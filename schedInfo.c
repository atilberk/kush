#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>

int processID = 0;
int processSPolicy = 0;
int processPrio = 0;

int schedinfo_init(void)
{
  struct task_struct *task;
  struct task_struct *task_child;
  struct list_head *list;

  struct sched_param param;

  printk(KERN_INFO "SchedInfo Module is Loading\n");

  for_each_process(task){
    if(task->pid == processID) {
      printk(KERN_INFO "Executable Name\t: %s\n", task->comm);
			printk(KERN_INFO "Process ID\t: %d\n", task->pid);
			printk(KERN_INFO "Priority\t\t: %d", task->prio);
      printk(KERN_INFO "Static Priority\t: %d\n", task->static_prio);
      printk(KERN_INFO "Parent PID\t: %d\n", task->parent->pid);
      printk(KERN_INFO "Time Slice\t: %d\n", task->rt.time_slice);
			printk(KERN_INFO "Policy\t\t: %u", task->policy);
			if (task->policy == 0)
				printk(" (SCHED_NORMAL)");
			else if (task->policy > 0)
        printk(" (SCHED_RR)");
      printk("\n");
      printk(KERN_INFO "User ID\t\t: %d\n", task->cred->uid);

      list_for_each(list, &task->children){
				task_child = list_entry(list, struct task_struct, sibling);
				printk(KERN_INFO "Child Executable Name\t: %s\n", task_child->comm);
				printk(KERN_INFO "Sibling Process ID\t: %d\n", task_child->pid);
			}
      printk(KERN_INFO "\n");

      param.sched_priority = processPrio;
      sched_setscheduler(task, processSPolicy, &param);
      printk(KERN_INFO "After changing process scheduler info\n");

      printk(KERN_INFO "Executable Name\t: %s\n", task->comm);
			printk(KERN_INFO "Process ID\t: %d\n", task->pid);
			printk(KERN_INFO "Priority\t\t: %d\n", task->prio);
      printk(KERN_INFO "Static Priority\t: %d\n", task->static_prio);
      printk(KERN_INFO "Parent PID\t: %d\n", task->parent->pid);
      printk(KERN_INFO "Time Slice\t: %d\n", task->rt.time_slice);
			printk(KERN_INFO "Policy\t\t: %u", task->policy);
			if (task->policy == 0)
				printk(" (SCHED_NORMAL)");
			else if (task->policy > 0)
        printk(" (SCHED_RR)");
      printk("\n");
      printk(KERN_INFO "User ID\t\t: %d\n", task->cred->uid);

      list_for_each(list, &task->children){
				task_child = list_entry(list, struct task_struct, sibling);
				printk(KERN_INFO "Child Executable Name\t: %s\n", task_child->comm);
				printk(KERN_INFO "Sibling Process ID\t: %d\n", task_child->pid);
			}
    }
  }
  return 0;
}

void schedinfo_exit(void)
{
  printk(KERN_INFO "Removing SchedInfo Module.\n");
}


/* Macros for registering module entry and exit points. */
module_init(schedinfo_init);
module_exit(schedinfo_exit);
module_param(processID, int, 0000);
module_param(processSPolicy, int, 0000);
module_param(processPrio, int, 0000);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("COMP304 P1p4 SchedInfo Module");
MODULE_AUTHOR("kush");
