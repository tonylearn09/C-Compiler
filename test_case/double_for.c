int main()
{
    int total = 0;
    int i, j;
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            if (i == 0) {
                total = total + j;
            } 
        }
    }
    print total;
    print "\n";
    return 0;
}
