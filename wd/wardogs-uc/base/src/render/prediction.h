#pragma once
#include "../util/math.h"
#include <cmath>

namespace pred {

inline bool EstimateLeadVelocity(Vector3 previous, Vector3 current, double dt, Vector3& velocity) {
    if (!std::isfinite(dt) || dt <= 0.0 || dt > 0.2)
        return false;

    velocity = Vector3(
        (current.x - previous.x) / dt,
        (current.y - previous.y) / dt,
        (current.z - previous.z) / dt);

    const double v2 =
        velocity.x * velocity.x +
        velocity.y * velocity.y +
        velocity.z * velocity.z;

    return std::isfinite(v2) && v2 <= 25000000.0;
}

inline bool PredictHeadAim(
    Vector3 origin, Vector3 head, Vector3 velocity,
    double speed, double gravity,
    Vector3& aim, double& flightSeconds)
{
    auto finite = [](Vector3 v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };

    if (!finite(origin) || !finite(head) || !finite(velocity) ||
        !std::isfinite(speed) || speed < 2000.0 || speed > 200000.0 ||
        !std::isfinite(gravity) || gravity < 0.0 || gravity > 5000.0)
        return false;

    Vector3 r = head - origin;

    auto displacement = [&](double t) {
        return Vector3(
            r.x + velocity.x * t,
            r.y + velocity.y * t,
            r.z + velocity.z * t + 0.5 * gravity * t * t);
    };

    auto error = [&](double t) {
        Vector3 d = displacement(t);
        return d.x * d.x + d.y * d.y + d.z * d.z - speed * speed * t * t;
    };

    if (error(0.0) < 1e-8)
        return false;

    double lo = 0.0, hi = 0.0;
    bool found = false;
    for (int i = 1; i <= 256; ++i) {
        hi = 5.0 * i / 256.0;
        if (error(hi) <= 0.0) {
            found = true;
            break;
        }
        lo = hi;
    }
    if (!found)
        return false;

    for (int i = 0; i < 48; ++i) {
        double mid = (lo + hi) * 0.5;
        if (error(mid) > 0.0)
            lo = mid;
        else
            hi = mid;
    }

    flightSeconds = (lo + hi) * 0.5;
    Vector3 d = displacement(flightSeconds);
    aim = Vector3(origin.x + d.x, origin.y + d.y, origin.z + d.z);
    return finite(aim);
}

inline bool PredictHeadLead(
    Vector3 origin, Vector3 head, Vector3 velocity,
    double speed, Vector3& result)
{
    auto finite = [](Vector3 v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };

    if (!finite(origin) || !finite(head) || !finite(velocity) ||
        !std::isfinite(speed) || speed < 2000.0 || speed > 200000.0)
        return false;

    Vector3 r = head - origin;

    double a =
        velocity.x * velocity.x +
        velocity.y * velocity.y +
        velocity.z * velocity.z - speed * speed;

    double b = 2.0 * (r.x * velocity.x + r.y * velocity.y + r.z * velocity.z);
    double c = r.x * r.x + r.y * r.y + r.z * r.z;
    double t = -1.0;

    if (std::abs(a) < 1e-6) {
        if (std::abs(b) > 1e-6)
            t = -c / b;
    } else {
        double disc = b * b - 4.0 * a * c;
        if (disc < 0.0)
            return false;
        double x = (-b - std::sqrt(disc)) / (2.0 * a);
        double y = (-b + std::sqrt(disc)) / (2.0 * a);
        if (x > 0.0)
            t = x;
        if (y > 0.0 && (t < 0.0 || y < t))
            t = y;
    }

    if (!std::isfinite(t) || t <= 0.0 || t > 3.0)
        return false;

    result = Vector3(
        head.x + velocity.x * t,
        head.y + velocity.y * t,
        head.z + velocity.z * t);
    return finite(result);
}

}
