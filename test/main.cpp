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

#include <shared_spin_mutex.hpp>

#include <fmt/format.h>

#include <boost/thread/shared_mutex.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <unordered_map>

static std::string human_readable(long x) {
    if (x / 1'000'000'000) {
        return fmt::format("{:.2f}G",
                           x / 1'000'000'000 + (x % 1'000'000'000) / 1'000'000'000.0);
    } else if (x / 1'000'000) {
        return fmt::format("{:.2f}M", x / 1'000'000 + (x % 1'000'000) / 1'000'000.0);
    } else if (x / 1'000) {
        return fmt::format("{:.2f}K",
                           static_cast<double>(x / 1'000) + (x % 1'000) / 1'000.0);
    } else {
        return fmt::format("{}", x);
    }
}

struct NoopMutex {
    void lock() const noexcept {
    }
    void unlock() const noexcept {
    }
    void lock_shared() const noexcept {
    }
    void unlock_shared() const noexcept {
    }
};

template <class Mutex, size_t consumer_size, bool no_producer, bool run_base_line>
void scenario(std::string const& mutex_name) {
    std::cout << fmt::format(
            "Mutex {}. Consumer count {}. No producer {}. Run base line {}.\n",
            mutex_name,
            consumer_size,
            no_producer,
            run_base_line);

    std::unordered_map<int, int> map{{1, 1}};
    Mutex mutex;

    auto const to_run = std::chrono::seconds(5);
    std::atomic<bool> run;
    auto const start_timer = [&] {
        const auto now = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - now < to_run) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        run.store(false, std::memory_order_relaxed);
    };

    if constexpr (run_base_line) {
        run = true;
        std::thread timer{start_timer};
        int i = 0;
        int not_found{0};
        int found{0};
        while (run.load(std::memory_order_relaxed)) {
            auto const it = map.find(i % 2);
            if (it == map.end()) {
                ++not_found;
            } else {
                ++found;
            }
            ++i;
        }
        std::cout << fmt::format(R"(Base line.
Consumer iterations {}.
Consumer successful finds {}.
Consumer failed finds {}.

)",
                                 human_readable(found + not_found),
                                 human_readable(found),
                                 human_readable(not_found));
        timer.join();
    }

    run = true;
    std::thread timer{start_timer};

    // producer
    int pi = 0;
    std::thread producer([&] {
        if constexpr (!no_producer) {
            int i = 2;
            while (run.load(std::memory_order_relaxed)) {
                mutex.lock();
                map.emplace(i, i);
                mutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ++i;
                ++pi;
            }
        }
    });

    struct Consumer {
        int found;
        int not_found;
        std::thread thread;
    };

    std::array<Consumer, consumer_size> per_consumer{};
    for (auto& c : per_consumer) {
        c.thread = std::thread([&] {
            int i = 0;
            while (run.load(std::memory_order_relaxed)) {
                mutex.lock_shared();
                auto const it = map.find(i % 2);
                if (it == map.end()) {
                    ++c.not_found;
                } else {
                    ++c.found;
                }
                mutex.unlock_shared();
                ++i;
            }
        });
    }

    timer.join();
    producer.join();
    for (auto& c : per_consumer) {
        c.thread.join();
    }

    std::cout << fmt::format("Producer iterations {}.\n", pi);

    int j{0};
    for (auto const& c : per_consumer) {
        std::cout << fmt::format(R"(Consumer{0} iterations {1}.
Consumer{0} successful finds {2}.
Consumer{0} failed finds {3}.
)",
                                 ++j,
                                 human_readable(c.found + c.not_found),
                                 human_readable(c.found),
                                 human_readable(c.not_found));
    }
}

int main() {
    scenario<boost::shared_mutex, 1, false, true>("Boost shared");
    std::cout << "\n";
    scenario<boost::shared_mutex, 4, false, false>("Boost shared");
    std::cout << "\n";
    scenario<SharedSpinMutex, 1, false, false>("Shared spin");
    std::cout << "\n";
    scenario<SharedSpinMutex, 4, false, false>("Shared spin");
    std::cout << "\n";
    scenario<SharedSpinMutex, 4, true, false>("Shared spin");
    std::cout << "\n";
    scenario<NoopMutex, 4, true, false>("Noop");
}
