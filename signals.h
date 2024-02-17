#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <utility>

#include "intrusive_list.h"

namespace signals {
template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  struct connection;
  struct signal_tag {};

  using connection_list = intrusive_list<connection, signal_tag>;
  using slot_t = std::function<void(Args...)>;
  using base_node = details::intrusive_list_node<signal_tag>;

  struct connection : public base_node {
    friend class signal;

    connection() = default;

    explicit connection(signal* sig, slot_t slot) : sig(sig), slot(std::move(slot)) {
      sig->list.push_back(*this);
    }

    connection(connection const&) = delete;
    connection(connection&& other) : slot(std::move(other.slot)), sig(other.sig) {
      this->update();
      if (sig) {
        sig->list.insert(++(typename connection_list::iterator(&other)), *this);
      }
      other.disconnect();
    }

    void swap(connection& other) {
      using std::swap;

      static_cast<base_node&>(*this).swap(static_cast<base_node&>(other));
      swap(slot, other.slot);
      swap(sig, other.sig);
    }

    connection& operator=(connection const&) = delete;
    connection& operator=(connection&& other) {
      if (&other != this) {
        connection(std::move(other)).swap(*this);
      }
      return *this;
    }

    void disconnect() {
      if (!sig) {
        return;
      }
      for (iterator_holder* cur = sig->tail; cur != nullptr; cur = cur->prev) {
        if (this == &(*(cur->current))) {
          ++(cur->current);
        }
      }
      this->remove();
      sig = nullptr;
    }

    ~connection() {
      disconnect();
    }

  private:
    slot_t slot;
    signal* sig{nullptr};
  }; // connection

  connection connect(slot_t slt) {
    return connection(this, slt);
  }

  template <typename... Args_>
  void operator()(Args_... args) {
    for (iterator_holder it(this); it.current != list.end();) {
      (it.current++)->slot(args...);
      if (!it.sig) {
        return;
      }
    }
  }

  void swap(signal& other) {
    using std::swap;
    list.swap(other.list);
    swap(tail, other.tail);
  }

  signal() = default;
  signal(signal const&) = delete;
  signal(signal&& other) : list(std::move(other.list)), tail(other.tail) {
    other.tail = nullptr;
  }

  signal& operator=(signal const&) = delete;
  signal& operator=(signal&& other) {
    if (&other != *this) {
      signal(std::move(other)).swap(*this);
    }
    return *this;
  }

  ~signal() {
    iterator_holder* it;
    while (tail != nullptr) {
      it = tail->prev;
      tail->sig = nullptr;
      tail = it;
    }
    for (typename connection_list::iterator it = list.begin(); it != list.end(); ++it) {
      it->sig = nullptr;
    }
  }

private:
  struct iterator_holder {

    explicit iterator_holder(signal* sig) : sig(sig), current(sig->list.begin()), prev(sig->tail) {
      sig->tail = this;
    }

    ~iterator_holder() {
      if (sig) {
        sig->tail = prev;
        sig = nullptr;
      }
    }

    iterator_holder* prev;
    typename connection_list::const_iterator current;
    signal const* sig;
  };

  connection_list list;
  mutable iterator_holder* tail{nullptr};
};
} // namespace signals

#endif