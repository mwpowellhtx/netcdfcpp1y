
#include "magic.h"

#include <memory>

///////////////////////////////////////////////////////////////////////////////

magic::magic()
    : /*size_reporter()
      , */version(classic) {
    init();
}

magic::magic(magic const & other)
    : /*size_reporter(other)
      , */version(other.version) {
    init();
}

void magic::init() {
    // When to (or to not) use auto... auto took some liberties with this.
    static const char tmp[3] = { 'C', 'D', 'F' };
    memcpy(&key, &tmp, sizeof(key));
}

bool magic::is_classic() const {
    return version == classic;
}

bool magic::is_x64() const {
    return version == x64;
}
