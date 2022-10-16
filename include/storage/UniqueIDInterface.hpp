#pragma once

#include <type_traits>

enum class UniqueIDName{
    Instance,
    Type,
};
template<UniqueIDName I>
class UniqueIDInterface{
public:
    static constexpr uint32_t DEFAULT = 0;
    UniqueIDInterface(){}
    UniqueIDInterface(uint32_t id): id(id) {}
    uint32_t getUniqueID() const { return id; }
    bool operator==(const UniqueIDInterface<I> &other) const{
        return id == other.id;
    }
protected:
    uint32_t id{DEFAULT};
};

using UniqueIDInstance = UniqueIDInterface<UniqueIDName::Instance>;
using UniqueIDType = UniqueIDInterface<UniqueIDName::Type>;

template <typename T, UniqueIDName U>
concept CUniqueID = std::is_base_of_v<UniqueIDInterface<U>, T>;