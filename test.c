int main() {
  int i = 3;
  int n;

  // initialize first and second terms
  int t1 = 0;
  int t2 = 1;
  
  // initialize the next term (3rd term)
  int nextTerm = t1 + t2;
  
  // get no. of terms from user
  printf("Enter the number of terms: ");
  scanf("%d", &n);
  
  // print the first two terms t1 and t2
  printf("Fibonacci Series: %d, %d, ", t1, t2);
  
  do {
    i *= 2;
  } while (i < 10);

  // while (i < n) {
  //   printf("%d, ", nextTerm);
  //   t1 = t2;
  //   t2 = nextTerm;
  //   nextTerm = t1 + t2;
  // 
  //   i = i + 1;
  // }
  
  return 0;
}