#ifndef INTRUSIVE_LIST_H
#define INTRUSIVE_LIST_H

#include <memory>
#include <type_traits>

namespace details {

struct default_tag {};

template <typename Tag = default_tag>
struct intrusive_list_node {
  intrusive_list_node* prev;
  intrusive_list_node* next;

  intrusive_list_node() : prev(this), next(this) {}

  intrusive_list_node(intrusive_list_node const&) = delete;

  intrusive_list_node(intrusive_list_node&& other) {
    if (!other.empty()) {
      next = other.next;
      prev = other.prev;
      update();
      other.quite_remove();
    } else {
      quite_remove();
    }
  }

  intrusive_list_node& operator=(intrusive_list_node const& other) = delete;
  intrusive_list_node& operator=(intrusive_list_node&& other) noexcept {
    if (&other != this) {
      intrusive_list_node(std::move(other)).swap(*this);
    }
    return *this;
  }

  ~intrusive_list_node() {
    remove();
  }

  bool empty() {
    return next == this && prev == this;
  }

  void swap(intrusive_list_node& other) {
    using std::swap;

    if (empty() && other.empty()) {
      return;
    } else if (empty() || other.empty()) {
      intrusive_list_node& empt = (empty()) ? *this : other;
      intrusive_list_node& not_empt = (!empty()) ? *this : other;
      empt.next = not_empt.next;
      empt.prev = not_empt.prev;
      empt.update();
      not_empt.quite_remove();
    } else {
      swap(prev, other.prev);
      swap(next, other.next);
      update();
      other.update();
    }
  }

  void quite_remove() {
    prev = this;
    next = this;
  }

  void remove() {
    if (!empty()) {
      next->prev = prev;
      prev->next = next;
    }
    quite_remove();
  }

  void update() {
    next->prev = this;
    prev->next = this;
  }
};
} // namespace details

template <typename T, typename Tag = details::default_tag>
class intrusive_list {
  using node_t = details::intrusive_list_node<Tag>;
  node_t end_;

  template <bool Const>
  class iterator_t {
    friend class intrusive_list;

    node_t* ptr;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = typename std::conditional<Const, const value_type*, value_type*>::type;
    using reference = typename std::conditional<Const, const value_type&, value_type&>::type;

    iterator_t() : ptr(nullptr) {}
    iterator_t(node_t* node) : ptr(node) {}
    iterator_t(const iterator_t&) = default;
    iterator_t& operator=(const iterator_t&) = default;

    template <bool Const_, class = std::enable_if_t<Const && !Const_>>
    iterator_t(const iterator_t<Const_>& other) : ptr(other.ptr) {}

    template <bool Const_>
    bool operator==(iterator_t<Const_> const& other) const {
      return ptr == other.ptr;
    }
    template <bool Const_>
    bool operator!=(iterator_t<Const_> const& other) const {
      return ptr != other.ptr;
    }
    typename iterator_t::reference operator*() const {
      return *static_cast<T*>(ptr);
    }
    typename iterator_t::pointer operator->() const {
      return static_cast<T*>(ptr);
    }

    iterator_t operator++() {
      ptr = ptr->next;
      return iterator_t(ptr);
    }
    iterator_t operator++(int) {
      node_t* copy = ptr;
      ++(*this);
      return iterator_t(copy);
    }
    iterator_t operator--() {
      ptr = ptr->prev;
      return iterator_t(ptr);
    }
    iterator_t operator--(int) {
      node_t* copy = ptr;
      --(*this);
      return iterator_t(copy);
    }
  };

public:
  using iterator = iterator_t<false>;
  using const_iterator = iterator_t<true>;

  intrusive_list() = default;

  ~intrusive_list() {
    iterator it = begin();
    while (it != end()) {
      erase(it++);
    }
  }

  iterator begin() noexcept {
    return iterator(end_.next);
  }

  iterator end() noexcept {
    return iterator(&end_);
  }

  const_iterator cbegin() const noexcept {
    return iterator(end_.next);
  }

  const_iterator cend() const noexcept {
    return iterator(end_);
  }

  iterator insert(const_iterator it, T const& value) noexcept {
    node_t* node = const_cast<node_t*>(static_cast<node_t const*>(&value));
    node->remove();
    node->prev = it.ptr->prev;
    node->next = it.ptr;
    node->update();
    return iterator(node);
  }

  iterator push_back(T const& value) noexcept {
    return insert(end(), value);
  }

  iterator erase(iterator it) noexcept {
    iterator del = it++;
    del.ptr->remove();
    return it;
  }

  bool empty() const noexcept {
    return &end_ == end_.next;
  }
};

#endif