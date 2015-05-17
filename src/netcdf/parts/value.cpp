#include "value.h"

///////////////////////////////////////////////////////////////////////////////

value::value() {
    init();
}

value::value(uint8_t x) {
    init();
    primitive.b = x;
}

value::value(int16_t x) {
    init();
    primitive.s = x;
}

value::value(int32_t x) {
    init();
    primitive.i = x;
}

value::value(float_t x) {
    init();
    primitive.f = x;
}

value::value(double_t x) {
    init();
    primitive.d = x;
}

value::value(std::string const & text) {
    init(text);
}

value::value(value const & other)
    : primitive(other.primitive)
    , text(other.text) {
}

value::~value() {
}

void value::init(std::string const & text) {
    this->text = text;
    memset(&primitive, 0, sizeof(primitive));
}
