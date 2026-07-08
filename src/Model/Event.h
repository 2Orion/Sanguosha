#ifndef EVENT_H
#define EVENT_H

#include <functional>
#include <vector>
#include <algorithm>

/// 简单的事件监听器，替代 Qt 信号槽系统
/// 使用观察者模式实现

template<typename... Args>
class EventListener {
public:
    using Callback = std::function<void(Args...)>;
    using CallbackId = size_t;

    /// 连接回调函数，返回连接 ID
    CallbackId connect(Callback callback) {
        CallbackId id = nextId++;
        callbacks.push_back({id, callback});
        return id;
    }

    /// 断开连接
    void disconnect(CallbackId id) {
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [id](const CallbackPair& pair) { return pair.first == id; }),
            callbacks.end()
        );
    }

    /// 发射事件，通知所有监听者
    void emit(Args... args) const {
        for (const auto& pair : callbacks) {
            pair.second(args...);
        }
    }

    /// 清空所有监听者
    void clear() {
        callbacks.clear();
    }

    /// 获取监听者数量
    size_t listenerCount() const {
        return callbacks.size();
    }

private:
    using CallbackPair = std::pair<CallbackId, Callback>;
    std::vector<CallbackPair> callbacks;
    CallbackId nextId = 0;
};

#endif // EVENT_H
