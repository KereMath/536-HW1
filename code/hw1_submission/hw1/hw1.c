#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include <sys/mman.h>

#include <time.h>
#include <sys/time.h>

#define _BSD_SOURCE

#include <limits.h>



extern int heap_insert(int nodeindex);
extern int heap_pop(void);
extern int heap_delete(int nodeindex);
extern int *heap_size_ptr;  

struct Node {
    double key;
    int heap_index;
};

extern struct Node *GLB;
extern int *HEAP_ARRAY;

#define heap_size (*heap_size_ptr)



#define MAX_CUSTOMERS 1024  
#define MAX_TOOLS 100
#define BUFFER_SIZE 4096
#define NIL -1


typedef enum {
    CUSTOMER_STATE_RESTING,
    CUSTOMER_STATE_WAITING,
    CUSTOMER_STATE_USING,
    CUSTOMER_STATE_DELETED
} CustomerState;

typedef enum {
    EVENT_NONE = 0,
    EVENT_TOOL_ASSIGNED,
    EVENT_TOOL_REMOVED,
    EVENT_TOOL_COMPLETED
} EventType;


typedef struct {
    int customer_id;
    int is_allocated;

    CustomerState state;
    double share;

    int request_duration;
    int remaining_duration;

    int current_tool;
    long long session_start;

    long long wait_start;
    int heap_index;

    pthread_cond_t agent_cond;
    int event_pending;
    EventType event_type;
    int event_tool_id;
} Customer;

typedef struct {
    int tool_id;
    long long total_usage;

    int current_user;
    int current_usage;
    long long session_start;

    pthread_cond_t tool_cond;
    int tool_should_exit;
} ToolInfo;

typedef struct {
    Customer customers[MAX_CUSTOMERS];
    int free_customer_slots[MAX_CUSTOMERS];
    int free_customer_count;

    int waiting_count;

    ToolInfo tools[MAX_TOOLS];
    int num_tools;

    int total_customers;
    int resting_customers;
    double total_share;

    pthread_mutex_t global_mutex;
    pthread_cond_t new_customer_cond;

    int q;
    int Q;
    long long start_time;

    int server_should_exit;  

    struct Node heap_nodes[MAX_CUSTOMERS];
    int heap_array[MAX_CUSTOMERS];
    int heap_size_value;  
} SharedMemory;

typedef struct {
    int socket_fd;
    int customer_idx;
} AgentContext;



static SharedMemory *shm = NULL;
static int server_socket = -1;
static volatile int should_exit = 0;
static char socket_path[256] = {0};  


long long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}


void setup_shared_memory(int q, int Q, int k) {
    size_t shm_size = sizeof(SharedMemory);
    shm = (SharedMemory *)mmap(NULL, shm_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS,
                               -1, 0);

    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    memset(shm, 0, sizeof(SharedMemory));

    shm->q = q;
    shm->Q = Q;
    shm->num_tools = k;
    shm->start_time = get_current_time_ms();
    shm->server_should_exit = 0;

    shm->free_customer_count = MAX_CUSTOMERS;
    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        shm->free_customer_slots[i] = i;
        shm->customers[i].is_allocated = 0;
        shm->customers[i].customer_id = -1;
        shm->customers[i].current_tool = -1;
        shm->customers[i].heap_index = -1;
        shm->customers[i].state = CUSTOMER_STATE_DELETED;
    }

    for (int i = 0; i < k; i++) {
        shm->tools[i].tool_id = i;
        shm->tools[i].current_user = -1;
        shm->tools[i].total_usage = 0;
        shm->tools[i].current_usage = 0;
        shm->tools[i].tool_should_exit = 0;
    }

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->global_mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shm->new_customer_cond, &cond_attr);

    for (int i = 0; i < k; i++) {
        pthread_cond_init(&shm->tools[i].tool_cond, &cond_attr);
    }

    pthread_condattr_destroy(&cond_attr);

    GLB = shm->heap_nodes;
    HEAP_ARRAY = shm->heap_array;
    heap_size_ptr = &shm->heap_size_value;  
    shm->heap_size_value = 0;

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        shm->heap_nodes[i].key = 0.0;
        shm->heap_nodes[i].heap_index = NIL;
    }
}


int allocate_customer(int customer_id) {
    if (shm->free_customer_count <= 0) {
        return -1;
    }

    int idx = shm->free_customer_slots[--shm->free_customer_count];
    Customer *c = &shm->customers[idx];

    c->is_allocated = 1;
    c->customer_id = customer_id;
    c->state = CUSTOMER_STATE_RESTING;

    if (shm->total_customers > 0) {
        c->share = shm->total_share / (double)shm->total_customers;
    } else {
        c->share = 0.0;  
    }

    c->request_duration = 0;
    c->remaining_duration = 0;
    c->current_tool = -1;
    c->session_start = 0;
    c->wait_start = 0;
    c->heap_index = idx;
    c->event_pending = 0;
    c->event_type = EVENT_NONE;

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&c->agent_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    shm->total_customers++;
    shm->resting_customers++;
    shm->total_share += c->share;

    return idx;
}

void deallocate_customer(int customer_idx) {
    Customer *c = &shm->customers[customer_idx];

    if (!c->is_allocated) return;

    if (c->state == CUSTOMER_STATE_RESTING) {
        shm->resting_customers--;
    } else if (c->state == CUSTOMER_STATE_WAITING) {
        if (GLB[customer_idx].heap_index != NIL) {
            heap_delete(customer_idx);
        }
        shm->waiting_count--;
    } else if (c->state == CUSTOMER_STATE_USING) {
        if (c->current_tool >= 0 && c->current_tool < shm->num_tools) {
            shm->tools[c->current_tool].current_user = -1;
            shm->tools[c->current_tool].current_usage = 0;
        }
    }

    shm->total_customers--;
    shm->total_share -= c->share;

    c->is_allocated = 0;
    c->state = CUSTOMER_STATE_DELETED;
    c->customer_id = -1;
    c->current_tool = -1;

    shm->free_customer_slots[shm->free_customer_count++] = customer_idx;
}


int find_free_tool(void) {
    int best_tool = -1;
    long long min_usage = LLONG_MAX;

    for (int i = 0; i < shm->num_tools; i++) {
        if (shm->tools[i].current_user == -1) {
            if (shm->tools[i].total_usage < min_usage ||
                (shm->tools[i].total_usage == min_usage && (best_tool == -1 || i < best_tool))) {
                min_usage = shm->tools[i].total_usage;
                best_tool = i;
            }
        }
    }

    return best_tool;
}

int find_preemption_candidate(double new_share) {
    int candidate = -1;
    int max_usage = 0;

    for (int i = 0; i < shm->num_tools; i++) {
        int user_idx = shm->tools[i].current_user;
        if (user_idx != -1) {
            int usage = shm->tools[i].current_usage;
            if (usage > max_usage || 
                (usage == max_usage && (candidate == -1 || i < candidate))) {
                max_usage = usage;
                candidate = i;
            }
        }
    }

    if (candidate == -1) return -1;

    int user_idx = shm->tools[candidate].current_user;
    Customer *c = &shm->customers[user_idx];

    if (c->share < new_share) {
        return -1;
    }

    if (shm->tools[candidate].current_usage < shm->q) {
        return -1;
    }

    return candidate;
}

int find_max_share_tool_above_q(void) {
    int max_tool = -1;
    double max_share = -1.0;

    for (int i = 0; i < shm->num_tools; i++) {
        int user_idx = shm->tools[i].current_user;
        if (user_idx != -1) {
            ToolInfo *t = &shm->tools[i];
            Customer *c = &shm->customers[user_idx];
            
            if (t->current_usage >= shm->q) {
                if (c->share > max_share ||
                    (c->share == max_share && (max_tool == -1 || i < max_tool))) {
                    max_share = c->share;
                    max_tool = i;
                }
            }
        }
    }

    return max_tool;
}

void assign_tool_to_customer(int customer_idx, int tool_id) {
    Customer *c = &shm->customers[customer_idx];
    ToolInfo *t = &shm->tools[tool_id];

    if (c->state == CUSTOMER_STATE_WAITING) {
        if (GLB[customer_idx].heap_index != NIL) {
            heap_delete(customer_idx);
        }
        shm->waiting_count--;
    }

    c->state = CUSTOMER_STATE_USING;
    c->current_tool = tool_id;
    c->session_start = get_current_time_ms();

    t->current_user = customer_idx;
    t->current_usage = 0;
    t->session_start = c->session_start;

    c->event_pending = 1;
    c->event_type = EVENT_TOOL_ASSIGNED;
    c->event_tool_id = tool_id;
    pthread_cond_signal(&c->agent_cond);

    printf("Customer %d with share %d is assigned to the tool %d.\n",
           c->customer_id, (int)c->share, tool_id);
    fflush(stdout);
}

void remove_customer_from_tool(int customer_idx, EventType event) {
    Customer *c = &shm->customers[customer_idx];
    if (c->current_tool == -1) return;

    ToolInfo *t = &shm->tools[c->current_tool];

    long long now = get_current_time_ms();
    int usage = (int)(now - c->session_start);
    c->share += usage;
    shm->total_share += usage;
    t->total_usage += usage;

    if (event == EVENT_TOOL_REMOVED) {
        printf("Customer %d with share %d is removed from the tool %d.\n",
               c->customer_id, (int)c->share, c->current_tool);
    } else {
        printf("Customer %d with share %d leaves the tool %d.\n",
               c->customer_id, (int)c->share, c->current_tool);
    }
    fflush(stdout);

    t->current_user = -1;
    t->current_usage = 0;
    c->current_tool = -1;

    c->event_pending = 1;
    c->event_type = event;
    c->event_tool_id = t->tool_id;
    pthread_cond_signal(&c->agent_cond);
}

void assign_next_from_queue(int tool_id) {
    if (heap_size <= 0) return;

    int next_idx = heap_pop();
    if (next_idx != NIL && next_idx >= 0) {
        assign_tool_to_customer(next_idx, tool_id);
    }
}


void handle_request(int customer_idx, int duration) {
    pthread_mutex_lock(&shm->global_mutex);

    Customer *c = &shm->customers[customer_idx];

    if (c->state == CUSTOMER_STATE_RESTING) {
        shm->resting_customers--;
    } else if (c->state == CUSTOMER_STATE_WAITING) {
        if (GLB[customer_idx].heap_index != NIL) {
            heap_delete(customer_idx);
        }
        shm->waiting_count--;
    }

    c->request_duration = duration;
    c->remaining_duration = duration;

    int tool = find_free_tool();
    if (tool != -1) {
        assign_tool_to_customer(customer_idx, tool);
    } else {
        tool = find_preemption_candidate(c->share);
        if (tool != -1) {
            int old_user = shm->tools[tool].current_user;
            remove_customer_from_tool(old_user, EVENT_TOOL_REMOVED);

            Customer *old_c = &shm->customers[old_user];
            old_c->state = CUSTOMER_STATE_WAITING;
            old_c->wait_start = get_current_time_ms();
            GLB[old_user].key = old_c->share;
            heap_insert(old_user);
            shm->waiting_count++;

            assign_tool_to_customer(customer_idx, tool);
        } else {
            c->state = CUSTOMER_STATE_WAITING;
            c->wait_start = get_current_time_ms();
            GLB[customer_idx].key = c->share;
            heap_insert(customer_idx);
            shm->waiting_count++;
            
            if (heap_size > 0) {
                int min_waiter_idx = HEAP_ARRAY[0];
                if (min_waiter_idx >= 0 && min_waiter_idx < MAX_CUSTOMERS) {
                    Customer *min_waiter = &shm->customers[min_waiter_idx];
                    
                    int max_tool = find_max_share_tool_above_q();
                    if (max_tool != -1) {
                        int max_user = shm->tools[max_tool].current_user;
                        Customer *max_customer = &shm->customers[max_user];
                        
                        if (min_waiter->share < max_customer->share) {
                            pthread_cond_signal(&shm->tools[max_tool].tool_cond);
                        }
                    }
                }
            }
        }
    }

    pthread_cond_broadcast(&shm->new_customer_cond);

    pthread_mutex_unlock(&shm->global_mutex);
}

void handle_rest(int customer_idx) {
    pthread_mutex_lock(&shm->global_mutex);

    Customer *c = &shm->customers[customer_idx];

    if (c->state == CUSTOMER_STATE_USING) {
        int tool_id = c->current_tool;
        remove_customer_from_tool(customer_idx, EVENT_TOOL_COMPLETED);
        if (tool_id != -1) {
            assign_next_from_queue(tool_id);
        }
        c->state = CUSTOMER_STATE_RESTING;
        shm->resting_customers++;
    } else if (c->state == CUSTOMER_STATE_WAITING) {
        if (GLB[customer_idx].heap_index != NIL) {
            heap_delete(customer_idx);
        }
        shm->waiting_count--;
        c->state = CUSTOMER_STATE_RESTING;
        shm->resting_customers++;
    } else if (c->state == CUSTOMER_STATE_RESTING) {
    }

    pthread_mutex_unlock(&shm->global_mutex);
}

void handle_report(int socket_fd) {
    pthread_mutex_lock(&shm->global_mutex);

    char buffer[BUFFER_SIZE * 10];
    int offset = 0;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "k: %d, customers: %d waiting, %d resting, %d in total\n",
                      shm->num_tools, shm->waiting_count,
                      shm->resting_customers, shm->total_customers);

    double avg_share = (shm->total_customers > 0) ?
                       (shm->total_share / shm->total_customers) : 0.0;
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "average share: %.2f\n", avg_share);

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "waiting list:\n");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "customer   duration  share\n");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "---------------------------\n");

    long long now = get_current_time_ms();

    typedef struct { int id; int duration; int share; } WaitEntry;
    WaitEntry wait_list[MAX_CUSTOMERS];
    int wait_count = 0;

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        Customer *c = &shm->customers[i];
        if (c->is_allocated && c->state == CUSTOMER_STATE_WAITING) {
            wait_list[wait_count].id = c->customer_id;
            wait_list[wait_count].duration = (int)(now - c->wait_start);
            wait_list[wait_count].share = (int)c->share;
            wait_count++;
        }
    }

    for (int i = 0; i < wait_count - 1; i++) {
        for (int j = i + 1; j < wait_count; j++) {
            if (wait_list[j].share < wait_list[i].share) {
                WaitEntry tmp = wait_list[i];
                wait_list[i] = wait_list[j];
                wait_list[j] = tmp;
            }
        }
    }

    for (int i = 0; i < wait_count; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "%-12d %10d %12d\n",
                          wait_list[i].id, wait_list[i].duration, wait_list[i].share);
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "\nTools:\n");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "id   totaluse currentuser share duration\n");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "--------------\n");

    for (int i = 0; i < shm->num_tools; i++) {
        ToolInfo *t = &shm->tools[i];
        if (t->current_user == -1) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%-5d %12d FREE\n", 
                              i, (int)t->total_usage);
        } else {
            Customer *c = &shm->customers[t->current_user];
            
            long long now_tool = get_current_time_ms();
            int current = (int)(now_tool - t->session_start);
            
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%-5d %12d %-12d %10d %12d\n",
                              i, 
                              (int)(t->total_usage + current),  
                              c->customer_id,
                              (int)c->share,
                              c->remaining_duration);  
        }
    }

    pthread_mutex_unlock(&shm->global_mutex);

    send(socket_fd, buffer, offset, 0);
}


void tool_process(int tool_id) {
    if (server_socket >= 0) {
        close(server_socket);
    }

    while (1) {
        pthread_mutex_lock(&shm->global_mutex);

        if (shm->server_should_exit) {
            pthread_mutex_unlock(&shm->global_mutex);
            break;
        }

        ToolInfo *t = &shm->tools[tool_id];

        if (t->current_user == -1) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            pthread_cond_timedwait(&shm->new_customer_cond,
                                   &shm->global_mutex, &ts);
        } else {
            Customer *c = &shm->customers[t->current_user];
            long long now = get_current_time_ms();
            int elapsed = (int)(now - t->session_start);

            t->current_usage = elapsed;

            int remaining = c->request_duration - elapsed;
            c->remaining_duration = (remaining < 0) ? 0 : remaining;

            if (c->remaining_duration <= 0) {
                remove_customer_from_tool(t->current_user, EVENT_TOOL_COMPLETED);
                c->state = CUSTOMER_STATE_RESTING;
                shm->resting_customers++;
                assign_next_from_queue(tool_id);
            } else if (elapsed >= shm->Q) {
                if (heap_size > 0) {
                    int old_user = t->current_user;
                    remove_customer_from_tool(old_user, EVENT_TOOL_REMOVED);

                    Customer *old_c = &shm->customers[old_user];
                    old_c->state = CUSTOMER_STATE_WAITING;
                    old_c->wait_start = get_current_time_ms();
                    GLB[old_user].key = old_c->share;
                    heap_insert(old_user);
                    shm->waiting_count++;

                    assign_next_from_queue(tool_id);
                }
            } else if (elapsed >= shm->q && heap_size > 0) {
                int min_idx = HEAP_ARRAY[0];
                if (min_idx >= 0 && min_idx < MAX_CUSTOMERS) {
                    Customer *waiting = &shm->customers[min_idx];
                    if (waiting->share < c->share) {
                        int old_user = t->current_user;
                        remove_customer_from_tool(old_user, EVENT_TOOL_REMOVED);

                        Customer *old_c = &shm->customers[old_user];
                        old_c->state = CUSTOMER_STATE_WAITING;
                        old_c->wait_start = get_current_time_ms();
                        GLB[old_user].key = old_c->share;
                        heap_insert(old_user);
                        shm->waiting_count++;

                        assign_next_from_queue(tool_id);
                    }
                }
            }
        }

        pthread_mutex_unlock(&shm->global_mutex);
        usleep(10000);
    }
}


void *agent_socket_thread(void *arg) {
    AgentContext *ctx = (AgentContext *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(ctx->socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0) {
            break;
        }

        char *cmd = strtok(buffer, " \r\n");
        if (!cmd) continue;

        if (strcmp(cmd, "REQUEST") == 0) {
            char *duration_str = strtok(NULL, " \r\n");
            if (duration_str) {
                int duration = atoi(duration_str);
                if (duration > 0) {
                    handle_request(ctx->customer_idx, duration);
                }
            }
        } else if (strcmp(cmd, "REST") == 0) {
            handle_rest(ctx->customer_idx);
        } else if (strcmp(cmd, "REPORT") == 0) {
            handle_report(ctx->socket_fd);
        } else if (strcmp(cmd, "QUIT") == 0) {
            break;
        }
    }

    return NULL;
}

void *agent_notify_thread(void *arg) {
    AgentContext *ctx = (AgentContext *)arg;
    Customer *c = &shm->customers[ctx->customer_idx];
    char buffer[BUFFER_SIZE];

    while (1) {
        pthread_mutex_lock(&shm->global_mutex);

        while (!c->event_pending && c->is_allocated) {
            pthread_cond_wait(&c->agent_cond, &shm->global_mutex);
        }

        if (!c->is_allocated) {
            pthread_mutex_unlock(&shm->global_mutex);
            break;
        }

        if (c->event_pending) {
            int event_type = c->event_type;
            int tool_id = c->event_tool_id;
            int share = (int)c->share;
            int customer_id = c->customer_id;
            c->event_pending = 0;

            pthread_mutex_unlock(&shm->global_mutex);

            if (event_type == EVENT_TOOL_ASSIGNED) {
                snprintf(buffer, sizeof(buffer),
                    "Customer %d with share %d is assigned to the tool %d.\n",
                    customer_id, share, tool_id);
            } else if (event_type == EVENT_TOOL_REMOVED) {
                snprintf(buffer, sizeof(buffer),
                    "Customer %d with share %d is removed from the tool %d.\n",
                    customer_id, share, tool_id);
            } else if (event_type == EVENT_TOOL_COMPLETED) {
                snprintf(buffer, sizeof(buffer),
                    "Customer %d with share %d leaves the tool %d.\n",
                    customer_id, share, tool_id);
            }

            ssize_t sent = send(ctx->socket_fd, buffer, strlen(buffer), MSG_NOSIGNAL);
            if (sent < 0) {
                break;
            }
        } else {
            pthread_mutex_unlock(&shm->global_mutex);
            break;
        }
    }

    return NULL;
}

void agent_process(int client_socket) {
    pthread_mutex_lock(&shm->global_mutex);
    int customer_idx = allocate_customer(getpid());
    pthread_mutex_unlock(&shm->global_mutex);

    if (customer_idx < 0) {
        close(client_socket);
        exit(1);
    }

    AgentContext ctx;
    ctx.socket_fd = client_socket;
    ctx.customer_idx = customer_idx;

    pthread_t socket_thread, notify_thread;
    pthread_create(&socket_thread, NULL, agent_socket_thread, &ctx);
    pthread_create(&notify_thread, NULL, agent_notify_thread, &ctx);

    pthread_join(socket_thread, NULL);

    pthread_mutex_lock(&shm->global_mutex);
    Customer *c = &shm->customers[customer_idx];
    
    if (c->state == CUSTOMER_STATE_USING && c->current_tool != -1) {
        int tool_id = c->current_tool;
        remove_customer_from_tool(customer_idx, EVENT_TOOL_COMPLETED);
        assign_next_from_queue(tool_id);
    }
    
    c->is_allocated = 0;
    pthread_cond_signal(&c->agent_cond);
    pthread_mutex_unlock(&shm->global_mutex);
    
    pthread_join(notify_thread, NULL);
    
    pthread_mutex_lock(&shm->global_mutex);
    pthread_cond_destroy(&c->agent_cond);
    deallocate_customer(customer_idx);
    pthread_mutex_unlock(&shm->global_mutex);

    close(client_socket);
}


int create_server_socket(const char *conn_str) {
    int sock;

    if (conn_str[0] == '@') {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        
        const char *path = conn_str + 1;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
        
        strncpy(socket_path, path, sizeof(socket_path) - 1);

        unlink(addr.sun_path);

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(1);
        }

        printf("Server listening on Unix socket: %s\n", addr.sun_path);
    } 
    else if (strchr(conn_str, ':')) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            exit(1);
        }

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        char ip_str[256];
        strncpy(ip_str, conn_str, sizeof(ip_str) - 1);
        ip_str[sizeof(ip_str) - 1] = '\0';

        char *colon = strchr(ip_str, ':');
        if (!colon) {
            fprintf(stderr, "Invalid TCP format. Expected IP:port\n");
            exit(1);
        }

        *colon = '\0';
        char *ip = ip_str;
        int port = atoi(colon + 1);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
            addr.sin_addr.s_addr = INADDR_ANY;
        }

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(1);
        }

        printf("Server listening on TCP socket: %s:%d\n", ip, port);
    }
    else {
        fprintf(stderr, "Invalid connection string format\n");
        exit(1);
    }

    if (listen(sock, 128) < 0) {
        perror("listen");
        exit(1);
    }

    printf("k=%d, q=%d, Q=%d\n", shm->num_tools, shm->q, shm->Q);
    fflush(stdout);

    return sock;
}

void signal_handler(int sig) {
    (void)sig;
    should_exit = 1;
    
    if (shm) {
        pthread_mutex_lock(&shm->global_mutex);
        shm->server_should_exit = 1;
        pthread_cond_broadcast(&shm->new_customer_cond);
        pthread_mutex_unlock(&shm->global_mutex);
    }
    
    if (server_socket != -1) {
        close(server_socket);
    }
    if (socket_path[0] != '\0') {
        unlink(socket_path);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s conn q Q k\n", argv[0]);
        fprintf(stderr, "  conn: @/path/to/socket (Unix) or IP:port (TCP)\n");
        fprintf(stderr, "  q: minimum tool usage limit (ms)\n");
        fprintf(stderr, "  Q: maximum tool usage limit (ms)\n");
        fprintf(stderr, "  k: number of tools\n");
        exit(1);
    }

    const char *conn_str = argv[1];
    int q = atoi(argv[2]);
    int Q = atoi(argv[3]);
    int k = atoi(argv[4]);

    if (q <= 0 || Q <= 0 || k <= 0 || k > MAX_TOOLS) {
        fprintf(stderr, "Invalid parameters\n");
        exit(1);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, SIG_IGN);

    setup_shared_memory(q, Q, k);
    server_socket = create_server_socket(conn_str);

    for (int i = 0; i < k; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            tool_process(i);
            exit(0);
        }
    }

    while (!should_exit) {
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket,
                                  (struct sockaddr *)&client_addr,
                                  &addr_len);

        if (client_socket < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_socket);
            agent_process(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    close(server_socket);
    if (socket_path[0] != '\0') {
        unlink(socket_path);
    }

    while (wait(NULL) > 0);

    if (shm) {
        pthread_mutex_destroy(&shm->global_mutex);
        pthread_cond_destroy(&shm->new_customer_cond);
        for (int i = 0; i < shm->num_tools; i++) {
            pthread_cond_destroy(&shm->tools[i].tool_cond);
        }
        munmap(shm, sizeof(SharedMemory));
    }

    return 0;
}