void announce(int v) {
    print(v);
}

int twice(int x) {
    print(x);
    return x * 2;
}

int main() {
    announce(7);
    print(twice(21));
    return 0;
}
