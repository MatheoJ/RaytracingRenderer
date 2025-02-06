#pragma once
#include <render/shapes/shape.h>
#include <render/texture.h>
#include <render/image.h>
#include <iostream>

class EnvironmentMap : public Shape {
private:
    Real m_radius;
    Texture<Color4> m_envmap;
    Array2d<Real> m_pdf_envmap;
    Array2d<Real> m_pdf_envmap_normalized;
    Real m_mean_envmap;
    std::vector<Real> m_marginal;
    std::vector<Real> m_cdf_marginal;
    Array2d<Real> m_condtional;
    Array2d<Real> m_cdf_condtional;
    bool USE_IMPORTANCE = true;
    // std::shared_ptr<Material> m_material;

    // TODO : compatibilité avec uv_offset

    Vec2 dir2uv(const Vec3& w) const {
        const Vec3 w_normalized = normalize(w);

        const double u = 1 + INV_PI * atan2(w_normalized.y, -w_normalized.x);
        const double v = INV_PI * acos(-w_normalized.z);

        return {modulo(u / 2.0, 1.0), modulo(v, 1.0)};
    }

    // TODO : utiliser binary search

    std::tuple<int, Real> sample_theta(const Real& x) const{
        int j = 0;
        const Real x_scale = x * m_cdf_marginal[m_pdf_envmap.sizeY() - 1];
        // spdlog::info("theta x = {}, cdf = {}", x_scale,  m_cdf_marginal[j]);
        while (x_scale > m_cdf_marginal[j] && j < m_pdf_envmap.sizeY()){
            j++;
        }
        return {j - 1, (Real)j * M_PI / (Real)m_pdf_envmap.sizeY()};
    }

    std::tuple<int, Real> sample_phi(const Real& x, const int& j) const{
        int i = 0;
        // spdlog::info("phi x = {}, cdf = {}", x,  m_cdf_condtional.at(i, j));
        while (x > m_cdf_condtional.at(i, j) && i < m_pdf_envmap.sizeX()){
            i++;
        }
        return {i, (Real)i * 2 * M_PI / (Real)m_pdf_envmap.sizeX()};
    } 

    void save_image(Array2d<Real> arr, std::string path){
        Image im = Image(arr.sizeX(), arr.sizeY());
        for (int i = 0; i < arr.sizeX(); i++){
            for (int j = 0; j < arr.sizeY(); j++){
                im.at(i, j) = {arr.at(i, j), arr.at(i, j), arr.at(i, j), 1.0};
            }
        }
        im.save(path);
    }

public:
    EnvironmentMap(){}

    EnvironmentMap(const json& param, Real radius){
        m_radius = radius;
        m_envmap = create_texture_color(param, "background", {0.0, 0.0, 0.0, 1.0});
        if (is_image(m_envmap)){
            m_marginal.reserve(m_pdf_envmap.sizeY());
            m_cdf_marginal.reserve(m_pdf_envmap.sizeY());
            m_pdf_envmap = luminance(get_image(m_envmap));
            m_pdf_envmap_normalized = Array2d<Real>(m_pdf_envmap.sizeX(), m_pdf_envmap.sizeY(), 0.0);
            // const Real mean_env = sum(m_pdf_envmap) / (m_pdf_envmap.sizeX() * m_pdf_envmap.sizeY());
            // for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
            //     for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
            //         m_pdf_envmap.at(i, j) = std::max(m_pdf_envmap.at(i, j) - mean_env, 0.0);
            //     }
            // }
            // save_image(m_pdf_envmap, "output/compet_justin/pdf_envmap_unnormalized.exr");
            // apply jacobian and normalize
            for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                    Real theta = ((double)j / (double)m_pdf_envmap.sizeY()) * M_PI;
                    m_pdf_envmap.at(i, j) = m_pdf_envmap.at(i, j) * sin(theta) + 1e-9;
                }
            }
            // save_image(m_pdf_envmap, "output/compet_justin/pdf_envmap_jacob.exr");
            Real sum_envmap = sum(m_pdf_envmap);
            for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                    m_pdf_envmap_normalized.at(i, j) = (m_pdf_envmap.at(i, j) / sum_envmap) ;//+ 1e-8;
                }
            }
            // save_image(m_pdf_envmap_normalized, "output/compet_justin/pdf_envmap_normalized.exr");

            m_mean_envmap = sum(m_pdf_envmap) / (m_pdf_envmap.sizeX() * m_pdf_envmap.sizeY());
            // pdf marginal
            for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                Real sum_marg = 0.0;
                for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                    sum_marg += m_pdf_envmap.at(i, j);
                }
                m_marginal.push_back(sum_marg);
            }
            for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                    m_pdf_envmap_normalized.at(i, j) = (m_pdf_envmap.at(i, j) / m_marginal[j]) ;//+ 1e-8;
                }
            }
            // save_image(m_pdf_envmap_normalized, "output/compet_justin/pdf_envmap_normalized.exr");
            // cdf marginal
            Real sum_marg_cdf = 0.0;
            for (int j = 0; j < m_marginal.size(); j++){
                sum_marg_cdf += m_marginal[j];
                m_cdf_marginal.push_back(sum_marg_cdf);
            }
            // pdf conditionnal
            m_condtional = Array2d<Real>(m_pdf_envmap.sizeX(), m_pdf_envmap.sizeY(), 0.0);
            for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                    m_condtional.at(i, j) = m_pdf_envmap.at(i, j) / m_marginal.at(j);
                }
            }
            // save_image(m_condtional, "output/compet_justin/pdf_conditional.exr");
            // cdf conditionnal
            m_cdf_condtional = Array2d<Real>(m_pdf_envmap.sizeX(), m_pdf_envmap.sizeY(), 0.0);
            for (int j = 0; j < m_pdf_envmap.sizeY(); j++){
                Real sum_cond = 0;
                for (int i = 0; i < m_pdf_envmap.sizeX(); i++){
                    sum_cond += m_condtional.at(i, j);
                    m_cdf_condtional.at(i, j) = sum_cond;
                }
            }
            // save_image(m_cdf_condtional, "output/compet_justin/cdf_conditional.exr");
            // computeHistogram(4000000, "output/compet_justin/histo.exr", param);
        }
    }

    void computeHistogram(int nb_samble, std::string path, const json& param){
        Array2d<Real> result(m_pdf_envmap.sizeX(), m_pdf_envmap.sizeY(), 0);
        std::shared_ptr<Sampler> sampler = create_sampler(param);
        const Real unit = 1.0 / nb_samble;
        for (int i = 0; i < nb_samble; i++){
            Vec2 sample = sampler->next2d();
            std::tuple<int, Real> sample_th = sample_theta(sample.x);
            Real theta = std::get<1>(sample_th);
            std::tuple<int, Real> sample_ph = sample_phi(sample.y, std::get<0>(sample_th));
            Real phi = std::get<1>(sample_ph);
            const Vec3 sample_env = {sin(theta) * cos(phi), -sin(theta) * sin(phi), -cos(theta)};
            result.at(std::get<0>(sample_ph), std::get<0>(sample_th)) += unit;
            // Vec2 uv = dir2uv(sample_env);
            // result.at((int)(uv.x * m_pdf_envmap.sizeX()), (int)(uv.y * m_pdf_envmap.sizeY())) += unit;

        }
        save_image(result, path);
    }

    bool hit(const Ray& _r, Intersection& its) const override {
        return false;
    }

    virtual AABB aabb() const override {
        const Real diag = M_SQRT2 * m_radius;
        return AABB(
            Vec3(-diag, -diag, -diag) - Vec3(1e-4),
            Vec3(diag, diag, diag) + Vec3(1e-4)
        );
    }

    Color3 eval_direct(const Vec3& wo) const {
        return eval(m_envmap, dir2uv(wo), {0.0, 0.0, 0.0}).xyz();
    }

    virtual std::tuple<EmitterSample, const Shape& > sample_direct(const Vec3& x, const Vec2& sample) const override{
        EmitterSample em;
        if (!USE_IMPORTANCE || !is_image(m_envmap)){
            const Vec3 sample_env = sample_spherical(sample); 
            em.y = x + 2 * m_radius * sample_env;
            em.n = -sample_env;
            em.uv = dir2uv(em.y);
            em.pdf = pdf_spherical(sample_env);
            return {em, *this};
        }
        std::tuple<int, Real> sample_th = sample_theta(sample.x);
        Real theta = std::get<1>(sample_th);
        std::tuple<int, Real> sample_ph = sample_phi(sample.y, std::get<0>(sample_th));
        Real phi = std::get<1>(sample_ph);
        const Vec3 d = {sin(theta) * cos(phi), -sin(theta) * sin(phi), -cos(theta)};
        Real inv_sin_theta = std::abs(1.0 / sin(theta));
        em.uv = {(Real)std::get<0>(sample_ph) / m_pdf_envmap.sizeX(), (Real)std::get<0>(sample_th) / m_pdf_envmap.sizeY()};
        em.y = x + d * 2 * m_radius;
        em.n = -d;

        em.pdf = m_pdf_envmap.at(std::get<0>(sample_ph), std::get<0>(sample_th)) * inv_sin_theta * M_1_PI / 4;
        return {em, *this};
    }
    
    virtual Real pdf(const Vec3& x, const Vec3& d) const {
        if (!USE_IMPORTANCE || !is_image(m_envmap)){
            const Vec3 dir = normalize(x + 2 * m_radius * d);
            return pdf_spherical(dir);
        }
        const Vec3 dir = normalize(x + 2 * m_radius * d);
        const Vec2 uv = dir2uv(dir);
        const Real inv_sin_theta = 1.0 / (std::abs(sin(uv.y * M_PI)) + 1e-8);
        const int i = int(uv.x * m_pdf_envmap.sizeX());
        const int j = int(uv.y * m_pdf_envmap.sizeY());
        return m_pdf_envmap.at(i, j) * inv_sin_theta * M_1_PI / 4;
    }

    virtual bool is_envmap() const override {
        return true;
    }

    virtual const Material* material() const override{
        return nullptr;
    }

    virtual Color3 evaluate_emission(const Vec3& wo, const Vec2& uv, const Vec3& p) const override{
        return eval_direct(wo);
    }

    ~EnvironmentMap(){
        spdlog::info("Deleting envmap !!!");
    }
};