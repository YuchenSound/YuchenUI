#pragma once

#include <cstddef>
#include <string_view>

namespace YuchenUI {
namespace Resources {

struct ResourceData {
    const unsigned char* data;
    size_t size;
    std::string_view path;
    float designScale;
};

} // namespace Resources
} // namespace YuchenUI
