#include <iostream>


void throw_exception()
{
	//throw "A testing exception...";
}

void bar()
{
	throw_exception();
}

void foo()
{
	bar();
}

int main(int argc, const char *argv[])
{
	std::cout << "====TEST EXCEPTIONS====" << std::endl;
	try{
		foo();
	}catch(const char *msg)
	{
		std::cout << "OK!" << std::endl;
	}
	return 0;
}
