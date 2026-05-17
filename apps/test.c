#include <xeapi.h>

int counter = 0;  // глобальная без RDATA — раньше не работало
char msg[] = "Hello from .data!";  // строка в .data

void entry(unsigned long long dummy, XornAPI* api) {
    // выведи адрес через первый указатель
    // просто вызови clear_screen — он хоть что-то делает?
    api->clear_screen(0xFF0000); // красный экран
    api->read_key();
}