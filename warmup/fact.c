#include "common.h"
#include <stdio.h>

int factRecursive(int n);
 
int main( int argc, char *argv[])
{
	int n;
	int result;
	double num;

	if(sscanf(argv[1],"%lf",&num)==1 && num == (double)(int)num ){
		n =(int) num;
		if(n>0 && n<13){
			result = factRecursive(n);
			printf("%d\n",result);
		}else if (n>=13)
			printf("Overflow\n");
		else if (n<=0)
			printf("Huh?\n");	
	}else{
		printf("Huh?\n");
	}
	
	/*if((scanf("%lf",&n) != 0)&& n==(float)(int)n && n<=(double)12 && n>0.0 ){ 
	result = factRecursive(n);
	printf("%d\n",(int)result);
	}else if(n>12.0){
	printf("Overflow\n");
	}else{
	printf("Huh?\n");
	}*/
	return 0;
}


int factRecursive(int n){

	if(n==0||n==1)
	return 1;

	return factRecursive(n-1)*n;

}


