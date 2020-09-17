#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>


using namespace std;

volatile int sum{};
volatile bool flags[2]{};
volatile int victim{};
mutex mtx;

void lock(int my_id)
{
	int other_id = 1 - my_id;
	flags[my_id] = true;
	victim = my_id;

	// prevent out of order execution 
	//_asm mfence; // for x84 inline asm
	atomic_thread_fence(memory_order_seq_cst);

	while( flags[other_id] == true && victim == my_id){}
}

void unlock(int my_id)
{
	flags[my_id] = false;
}


void test_using_peterson_lock(int id)
{
	for (int n{}; n < 2500'0000; ++n)
	{
		lock(id);

		sum += 2;

		unlock(id);
	}
}


void test_using_mutex(int id)
{
	for (int n{}; n < 2500'0000; ++n)
	{
		mtx.lock();

		sum += 2;

		mtx.unlock();
	}
}

struct task_context
{
	task_context(void (*function)(int), const char* tag) 
		: function{ function }, tag{tag}
	{}

	void (*function)(int);
	const char* tag;
};


auto main() -> void
{
	vector<task_context> cases;
	vector<thread> threads;

	cases.emplace_back( test_using_mutex, "mutex" );
	cases.emplace_back( test_using_peterson_lock, "peterson_lock" );

	for (auto& the_case : cases)
	{
		sum = 0;

		auto start_time = chrono::high_resolution_clock::now();

		threads.emplace_back( the_case.function, 1 );
		threads.emplace_back( the_case.function, 0 );

		for (auto& thread : threads)
		{
			thread.join();
		}

		auto exec_time = chrono::high_resolution_clock::now() - start_time;

		cout << the_case.tag << "  sum = " << sum << " Exec_time = " << chrono::duration_cast<chrono::milliseconds>(exec_time).count() << "ms\n";

		threads.clear();
	}
}