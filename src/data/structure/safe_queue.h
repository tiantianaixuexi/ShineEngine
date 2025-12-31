
#ifndef SHINE_IMPORT_STD
#include <mutex>
#include <queue>
#endif

export module shine.safe_queue;

#ifdef SHINE_IMPORT_STD
import std;
#endif




namespace shine::data
{
    template<typename T>
    class safe_queue
    {
    public:
        safe_queue() = default;

        void push(const T& event) {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(event);
            cv.notify_one();
        }

        T pop() {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return !queue.empty(); });
            T item = std::move(queue.front());
            queue.pop();
            return item;
        }

        bool try_pop(T& item) {
            std::lock_guard<std::mutex> lock(mtx);
            if (queue.empty()) return false;
            item = std::move(queue.front());
            queue.pop();
            return true;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mtx);
            return queue.empty();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mtx);
            return queue.size();
        }

    private:
        std::queue<T> queue;
        mutable std::mutex mtx;
        std::condition_variable cv;
    };
}
