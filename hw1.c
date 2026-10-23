#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cpumask.h> 
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>   // for_each_process_thread
#include <linux/rcupdate.h>

#define PROC_NAME "hw1"
#define STUDENT_ID "2026848368"
#define STUDENT_NAME "Simone Rea"
#define LINE_LENGHT 40

MODULE_AUTHOR("Simone, Rea");
MODULE_LICENSE("GPL");

/* FAIR processes have policy NORMAL, BATCH or IDLE */
static bool is_fair(struct task_struct *p)
{
    return p->policy == SCHED_NORMAL ||
           p->policy == SCHED_BATCH  ||
           p->policy == SCHED_IDLE;
}


static int count_fair(struct seq_file *s)
{
    struct task_struct *g, *p;
    unsigned int fair_total = 0;

    /* Need to get the lock when reading the list */
    rcu_read_lock();
    for_each_process_thread(g, p) {

        if (!is_fair(p)){
            continue;
	}
	/* I need to check that the task is actually on the runqueue and not in the waiting queue 
	 * and the task must not be the this process itself */
        if (p->on_rq && p->pid != current->pid)             
            fair_total++;
    }
    rcu_read_unlock();

    return fair_total;
}

static void print_line(struct seq_file *s){
    int i = 0;
    while(i++ < LINE_LENGHT)
	seq_printf(s,"-");
    seq_printf(s,"\n");
}

static void *hello_seq_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter = 0;
    if (*pos == 0)
        return &counter;
    else {
        *pos = 0;
        return NULL;
    }
}

static void *hello_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    unsigned long *tmp_v = (unsigned long *)v;
    (*tmp_v)++;
    (*pos)++;
    return NULL;
}

static void hello_seq_stop(struct seq_file *s, void *v)
{
    // nothing to do
}

static int hello_seq_show(struct seq_file *s, void *v)
{
    loff_t *spos = (loff_t *) v;
    seq_printf(s, "ID: %s\n", STUDENT_ID);
    seq_printf(s, "Name: %s\n", STUDENT_NAME);

    /* Use num_present_cpus() to read the number of CPUs. This function comes from cpumask.h */
    seq_printf(s, "Number of CPUs: %d\n", (int) num_present_cpus());

    int *queued_fair_tasks_count = (int *) count_fair(s);
    seq_printf(s, "Number of queued FAIR tasks: %d\n", (int) queued_fair_tasks_count);

    print_line(s);
    return 0;
}

static struct seq_operations hello_seq_ops = {
    .start = hello_seq_start,
    .next = hello_seq_next,
    .stop = hello_seq_stop,
    .show = hello_seq_show
};

static int hello_proc_open(struct inode *inode, struct file *file) {
    return seq_open(file, &hello_seq_ops);
}

static const struct proc_ops hello_proc_ops = {
    .proc_open = hello_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

static int __init hello_init(void) {
    struct proc_dir_entry *proc_file_entry;
    proc_file_entry = proc_create(PROC_NAME, 0, NULL, &hello_proc_ops);
    return 0; 
}

static void __exit hello_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(hello_init);
module_exit(hello_exit);
