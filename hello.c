extern int printf(const char* __restrict __format, ...);

int main() {
    int a;
    int b;
    a = 65;
    b = 66;

    if (a == b) {
        return 1;
    } else {
        return 0;
    }
}
