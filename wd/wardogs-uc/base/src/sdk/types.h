#pragma once
#include <cstdint>
#include <cstddef>

namespace sdk {

struct FVector {
    double X;
    double Y;
    double Z;
};

struct FRotator {
    double Pitch;
    double Yaw;
    double Roll;
};

}
