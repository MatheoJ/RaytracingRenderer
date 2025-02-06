#pragma once

#include <render/samplers/sampler.h>

class Independent: public Sampler {
public:
    Independent(const json& param = json::object()) : Sampler(param){}
    virtual double next() override {
        return r.nextDouble();
    }
    virtual Vec2 next2d() override {
        return { r.nextDouble(), r.nextDouble() };
    }
    std::shared_ptr<Sampler> clone() override {
        auto s = std::make_shared<Independent>();
        *s = *this; // Copy?
        s->r.seed(this->r.nextUInt(), this->r.nextUInt());
        return s;
    }
};