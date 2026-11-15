#pragma once

#include <core/context/Type.hpp>
#include <vector>
#include <unordered_map>

namespace blast::core::context {

struct TypeLayout {
    // Type size, in bytes
    std::size_t m_size;
    // Alignement of this type
    std::size_t m_align;
    // Offset of inner fields (for structs) in declaration order
    std::vector<std::size_t> m_offsets;
};


class DataLayout {

public:
    // Get the size of a given type `t`, in bytes
    std::size_t m_sizeof(const Type* t);
    // Get the size of a given type `t`, within struct `s`, in bytes
    std::size_t m_sizeof(const StructType* s, const Type* t);

    // Get the alignment of a given type `t`, in bytes
    std::size_t m_alignof(const Type* t);
    // Get the alignment of a given type `t`, within struct `s`, in bytes
    std::size_t m_alignof(const StructType* s, const Type* t);

    // Get the offset of a given type `t`, within struct `s`, in bytes
    std::size_t m_offsetof(const StructType* s, const Type* t);
    const std::vector<std::size_t>& m_offsets(const StructType* s);


private:
    void registerTypeLayout(const Type* t);

private:
    // Mapping types to their memory layouts
    std::unordered_map<const Type*, TypeLayout> m_layouts;

};


} // namespace blast::core::context
