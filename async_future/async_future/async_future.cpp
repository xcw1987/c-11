// async_future.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <time.h>
#include<thread>
#include<future>
using namespace std;
class TimeCost
{
public:
	TimeCost(std::string str=".." )
	{
		prfX = str;
		m_time = clock();
	}

	~TimeCost()
	{
		std::cout << "pre_str :" << prfX.c_str() <<  ", time_cost: " << clock() - m_time << std::endl;
	}


private:
	clock_t  m_time;
	std::string prfX;
};


long  doSomeThing(long end, int chu =1)
{
	//std::cout << "dosomething: " << std::endl;
	std::this_thread::sleep_for(chrono::microseconds(1000));
	long  sum = 0;
	int rr = sum / chu;
	for (int i = 0; i < end; i++)
	{
		sum += i;
		std::this_thread::sleep_for(chrono::microseconds(1));
	}
	return sum;
}

void test1()
{
	TimeCost T1("test1");
	long sum = doSomeThing(4000) + doSomeThing(4000);
	std::cout << "sum: " << sum << std::endl;
}

void test2()
{
	TimeCost T1("test2");
	//std::future<long> result1(std::async([=](long end, int chu){
	//	std::cout << "dosomething: " << std::endl;
	//	long  sum = 0;
	//	int rr = sum / chu;
	//	for (int i = 0; i < end; i++)
	//	{
	//		sum += i;
	//		std::this_thread::sleep_for(chrono::microseconds(1));
	//	}
	//	return sum;
	//}, 4000, 1));

	std::future<long> result1(std::async(doSomeThing, 4000, 1));  //get只能调用一次，wait可以调用多次
	//std::async(std::launch::async, doSomeThing, 4000, 1);  //使异步必须执行， 如果不赋值给future，则相当于同步
	//std::async(std::launch::deferred, doSomeThing, 4000, 1);//延迟调用
	long result2 = doSomeThing(4000);
	long sum = 0;
	if (result1.wait_for(std::chrono::seconds(5)) == std::future_status::ready)
	{
		std::cout << "result1  run over" << std::endl;
		sum = result1.get() + result2;
	}
	else if (result1.wait_for(std::chrono::seconds(5)) == std::future_status::deferred)
	{
		std::cout << "result1  dont  start run" << std::endl;
	}
	
	if(result1.valid())
	result1.get();
	std::cout << "sum: " << sum << std::endl;
}


long  quickCompute(std::shared_future<long> f, int n)
{
	std::cout << "input_data: " << n  << std::endl;
	TimeCost T1("quickCompute");
	long result = 0;
	if (f.wait_for(std::chrono::seconds(5)) != std::future_status::deferred)
	{
		long ret = f.get();
		for (int i = 0; i < n; i++)
		{
			ret = ret - i;
			std::this_thread::sleep_for(chrono::microseconds(1));
		}
		result = ret;
	}

	return result;
}

void  test3()
{
	try {

		//std::shared_future<long> f = std::async(doSomeThing, 4000, 1);
	    auto f = std::async(std::launch::async, doSomeThing, 4000, 1).share();
		std::future<long> f1 = std::async(std::launch::async, quickCompute, f, 1000);
		std::future<long> f2 = std::async(std::launch::async, quickCompute, f, 2000);
		std::future<long> f3 = std::async(std::launch::async, quickCompute, f, 3000);
		f1.get();
		f2.get();
		f3.get();

	}
	catch (const std::exception& a)
	{
		std::cerr << "error: " << a.what() << std::endl;
	}

}


int main()
{
    std::cout << "Hello World!\n";
	test1();
	test2();
	test3();
	getchar();
}

