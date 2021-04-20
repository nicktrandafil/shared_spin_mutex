/*
  MIT License

  Copyright (c) 2021 Nicolai Trandafil

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#pragma once

#include <atomic>
#include <chrono>
#include <climits>
#include <cstdint>
#include <thread>

class Sleepyhead {
public:
    Sleepyhead() noexcept
            : spin_count(0) {
    }

    void wait() noexcept {
        if (spin_count < max_spin) {
            ++spin_count;
            pause();
        } else {
            sleep();
        }
    }

private:
    static void sleep() noexcept {
        std::this_thread::sleep_for(std::chrono::nanoseconds(500000)); // 0.5ms
    }

#if defined(__i386__) || defined(__x86_64__) || defined(_M_X64)
    static inline void pause() noexcept {
        asm volatile("pause");
    }
#else
#error "Provide 'pause' for the platform"
#endif

private:
    constexpr static uint32_t max_spin = 4000;
    uint32_t spin_count;
};

class SharedSpinMutex {
public:
    using IntT = uint64_t;
    static constexpr auto bits_count = sizeof(IntT) * CHAR_BIT;
    static constexpr IntT biggest_bit_s_mask = static_cast<IntT>(1) << (bits_count - 1);
    static constexpr IntT half_max_value = biggest_bit_s_mask / 2;

    void lock() noexcept {
        Sleepyhead sleepy;
        while (true) {
            IntT expected{0};
            if (m.compare_exchange_weak(expected,
                                        biggest_bit_s_mask,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed)) {
                break;
            }
            sleepy.wait();
        }
    }

    void unlock() noexcept {
        m.store(0, std::memory_order_release);
    }

    void lock_shared() noexcept {
        Sleepyhead sleepy;
        while (true) {
            auto prev = m.fetch_add(1, std::memory_order_acq_rel);
            if ((prev & (biggest_bit_s_mask)) == 0) {
                break;
            } else if (prev
                       > (half_max_value)) { // producer is too slow, afraid to overflow
                m.compare_exchange_strong(++prev,
                                          biggest_bit_s_mask,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed);
            }
            sleepy.wait();
        }
    }

    void unlock_shared() noexcept {
        m.fetch_sub(1, std::memory_order_acq_rel);
    }

private:
    std::atomic<IntT> m{0};
};
