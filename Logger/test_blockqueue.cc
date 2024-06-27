#include "blockqueue.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;
// BlockQueue's max_size
const int bqSize = 1024;
int bqTestNum = 0;
bool multiThread_correct = true;

template<typename T>
bool singlePushPop(BlockQueue<T>& bq);
template<typename T>
bool multiplePushPop(BlockQueue<T>& bq);
template<typename T>
bool rvaluePushPop(BlockQueue<T>& bq);
template<typename T>
void oneThreadPush(BlockQueue<T>& bq);
template<typename T>
void oneThreadPop(BlockQueue<T>& bq);
template<typename T>
bool multiThread(BlockQueue<T>& bq);

template<>
bool singlePushPop(BlockQueue<string>& bq) {
	string test_name = "singlePushPop";
	cout << "Test " << bqTestNum++ << ": single push and pop";
	string str = "abc";
	bq.push(str);
	string to_check;
	bq.pop(to_check);
	if (str != to_check) {
		cout << '\n' << test_name << " -> failed" << endl;
		return false;
	}
	cout << " -> OK" << endl;
	return true;
}

template<>
bool multiplePushPop(BlockQueue<string>& bq) {
	string test_name = "multiplePushPop";
	cout << "Test " << bqTestNum++ << ": multiple push and pop";
	vector<string> strs(bqSize);
	for (int i = 0; i < bqSize; i++) {
		strs[i] = to_string(i);
		bq.push(strs[i]);
	}
	// bq should be fully filled, call push should be failed
	if (bq.push(to_string(bqSize + 1)) != false) {
		cout << '\n' << test_name << " -> failed" << endl;
		cout << "Fully filled BlockQueue should not be pushed" << endl;
		return false;
	}
	for (int i = 0; i < bqSize; i++) {
		string tmp;
		bq.pop(tmp);
		if (tmp != strs[i]) {
			cout << '\n' << test_name << " -> failed" << endl;
			cout << "Pushed string is not equal to poped string" << endl;
			return false;
		}
	}
	cout << " -> OK" << endl;
	return true;
}

template<>
bool rvaluePushPop(BlockQueue<string>& bq) {
	string test_name = "rvaluePushPop";
	cout << "Test " << bqTestNum++ << ": rvalue push and pop";
	bq.push("abcdefg");
	string to_check;
	bq.pop(to_check);
	if (to_check != "abcdefg") {
		cout << '\n' << test_name << " -> failed" << endl;
		cout << "Rvalue push and pop failed" << endl;
		return false;
	}
	cout << " -> OK" << endl;
	return true;
}

template<>
void oneThreadPush(BlockQueue<string>& bq) {
	for (int i = 0; i < 5; i++) {
		bq.push("This string should be completed");
	}
}
template<>
void oneThreadPop(BlockQueue<string>& bq) {
	for (int i = 0; i < 5; i++) {
		string tmp;
		bq.pop(tmp);
		if (tmp != "This string should be completed") {
			cout << "\nThere is a thread read unexpected string" << endl;
			multiThread_correct = false;
		}
	}
}

template<>
bool multiThread(BlockQueue<string>& bq) {
	string test_name = "multiThread";
	cout << "Test " << bqTestNum++ << ": multiple thread push and pop at the same time";
	vector<thread> w_threads;
	vector<thread> r_threads;
	for (int i = 0; i < 1000; i++) {
		w_threads.push_back(thread(oneThreadPush<string>, std::ref(bq)));
		r_threads.push_back(thread(oneThreadPop<string>, std::ref(bq)));
	}
	for (int i = 0; i < 1000; i++) {
		if (w_threads[i].joinable())
			w_threads[i].join();
		if (r_threads[i].joinable())
			r_threads[i].join();
	}
	if (multiThread_correct == false) {
		cout << '\n' << test_name << " -> failed" << endl;
		cout << "Multiple threads push and pop failed" << endl;
		return false;
	}
	string tmp;
	if (bq.pop(tmp, 1000) != false) {
		cout << '\n' << test_name << " -> failed" << endl;
		cout << "Multiple threads pushed and poped. An empty BlockQueue should have no element" << endl;
		return false;
	}
	cout << " -> OK" << endl;
	return true;
}

int main() {
	cout << "Test: BlockQueue" << endl;
	BlockQueue<string> bq(bqSize);
	singlePushPop(bq);
	multiplePushPop(bq);
	rvaluePushPop(bq);
	multiThread(bq);
	cout << "Test: BlockQueue completed" << endl;
}
