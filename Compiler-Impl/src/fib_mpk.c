int fib_mpk(int n)
{
	if(n <= 0)return 0;
	if(n <= 2)return 1;
	return fib_mpk(n-1) + fib_mpk(n-2);
}
