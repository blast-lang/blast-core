#include <core/context/DataLayout.hpp>
#include <core/context/Exception.hpp>
#include <algorithm>
#include <numeric>

namespace blast::core::context {

namespace {

std::size_t alignUp(std::size_t offset, std::size_t align) {
    return (offset + align - 1) / align * align;
}

} // namespace

std::size_t DataLayout::m_sizeof(const Type* t) {
    this->registerTypeLayout(t);
    return this->m_layouts.at(t).m_size;
}

std::size_t DataLayout::m_alignof(const Type* t) {
    this->registerTypeLayout(t);
    return this->m_layouts.at(t).m_align;
}

const std::vector<std::size_t>& DataLayout::m_offsets(const StructType* s) {
    this->registerTypeLayout(s);
    return this->m_layouts.at(s).m_offsets;
}

void DataLayout::registerTypeLayout(const Type* t) {
    if (this->m_layouts.find(t) != this->m_layouts.end()) {
        return;
    }

    // Default alignment is always 1, even on empty structs and 'Nothing'
    TypeLayout layout{0, 1, {}};
    switch (t->kind()) {
        case Type::Kind::Primitive: {
            const auto* p = static_cast<const PrimitiveType*>(t);
            layout.m_size = p->bitWidth() / 8;
            layout.m_align = layout.m_size;
            break;
        }
        case Type::Kind::Struct: {
            const auto& fields = static_cast<const StructType*>(t)->fields();
            // First, register the elements of the struct
            for (const auto& field: fields) {
                this->registerTypeLayout(field.type);
            }

            // Fields are placed by growing alignment, ties keep declaration order
            std::vector<std::size_t> order(fields.size());
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
                return this->m_layouts.at(fields[a].type).m_align < this->m_layouts.at(fields[b].type).m_align;
            });

            // Offsets stay indexed by declaration order
            layout.m_offsets.resize(fields.size());
            std::size_t offset = 0;
            for (std::size_t i: order) {
                const TypeLayout& field_layout = this->m_layouts.at(fields[i].type);
                offset = alignUp(offset, field_layout.m_align);
                layout.m_offsets[i] = offset;
                offset += field_layout.m_size;
                // A struct alignment is the equal to the biggest aligment between its fields
                layout.m_align = std::max(layout.m_align, field_layout.m_align);
            }
            layout.m_size = alignUp(offset, layout.m_align);
            break;
        }
        default:
            throw UnsupportedLayout(t);
    }

    this->m_layouts.emplace(t, std::move(layout));
}

} // namespace blast::core::context
