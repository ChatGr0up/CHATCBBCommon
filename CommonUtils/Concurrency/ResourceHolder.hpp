#pragma once
#include <memory>
#include <shared_mutex>
#include <map>
#include <functional>

namespace utils {
template<typename T>
class ObjectHolder {
public:
    using DeleterCallback = std::function<void(const T&)>;

    explicit ObjectHolder(DeleterCallback cb = nullptr)
        : cleanupCb_(std::move(cb)) {}

    template<typename... Args>
    std::shared_ptr<T> getResource(int key, Args&&... args) {
        {
            std::shared_lock lock(mutex_);
            auto it = resources_.find(key);
            if (it != resources_.end()) {
                if (auto sp = it->second.lock()) return sp;
            }
        }
        {
            std::unique_lock lock(mutex_);
            auto it = resources_.find(key);
            if (it != resources_.end()) {
                if (auto sp = it->second.lock()) return sp;
                resources_.erase(it);
            }

            auto deleter = [cb = cleanupCb_](T* p) {
                try { if (cb) cb(*p); } catch(...) {}
                delete p;
            };

            auto sp = std::shared_ptr<T>(new T(std::forward<Args>(args)...), deleter);
            resources_[key] = sp;
            return sp;
        }
    }

private:
    std::map<int, std::weak_ptr<T>> resources_;
    DeleterCallback cleanupCb_;
    std::shared_mutex mutex_;
};
} // namespace utils
