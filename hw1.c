#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cpumask.h> 
#include <linux/sched.h>
#include <linux/sched/signal.h>   // for_each_process_thread
#include <linux/rcupdate.h>
#include <linux/rbtree.h>         // everything related to black tree

#define PROC_NAME "hw1"
#define STUDENT_ID "2026848368"
#define STUDENT_NAME "Simone Rea"
#define LINE_LENGHT 40

MODULE_AUTHOR("Simone, Rea");
MODULE_LICENSE("GPL");


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
    for_each_process_thread(g, p) {

        if (!is_fair(p)){
            continue;
	}
	/* I need to check that the task is actually on the runqueue and not in the waiting queue 
	 * and the task must not be the this process itself */
        if (p->on_rq && p != current && !p->on_cpu)
            fair_total++;
    }

    return fair_total;
}

/* Same as count_fair but we only consider tasks on (int *cpu) */
static int count_fair_on_cpu(int cpu)
{
    struct task_struct *g, *p;
    unsigned int fair_total = 0;

    for_each_process_thread(g, p) {

        if (!is_fair(p)){
            continue;
	}

	/* We check by comparing CPU value in p */
	/* We also check that the task is actually in the tree with RB_EMPTY_NODE */
        if (p->on_rq && p->cpu == cpu && p != current && !p->on_cpu)             
            fair_total++;
    }

    return fair_total;
}

static struct task_struct* pick_a_fair_from_cpu(int cpu)
{
    struct task_struct *g, *p;

    for_each_process_thread(g, p) {

        if (!is_fair(p)){
            continue;
	}

	/* We check by comparing CPU value in p 
	 * I want the process to have run_node defined */
        if (p->on_rq && p->cpu == cpu && !p->on_cpu){
            return p;
	}
    }

    return NULL;
}

static struct rb_root get_root(struct rb_node *node){
    struct rb_root root;

    while (rb_parent(node))
        node = rb_parent(node);

    root.rb_node = node;
    return root;
}

static void print_information_of_rb_node(struct seq_file *s, struct rb_node *node){
	struct sched_entity *se;
	struct sched_dl_entity *dl;
	struct task_struct *t;
	unsigned long color = node->__rb_parent_color;

	se = rb_entry(node, struct sched_entity, run_node);
	t  = container_of(se, struct task_struct, se);
	dl = &(t->dl);

        seq_printf(s, "%lld %lld %d ", dl->deadline, se->vruntime, t->pid);
        seq_printf(s, "%lld \n", color);

	
	return;
}
static void reverse_the_tree(struct seq_file *s, int cpu){

	/* I pick a random task from cpu so that I get the rb tree of cpu from it */
	struct rb_root root;
	struct rb_node *random_node, *curr;
	struct task_struct *random_task = pick_a_fair_from_cpu(cpu);


	/*
	if(current->cpu == cpu){
		print_information_of_rb_node(s, &current->se.run_node);
	}*/
	
	if (random_task == NULL)
		return;
	random_node = &random_task->se.run_node;
	root = get_root(random_node);
	for(curr = rb_first(&root); curr; curr = rb_next(curr))
		print_information_of_rb_node(s, curr);
	
	

}


static int print_rb_tree_informations(struct seq_file *s)
{
    int cpu;
    unsigned int queued_fair_tasks_in_cpu;

    for_each_online_cpu(cpu){
        queued_fair_tasks_in_cpu = count_fair_on_cpu(cpu);
	
        seq_printf(s, "[CPU #%d] %d tasks\n", cpu, queued_fair_tasks_in_cpu);

	reverse_the_tree(s, cpu);
      	
    	print_line(s);
    }
    return 0;
}

static int hello_seq_show(struct seq_file *s, void *v)
{
    int queued_fair_tasks_count;
    loff_t *spos = (loff_t *) v;
    seq_printf(s, "ID: %s\n", STUDENT_ID);
    seq_printf(s, "Name: %s\n", STUDENT_NAME);

    /* Use num_present_cpus() to read the number of CPUs. This function comes from cpumask.h */
    seq_printf(s, "Number of CPUs: %d\n", (int) num_present_cpus());

    rcu_read_lock();

    queued_fair_tasks_count =  count_fair(s);
    seq_printf(s, "Number of queued FAIR tasks: %d\n", queued_fair_tasks_count);

    print_line(s);
    
    print_rb_tree_informations(s);

    rcu_read_unlock();

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
