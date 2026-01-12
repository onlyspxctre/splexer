extern int printf(const char* format, ...);

int main() {
    int a;
    float b;
    a = 65;
    b = 66.0;

    if (a == b) {
        a + b;
        return 1;
    }
    else if (a > b) {
        a - b;
        return 2;
    }
    else if (a < b) {
        ++a;
        return 3;
    }
    else {
        --b;
        return 0;
    }
}
