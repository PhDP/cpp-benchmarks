#ifndef VARIANT_GATES_HH_
#define VARIANT_GATES_HH_

#include <iostream>
#include <variant>
#include <vector>
#include <boost/variant.hpp>

namespace va_gate {

struct toffoli_gate {
  uint32_t c0, c1, x;
  toffoli_gate(uint32_t c0_, uint32_t c1_, uint32_t x_) noexcept
    : c0(c0_), c1(c1_), x(x_) {
  }
  auto apply(std::vector<bool>& bits) const -> void {
    if (bits[c0] && bits[c1]) bits[x] = !bits[x];
  }
  auto print(std::ostream& os) const -> std::ostream& {
    os << "toffoli(" << c0 << ',' << c1 << ',' << x << ')';
    return os;
  };
};

struct fredkin_gate {
  uint32_t c, a, b;
  fredkin_gate(uint32_t c_, uint32_t a_, uint32_t b_) noexcept
    : c(c_), a(a_), b(b_) {
  }
  auto apply(std::vector<bool>& bits) const -> void {
    if (bits[c]) {
      // std::vector<bool>'s operator() returns values so std::swap would fail.
      auto const old_a = bits[a]; // The xor trick would do too...
      bits[a] = bits[b];
      bits[b] = old_a;
    }
  }
  auto print(std::ostream& os) const -> std::ostream& {
    os << "fredkin(" << c << ',' << a << ',' << b << ')';
    return os;
  };
};

struct not_gate {
  uint32_t x;
  not_gate(uint32_t x_) noexcept : x(x_) { }
  auto apply(std::vector<bool>& bits) const -> void {
    bits[x] = !bits[x];
  }
  auto print(std::ostream& os) const -> std::ostream& {
    os << "not(" << x << ')';
    return os;
  };
};

struct cnot_gate {
  uint32_t c, x;
  cnot_gate(uint32_t c_, uint32_t x_) noexcept : c(c_), x(x_) { }
  auto apply(std::vector<bool>& bits) const -> void {
    if (bits[c]) bits[x] = !bits[x];
  }
  auto print(std::ostream& os) const -> std::ostream& {
    os << "cnot(" << c << ',' << x << ')';
    return os;
  };
};

struct swap_gate {
  uint32_t a, b;
  swap_gate(uint32_t a_, uint32_t b_) noexcept : a(a_), b(b_) { }
  auto apply(std::vector<bool>& bits) const -> void {
    auto const old_a = bits[a];
    bits[a] = bits[b];
    bits[b] = old_a;
  }
  auto print(std::ostream& os) const -> std::ostream& {
    os << "swap(" << a << ',' << b << ')';
    return os;
  };
};

using gate = std::variant<
  not_gate,
  cnot_gate,
  swap_gate,
  toffoli_gate,
  fredkin_gate
>;

// Can store directly in a vector because it's not fucking pointers to a base class:
using gates = std::vector<gate>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

inline auto apply_gates(std::vector<bool> const& bits, gates const& gs) -> std::vector<bool> {
  auto new_bits = bits;
  for (auto const& g : gs) {
    std::visit([&new_bits](auto const& x) { x.apply(new_bits); }, g);
  }
  return new_bits;
}

struct visitor {
  std::vector<bool>& m_bits;

  visitor(std::vector<bool>& bits) : m_bits(bits) {}

  template<typename Gate>
  auto operator()(Gate const& g) const -> void {
    g.apply(m_bits);
  }
};

struct explicit_visitor {
  std::vector<bool>& m_bits;

  explicit_visitor(std::vector<bool>& bits) : m_bits(bits) {}

  auto operator()(toffoli_gate const& g) const -> void {
    if (m_bits[g.c0] && m_bits[g.c1]) m_bits[g.x] = !m_bits[g.x];
  }

  auto operator()(fredkin_gate const& g) const -> void {
    if (m_bits[g.c]) {
      // std::vector<bool>'s operator() returns values so std::swap would fail.
      auto const old_a = m_bits[g.a]; // The xor trick would do too...
      m_bits[g.a] = m_bits[g.b];
      m_bits[g.b] = old_a;
    }
  }
  auto operator()(not_gate const& g) const -> void {
    m_bits[g.x] = !m_bits[g.x];
  }

  auto operator()(cnot_gate const& g) const -> void {
    if (m_bits[g.c]) m_bits[g.x] = !m_bits[g.x];
  }

  auto operator()(swap_gate const& g) const -> void {
    auto const old_a = m_bits[g.a];
    m_bits[g.a] = m_bits[g.b];
    m_bits[g.b] = old_a;
  }
};

inline auto apply_gates_stdvisitor(std::vector<bool> const& bits, gates const& gs) -> std::vector<bool> {
  auto new_bits = bits;
  for (auto const& g : gs) {
    std::visit(visitor{new_bits}, g);
  }
  return new_bits;
}

inline auto apply_gates_explicit_stdvisitor(std::vector<bool> const& bits, gates const& gs) -> std::vector<bool> {
  auto new_bits = bits;
  for (auto const& g : gs) {
    std::visit(explicit_visitor{new_bits}, g);
  }
  return new_bits;
}

inline auto operator<<(std::ostream& os, gate const& g) -> std::ostream& {
  return std::visit([&os](auto const& v) -> std::ostream& { return v.print(os); }, g);
}

using boost_gate = boost::variant<
  not_gate,
  cnot_gate,
  swap_gate,
  toffoli_gate,
  fredkin_gate
>;

using boost_gates = std::vector<boost_gate>;

struct gate_vstr : public boost::static_visitor<void> {
  std::vector<bool>& m_bits;

  gate_vstr(std::vector<bool>& bits) : m_bits(bits) {}

  template<typename Gate>
  auto operator()(Gate const& g) const -> void {
    g.apply(m_bits);
  }
};

inline auto apply_gates(std::vector<bool> const& bits, boost_gates const& gs) -> std::vector<bool> {
  auto new_bits = bits;
  auto const vstr = gate_vstr{new_bits};
  for (auto const& g : gs) {
    boost::apply_visitor(vstr, g);
  }
  return new_bits;
}

inline auto apply_which_gates(std::vector<bool> const& bits, boost_gates const& gs) -> std::vector<bool> {
  auto new_bits = bits;
  auto const vstr = gate_vstr{new_bits};
  for (auto const& g : gs) {
    auto const id = g.which();
    if (id == 0) {
      not_gate const& g_ = boost::get<not_gate>(g);
      new_bits[g_.x] = !new_bits[g_.x];
    } else if (id == 1) {
      cnot_gate const& g_ = boost::get<cnot_gate>(g);
      if (new_bits[g_.c]) new_bits[g_.x] = !new_bits[g_.x];
    } else if (id == 2) {
      swap_gate const& g_ = boost::get<swap_gate>(g);
      auto const old_a = new_bits[g_.a];
      new_bits[g_.a] = new_bits[g_.b];
      new_bits[g_.b] = old_a;
    } else if (id == 3) {
      toffoli_gate const& g_ = boost::get<toffoli_gate>(g);
      if (new_bits[g_.c0] && new_bits[g_.c1]) new_bits[g_.x] = !new_bits[g_.x];
    } else if (id == 4) {
      fredkin_gate const& g_ = boost::get<fredkin_gate>(g);
      if (new_bits[g_.c]) {
        // std::vector<bool>'s operator() returns values so std::swap would fail.
        auto const old_a = new_bits[g_.a]; // The xor trick would do too...
        new_bits[g_.a] = new_bits[g_.b];
        new_bits[g_.b] = old_a;
      }
    }
  }
  return new_bits;
}

} /* end namespace */

#endif
