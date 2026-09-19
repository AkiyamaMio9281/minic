void countdown(int n) {
    while (n > 0) {
        n = n - 1;
    }

    return;
}

int sign(int x) {
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    else
        return 0;
}

int dangling(int a, int b) {
    // The else belongs to the inner if, as in C.
    if (a)
        if (b)
            return 1;
        else
            return 2;

    return 3;
}

int main(void) {
    int i = 0;
    ;
    {
        int i = 5;
        {}
    }

    while (i < 3) i = i + 1;

    countdown(sign(-4) + dangling(1, 0));
    return i;
}
