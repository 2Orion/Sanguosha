#ifndef CORE_RANDOMUTILS_H
#define CORE_RANDOMUTILS_H

#include <random>

/// 随机数工具，替代 QRandomGenerator

namespace RandomUtils {

/// 返回 [0, bound) 范围内的随机整数
inline int bounded(int bound) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, bound - 1);
    return dist(gen);
}

} // namespace RandomUtils

#endif // CORE_RANDOMUTILS_H
