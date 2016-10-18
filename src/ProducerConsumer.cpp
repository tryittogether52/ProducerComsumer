//============================================================================
// Name        : ProducerConsumer.cpp
// Author      : thanhvv8
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream> //! cout
#include <unistd.h> //! sleep
#include <queue> //! queue
#include <pthread.h> //! pthread, pthread_mutex_t, pthread_cond_t
#include <stdlib.h>  /* srand, rand */
#include <time.h>       /* time */

using namespace std;

//! How long a task is processed (seconds).
int consumeTaskTime = 1;
//! How long a task is created (seconds).
int produceTaskTime = 1;

//! Maximum task that Producer can create.

int maxTasksWillBeCreate = 5;

class Task {
public:
	Task(int id) :
			_id(id) {
//		cout << "++++++ Task: " << _id << " is created." << endl;
	}

	virtual void run() {

		/* initialize random seed: */
		/*srand(time(NULL));
		 consumeTaskTime = rand() % 3 + 1; // consumeTaskTime in the range 1 - 3
		 sleep(consumeTaskTime);*/

		sleep(consumeTaskTime);
		cout << "	 >>>>> PROCESSING Task " << _id << ". Done after: "
				<< consumeTaskTime << " seconds." << endl;
	}

	virtual ~Task() {
//		cout << "		---- Task: " << _id << " is destroyed." << endl;
	}

	int id() const {
		return _id;
	}

private:
	int _id;
};

// Wrapper around std::queue with some mutex protection
class Queue {
public:
	Queue(const unsigned int& maxSize) :
			_maxSize(maxSize), _finished(false) {
		// Initialize the mutex protecting the queue
		pthread_mutex_init(&_queueMutex, 0);
		// _notEmptyCond is a condition variable that's signaled
		// when new task arrives
		pthread_cond_init(&_notEmptyCond, 0);

		// _notFullCond is a condition variable that's signaled
		// when a task is remove from the queue
		pthread_cond_init(&_notFullCond, 0);
	}

	~Queue() {
		// Cleanup pthreads
		pthread_mutex_destroy(&_queueMutex);
		pthread_cond_destroy(&_notEmptyCond);
		pthread_cond_destroy(&_notFullCond);
	}

	// Retrieves the next task from the queue
	Task* nextTask() {
		Task* task = NULL;
		// Lock the queue
		pthread_mutex_lock(&_queueMutex);
		if (_finished && _tasks.size() == 0) {
			// If not return null
			task = NULL;
		} else {
			// Not finished, but there are no tasks, so wait for
			// _workCond to be signaled
			while (!_finished && _tasks.size() == 0) {
				cout
						<< "OOOOOOOOOOOOOOOOOOOO--> EMPTY: Queue is empty now. So we must wait until there is new task added."
						<< endl;
				pthread_cond_wait(&_notEmptyCond, &_queueMutex);
			}
			if (_tasks.size()) { //! has new task.
				// get the next task
				task = _tasks.front();
				_tasks.pop();
				cout << "---- REMOVE Task " << task->id() << " from queue."
						<< endl;
				pthread_cond_signal(&_notFullCond);
			} else { //! _finished is set to true
				task = NULL;
			}
		}
		// Unlock the queue and return
		pthread_mutex_unlock(&_queueMutex);
		return task;
	}

	// Add a task
	void addTask(Task* task) {
		// Lock the queue
		pthread_mutex_lock(&_queueMutex);

		while (_tasks.size() == _maxSize) {
			cout << "FFFFFFFFFFFFFFFFFFFF --> FULL: Try to add Task "
					<< task->id()
					<< " to queue but queue is full now. So we must wait until a task is removed."
					<< endl;
			pthread_cond_wait(&_notFullCond, &_queueMutex);
		}
		// Add the task
		_tasks.push(task);
		cout << "++++++ ADD Task " << task->id() << " to queue." << endl;

		// signal there's new task
		pthread_cond_signal(&_notEmptyCond);
		// Unlock the queue
		pthread_mutex_unlock(&_queueMutex);
	}

	void makeFinish() {
		pthread_mutex_lock(&_queueMutex);
		_finished = true;
		// Signal the condition variable in case any threads are waiting
		pthread_cond_broadcast(&_notEmptyCond);
		pthread_mutex_unlock(&_queueMutex);
	}

private:
	std::queue<Task*> _tasks;
	pthread_mutex_t _queueMutex;
	pthread_cond_t _notEmptyCond;
	pthread_cond_t _notFullCond;
	const unsigned int _maxSize;
	bool _finished;

};

// Function that retrieves a task from a queue, runs it and deletes it
void* getWork(void* param) {
	Task *nextTask = NULL;
	Queue *workQueue = (Queue*) param;
	while ((nextTask = workQueue->nextTask())) {
		nextTask->run();
		delete nextTask;
		nextTask = NULL;
	}
	return NULL;
}

void* createWork(void* param) {
	Queue *workQueue = (Queue*) param;
	for (int i = 0; i < maxTasksWillBeCreate; i++) {
		/* initialize random seed: */
		/*srand(time(NULL));
		 produceTaskTime = rand() % 3 + 1; // produceTaskTime in the range 1 - 3
		 sleep(produceTaskTime);*/
		workQueue->addTask(new Task(i));
		sleep(produceTaskTime);
	}

	workQueue->makeFinish();
	return NULL;
}

//! Producer class: a thread that creates new task and pushes to TaskQueue.

class ProducerThread {
public:
	ProducerThread(Queue* workQueue) :
			_taskQueue(workQueue) {
		pthread_create(&_thread, NULL, createWork, _taskQueue);
	}

	void join() {
		pthread_join(_thread, NULL);
	}

private:
	pthread_t _thread;
	Queue* _taskQueue;
};

//! Consumer class: a thread that takes task from TaskQueue and process it.
class ConsumerThread {
public:
	ConsumerThread(Queue* workQueue) :
			_taskQueue(workQueue) {
		pthread_create(&_thread, NULL, getWork, _taskQueue);
	}

	void join() {
		pthread_join(_thread, NULL);
	}
private:
	pthread_t _thread;
	Queue* _taskQueue;
};

int main() {
	cout << "------------ Start program ---------------" << endl;

	//! Maximum tasks that producer can create.
	maxTasksWillBeCreate = 6;

	//! How long a task is created (seconds).
	produceTaskTime = 3;

	//! How long a task is processed (seconds).
	consumeTaskTime = 1;

	//! Maximum tasks in queue.
	const unsigned int MAX_SIZE = 2;
	Queue taskQueue(MAX_SIZE);
	ProducerThread producer(&taskQueue);
	ConsumerThread consumer(&taskQueue);

	//! Main thread must wait for producer and consumer done their jobs.
	producer.join();
	consumer.join();

	cout << "------------ End program ---------------" << endl;
	return 0;
}
