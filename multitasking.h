typedef void (*task_func)();

typedef struct {
    task_func func;
    bool active;
    char desc[32];
} Task;
extern Task tasks[16];

extern int task_count;

int add_task(task_func func, char* description);
int check_task_existing_by_func(task_func func);
int get_active_process_by_func(task_func function);
int get_process_by_func(task_func function);
int enable_task(int task_num);
int disable_task(int task_num);
void run_scheduler();