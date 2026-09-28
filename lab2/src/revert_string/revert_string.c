#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
    int len = strlen(str);           // длина строки (без '\0')

    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];          // сохранили символ с начала
        str[i] = str[len - 1 - i];   // на его место — символ с конца
        str[len - 1 - i] = temp;     // на место с конца — сохранённый
    }
}