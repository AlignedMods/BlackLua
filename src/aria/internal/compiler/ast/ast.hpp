#pragma once

namespace Aria::Internal {

    template <typename T, typename TT>
    inline T* GetNode(TT* t) {
        T* result = dynamic_cast<T*>(t);
        return result;
    }

} // namespace Aria::Internal
