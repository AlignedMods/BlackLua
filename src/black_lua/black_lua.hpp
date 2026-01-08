#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <iostream>

#ifndef BLUA_ASSERT
    #define BLUA_ASSERT(condition, error) do { if (!(condition)) { std::cerr << "Assertion failed at: " << __LINE__ << ", " << __FILE__ << "\nError: " << error; __debugbreak(); abort(); } } while(0)
#endif

namespace BlackLua {

    // The allocator which gets used internally
    // NOTE: This allocator WON'T call destructors, so NEVER store std::string, std::vector, etc in the compiler!
    // If you don't provide an allocator you must call BlackLua::SetupDefaultAllocator() to set up a default one
    class Allocator {
    public:
        inline Allocator(size_t bytes)
            : m_Capacity(bytes), m_Data(new uint8_t[bytes]) {}

        inline ~Allocator() {
            delete[] m_Data;
            m_Data = nullptr;
            m_Capacity = 0;
            m_Index = 0;
        }

        // Copying/moving an allocator is not valid
        // The only way to pass around an allocator is by pointer
        Allocator(const Allocator& other) = delete;
        Allocator(Allocator&& other) = delete;
        void operator=(const Allocator& other) = delete;
        void operator=(Allocator&& other) = delete;

        [[nodiscard]] inline void* Allocate(size_t bytes) {
            BLUA_ASSERT(m_Index + bytes < m_Capacity, "Too much memory allocated!");
            uint8_t* mem = &m_Data[m_Index];

            m_Index += bytes;

            return reinterpret_cast<void*>(mem);
        }

        template <typename T>
        [[nodiscard]] inline T* AllocateNamed() {
            T* mem = reinterpret_cast<T*>(Allocate(sizeof(T)));

            // return mem;
            return new (mem) T{};
        }

        template <typename T, typename... Args>
        [[nodiscard]] inline T* AllocateNamed(Args&&... args) {
            T* mem = reinterpret_cast<T*>(Allocate(sizeof(T)));

            return new (mem) T{std::forward<Args>(args)...};
        }

        inline void Reset() {
            delete[] m_Data;
            m_Data = new uint8_t[m_Capacity];
            m_Index = 0;
        }

        inline static Allocator* Get() { return s_ActiveAllocator; }
        inline static void Set(Allocator* const alloc) { s_ActiveAllocator = alloc; }

    private:
        uint8_t* m_Data = nullptr;
        size_t m_Capacity = 0;
        size_t m_Index = 0;

        inline static Allocator* s_ActiveAllocator = nullptr;
    };

    inline void SetupDefaultAllocator() {
        static Allocator s_Alloc(10 * 1024 * 1024); // 10 MB, should be plenty for any compilation unit
        Allocator::Set(&s_Alloc);
    }

    inline void SetupAllocator(Allocator* const alloc) {
        Allocator::Set(alloc);
    }

    inline Allocator* GetAllocator() {
        return Allocator::Get();
    }

} // namespace BlackLua