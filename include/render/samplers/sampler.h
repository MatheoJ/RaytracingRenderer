#pragma once

#include <render/vec.h>
#include <render/json.h>
#include <pcg32.h>

class Sampler {
protected:
    std::size_t m_samples;
public:
    Sampler(const json& param = json::object()) {
        m_samples = param.value("samples", 1);
    }

    /// Generate 1D sample
    virtual double next() = 0;
    /// Generate 2D sample
    virtual Vec2 next2d() = 0;

    /// return the number of samples requested 
    std::size_t samples() const {
        return m_samples;
    }
    void set_samples(std::size_t s) {
        m_samples = s;
    }
    virtual std::shared_ptr<Sampler> clone() = 0;
    pcg32 r;
};

std::shared_ptr<Sampler> create_sampler(const json& param); 

