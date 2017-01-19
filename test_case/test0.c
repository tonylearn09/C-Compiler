//void foo(int a);
int main()
{
    int i = 0, j, result = 0;
    const float a = 3.14;
    do {
        for (j = 0; i + j < 2 && j < 3; j = j + 1) {
            if (i == 1) {
                break;
            } else {
                if (j == 1) {
                    continue;
                }
            }
            result = result + j;
        }
        if (true) {
            i = i + 1;
        }
    } while ( i < 3); 
    while (i < 5 && result < 4) {
        i = i + 1;
        result = result + 1;
    }
    foo(i);
    print i;
    print "\n";
    print result;
    print "\n";
    print a;
    print "\n";
    return 0;
}

void foo(int a) {
    int i, result;
    result = 0;
}

