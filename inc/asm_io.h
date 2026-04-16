void print_string();
char read_char();
void print_char();
int read_int();
void print_int();
void print_nl();

#define print_int(x) \
        __asm__( \
                "mov    %0, %%eax;" \
                : \
                : "r"(x) \
                : \
        ); \
        print_int();

#define print_char(x) \
        __asm__( \
                "mov    %0, %%al;" \
                : \
                : "r"(x) \
                : \
        ); \
        print_char();

#define print_string(x) \
        __asm__( \
                "mov    %0, %%eax;" \
                : \
                : "r"(x) \
                : \
        ); \
        print_string();