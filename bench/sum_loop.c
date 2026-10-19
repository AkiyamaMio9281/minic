// A counted loop whose body touches two locals, which is the shape that
// shows stack slots turning into SSA values.
int sum(int n) {
    int acc = 0;
    int i = 0;

    while (i < n) {
        acc = acc + i;
        i = i + 1;
    }

    return acc;
}

int main() {
    print(sum(1000));
    return 0;
}
