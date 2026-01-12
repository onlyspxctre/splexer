extern int printf(const char* format, ...);

int main() {
    int a;
    int b;
    a = 65;
    b = 66;

    if (a == b) {
        return 1;
    }
    else if (a > b) {
        return 2;
    }
    else if (a < b) {
        return 3;
    }
    else {
        return 0;
    }
}
