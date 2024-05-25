#include "dutils.h"
#include "architecture.h"

struct myStruct {
    public:
        int aux = 1;
        void fun1() {}

    // Virtual destructor to support proper polymorphic deletion
    virtual ~myStruct() {}

    // Explicitly default the copy constructor
    myStruct(const myStruct&) = default;

    // Explicitly default the move constructor
    myStruct(myStruct&&) noexcept = default;
};

ENFORCE_ARCHITECTURE_DESIGN(myStruct);

int main() {
    return 0;
}