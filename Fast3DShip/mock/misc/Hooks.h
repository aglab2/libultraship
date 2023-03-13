#pragma once

namespace Ship {
struct ExitGame {};
struct GfxInit {};

template <typename H, typename... Args> static inline void ExecuteHooks(Args&&... args) {
}
} // namespace Ship
