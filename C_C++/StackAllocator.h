//
// Created by Rubén Osorio López on 1/12/16.
//

#ifndef C_C_STACKALLOCATOR_H
#define C_C_STACKALLOCATOR_H


#include <cstddef>
#include <cassert>
#include <new>

template<std::size_t N, std::size_t alignment = alignof(std::max_align_t)>
class StackAllocator {

    alignas(alignment) char buffer[N];          //Using char because the size is equal to a byte.
    char *pointer;                              //Actual pointer to the position we are in in the buffer

public:
    ~StackAllocator() {
        pointer = nullptr;
    }

    StackAllocator() noexcept : pointer(buffer) {}

    StackAllocator(const StackAllocator &) = delete;

    StackAllocator &operator=(const StackAllocator &) = delete;

    template<std::size_t ReqAlign>
    char *allocate(std::size_t n);

    void deallocate(char *p, std::size_t n) noexcept;

    char *getCurrentMarker() noexcept;

    static constexpr std::size_t size() noexcept { return N; }

    std::size_t used() const noexcept { return static_cast<std::size_t>(pointer - buffer); }

    void reset() noexcept { pointer = buffer; }

    void resetToMarker(char *p);


private:
    static std::size_t align_up(std::size_t n) noexcept {
        /*
         *  This calculates the alignment using a bitwise AND and a bitwise OR:
         *
         *      First you sum to the position the alignment minus one to offset it further.
         *
         *      Then you do an AND with the alignment negated because the positions with a bit
         *      1 in (alignment - 1) have to be 0 in an aligned position (aligned - 1 has an array of
         *      zeroes followed by ones like in 0..01..1)
         *
         * */
        return (n + (alignment - 1)) & ~(alignment - 1);
    }

    bool isAligned(char *p) {
        return (((uintptr_t) (const void *) p & (alignment - 1)) == 0);
    }

    bool pointer_in_buffer(char *p) noexcept {
        /*
         *  This just checks if a pointer is after the beginning
         *  of the block and before the end. Our block is always
         *  contiguous so we should have no problem.
         *
         * */
        return buffer <= p && p <= buffer + N;
    }
};

template<std::size_t N, std::size_t alignment>
template<std::size_t ReqAlign>
char *StackAllocator<N, alignment>::allocate(std::size_t n) {
    static_assert(ReqAlign <= alignment, "Alignment is too small for this arena");
    auto const aligned_n = align_up(n);
    if (static_cast<decltype(aligned_n)>(buffer - pointer + N) >= aligned_n) {
        char *r = pointer;
        pointer += aligned_n;
        return r;       //Success allocating the amount of memory required
    }
    //We could not alloc that much memory
    return nullptr;
}

template<std::size_t N, std::size_t alignment>
void StackAllocator<N, alignment>::deallocate(char *p, std::size_t n) noexcept {
    if (pointer_in_buffer(p)) {
        n = align_up(n);
        /*
         * If the pointer given to us plus the aligned position
         * is equal to the current pointer
         * */
        if (p + n == pointer) {
            /*
             * We can put the pointer in the position given to us
             * since we are deleting only the last element.
             * */
            pointer = p;
        }
    }

    //If we get here the pointer was not on our pool of memory
}


/**
 * We get the current position of the internal pointer.
 *
 * Useful if we want to make some allocs and the come back to the state
 * that we were before allocating any of those.
 *
 * */
template<std::size_t N, std::size_t alignment>
char *StackAllocator<N, alignment>::getCurrentMarker() noexcept {
    return pointer;
}

/**
 * We set the internal pointer to point to an old correct position
 * and this way we reset the data inserted after that position.
 *
 * */
template<std::size_t N, std::size_t alignment>
void StackAllocator<N, alignment>::resetToMarker(char *p) {
    if (pointer_in_buffer(p) && isAligned(p)) {
        pointer = p;
    }

    std::cerr << "The pointer provided is not aligned: " << static_cast<void *>(p) << std::endl;
}

#endif //C_C_STACKALLOCATOR_H
