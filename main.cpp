#include <iostream>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>

std::mutex coutMutex;

class TaskQueue {
private:
    std::queue<int> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopped;

public:
    TaskQueue() : stopped(false) {
    }

    void push(int task) {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(task);
        cv.notify_one();
    }
    bool pop(int& task) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() {
            return !tasks.empty() || stopped;
        });
        if (tasks.empty() && stopped) {
            return false;
        }
        task = tasks.front();
        tasks.pop();
        return true;
    }
    void stop() {
        std::lock_guard<std::mutex> lock(mutex);
        stopped = true;
        cv.notify_all();
    }
};

void worker(TaskQueue& queue, int workerId) {
    int task;
    while (queue.pop(task)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Worker-" << workerId << " processed task " << task << std::endl;
    }
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "Worker-" << workerId << " finished work" << std::endl;
}

int main() {
    TaskQueue queue;
    std::vector<std::thread> workers;
    const int workerCount = 4;
    for (int i = 1; i <= workerCount; i++) {
        workers.push_back(std::thread(worker, std::ref(queue), i));
    }
    for (int i = 1; i <= 20; i++) {
        queue.push(i);
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Main thread added task " << i << std::endl;
    }
    queue.stop();
    for (size_t i = 0; i < workers.size(); i++) {
        workers[i].join();
    }
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "All tasks are processed, program finished" << std::endl;
    return 0;
}
