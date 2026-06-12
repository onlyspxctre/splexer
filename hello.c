#            include <stdbool.h>

extern int printf(const char* format, ...);

int main() {
    int a;
    float b;
    a = 65;
    b = 66.0;
    b = 67.0f;
    b = .2f;
    b = .2f5;
    "werwer"notsupposedtobeinstring;
    '1234 i  i i dfsdf\'#$##$fant\n';

    if (a == b) {
        a + b;
        return 1;
    }
    else if (a>b) {
        a-b;
        return 2;
    }
    else if (a < b) {
        ++a;
        a ++
            ;
        return 3;
    }
    else {
        --b;
        return 0;
    }
}
