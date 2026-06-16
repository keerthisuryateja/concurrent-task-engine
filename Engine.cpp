#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>
#include <string>
#include <hiredis/hiredis.h>

enum class Priority { LOW, MEDIUM, HIGH };

struct Task {
    Priority priority;
    std::function<void()> func;
    
    // priority queue max-heap setup
    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

class TaskEngine {
private:
    std::vector<std::thread> workers;
    std::priority_queue<Task> tasks;
    
    std::mutex queue_mtx;
    std::mutex redis_mtx; 
    std::mutex cout_mtx;  
    std::condition_variable cv;
    bool stop;
    
    redisContext* redis_ctx;

public:
    TaskEngine(size_t num_threads) : stop(false), redis_ctx(nullptr) {
        redis_ctx = redisConnect("127.0.0.1", 6379);
        if (!redis_ctx || redis_ctx->err) {
            std::cerr << "Redis connection failed: " 
                      << (redis_ctx ? redis_ctx->errstr : "allocation error") << std::endl;
            if (redis_ctx) {
                redisFree(redis_ctx);
                redis_ctx = nullptr;
            }
        } else {
            std::cout << "Connected to Redis." << std::endl;
            // Reset tracking counter
            auto* reply = (redisReply*)redisCommand(redis_ctx, "SET total_tasks_completed 0");
            if (reply) freeReplyObject(reply);
        }

        // Spin up workers
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mtx);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        
                        if (stop && tasks.empty()) return; 
                        
                        task = std::move(tasks.top());
                        tasks.pop();
                    } 
                    
                    task.func();
                    
                    if (redis_ctx) {
                        std::lock_guard<std::mutex> r_lock(redis_mtx);
                        auto* reply = (redisReply*)redisCommand(redis_ctx, "INCR total_tasks_completed");
                        if (reply) freeReplyObject(reply);
                    }
                }
            });
        }
    }

    ~TaskEngine() {
        {
            std::lock_guard<std::mutex> lock(queue_mtx);
            stop = true;
        }
        cv.notify_all();
        
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
        
        if (redis_ctx) redisFree(redis_ctx);
        std::cout << "Engine shutdown complete." << std::endl;
    }

    void enqueue(Priority priority, std::function<void()> func) {
        {
            std::lock_guard<std::mutex> lock(queue_mtx);
            tasks.push({priority, std::move(func)});
        }
        cv.notify_one();
    }

    void safe_print(const std::string& msg) {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << msg << std::endl;
    }
};

int main() {
    std::cout << "Starting Task Engine...\n";
    TaskEngine engine(4); 

    for (int i = 1; i <= 20; ++i) {
        engine.enqueue(Priority::LOW, [&engine, i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            engine.safe_print("Executed LOW priority task " + std::to_string(i));
        });
    }
    
    for (int i = 1; i <= 10; ++i) {
        engine.enqueue(Priority::HIGH, [&engine, i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
            engine.safe_print("Executed HIGH priority task " + std::to_string(i));
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0; 
}
