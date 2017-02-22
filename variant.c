#include <stdio.h>
#include <math.h>
int main(int argc, char *argv[]){
  int randX,passport, countOfVariants, i, variant;
  passport = atoi(argv[1]);
  printf("Passport serie- %d\n", passport);
  countOfVariants = atoi(argv[2]);
  printf("Count of variants- %d\n", countOfVariants);
  for (i = 0; i < passport;i++){
    randX = rand(); 
  }
  variant = randX % countOfVariants + 1;
  printf("Your variant is %d\n", variant);
  return 0;
}
