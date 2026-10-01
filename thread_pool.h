#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <vector> 
class ThreadPool {
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    explicit ThreadPool(size_t numThreads){
        for(size_t i = 0;i<numThreads;++i){
            workers.emplace_back([this](){
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock,[this](){return !tasks.empty() || stop;});
                        if(stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "task exception: " << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "task unknown exception\n";
                    }
                }
            });
        }
    }
    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker:workers){
            if (worker.joinable()) worker.join();
        }
    }

    void enqueue(std::function<void()> task){
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }
    
private:
    std::vector<std::thread>workers;
    std::queue<std::function<void()>>tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;

};