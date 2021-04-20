# shared_spin_lock

* It is faster than `boost::shared_mutex`.
* `lock_shared` is very greedy, so wait duration of `lock` is unfair. It
should not be a problem because this mutex is intended for
_passive-writer-active-readers_ scenario.
* In best case `lock_shared` is just one `fetch_add`.
* In best case `lock` is just one `compare_exchange_weak`.
