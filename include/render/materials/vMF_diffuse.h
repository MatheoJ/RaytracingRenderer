#pragma once
#include <cmath>
#include <render/materials/material.h>
#include <render/texture.h>
#include <render/samplers/sampler.h>
#include <render/json.h>


// Implementation from : https://www.shadertoy.com/view/MXKSWW

#define P  ( sqrt(3.14159265359)/2. )

Real erf(Real x) {
    Real e = exp(-x*x);
    return copysign(1.0 / P * sqrt( 1. - e ) * ( P + 31./200.*e - 341./8000. *e*e ), x);
}

// -------------------------------
// VMF DIFFUSE BRDF IMPLEMENTATION
// -------------------------------

Real Coth(Real x)
{
    return (exp(-x) + exp(x))/(-exp(-x) + exp(x));
}

Real Sinh(Real x)
{
    return -0.5*1.0/exp(x) + exp(x)/2.;
}

// cross section for the Beckmann NDF with roughness m and direction cosine u
Real sigmaBeckmannExpanded(Real u, Real m)
{
    if(0.0 == m) 
        return (u + std::abs(u))/2.;

    Real m2 = m * m;

    if(1.0 == u)
        return 1.0 - 0.5 * m2;
        
    Real expansionTerm = -0.25 * m2 * (u + std::abs(u)); // accurate approximation for m < 0.25 that avoids numerical issues
    
    Real u2 = u * u;
    return ((exp(u2/(m2*(-1.0 + u2)))*m*sqrt(1.0 - u2))/ M_2_SQRTPI + 
     u*(1.0 + erf(u/(m*sqrt(1.0 - u2)))))/2. + expansionTerm;
}

// vmf sigma (cross section)
Real sigmaVMF(Real u, Real m)
{
    if(m < 0.25)
        return sigmaBeckmannExpanded(u, m);
        
    Real m2 = m * m;
    Real m4 = m2 * m2;
    Real m8 = m4 * m4;

    Real u2 = u * u;
    Real u4 = u2 * u2;
    Real u6 = u2 * u4;
    Real u8 = u4 * u4;
    Real u10 = u6 * u4;
    Real u12 = u6 * u6;

    Real coth2m2 = Coth(2./m2);
    Real sinh2m2 = Sinh(2./m2);
        
    if(m > 0.9)
        return 0.25 - 0.25*u*(m2 - 2.*coth2m2) + 0.0390625*(-1. + 3.*u2)*(4. + 3.*m4 - 6.*m2*coth2m2);
        
    return 0.25 - 0.25*u*(m2 - 2.*coth2m2) + 0.0390625*(-1. + 3.*u2)*(4. + 3.*m4 - 
      6.*m2*coth2m2) - 0.000732421875*(3. - 30.*u2 + 35.*u4)*(16. + 180.*m4 + 105.*m8 - 
      10.*m2*(8. + 21.*m4)*coth2m2) + 0.000049591064453125*(-5. + 105.*u2 - 315.*u4 + 231.*u6)*
      (64. + 105.*m4*(32. + 180.*m4 + 99.*m8) - 42.*m2*(16. + 240.*m4 + 495.*m8)*coth2m2) + 
      (1.0132789611816406e-6*(35. - 1260.*u2 + 6930.*u4 - 12012.*u6 + 6435.*u8)*(1. + coth2m2)*
      (-256. - 315.*m4*(128. + 33.*m4*(80. + 364.*m4 + 195.*m8)) + 18.*m2*(256. + 385.*m4*
      (32. + 312.*m4 + 585.*m8))*coth2m2)*sinh2m2)/exp(2./m2) - (9.12696123123169e-8*(-63. + 3465.*u2 
      - 30030.*u4 + 90090.*u6 - 109395.*u8 + 46189.*u10)*(1. + coth2m2)*(-1024. - 
      495.*m4*(768. + 91.*m4*(448. + 15.*m4*(448. + 1836.*m4 + 969.*m8))) + 110.*m2*(256. + 117.*m4*
      (256. + 21.*m4*(336. + 85.*m4*(32. + 57.*m4))))*coth2m2)*sinh2m2)/exp(2./m2)
      + (4.3655745685100555e-9*(231. - 18018.*u2 + 225225.*u4 - 1.02102e6*u6 + 2.078505e6*u8 
      - 1.939938e6*u10 + 676039.*u12)*(1. + coth2m2)*(-4096. - 3003.*m4*(1024. + 
      45.*m4*(2560. + 51.*m4*(1792. + 285.*m4*(80. + 308.*m4 + 161.*m8)))) + 78.*m2*(2048. + 385.*m4*
      (1280. + 153.*m4*(512. + 57.*m4*(192. + 35.*m4*(40. + 69.*m4)))))*coth2m2)*sinh2m2)/exp(2./m2);
}

Vec3 Erf(Vec3 c)
{
    return Vec3(erf(c.x), erf(c.y), erf(c.z));
}

Vec3 nonNegative(Vec3 c)
{
    return Vec3(std::max(0.0, c.x), std::max(0.0, c.y), std::max(0.0, c.z));
}

Vec3 fm(Real ui, Real uo, Real r, Vec3 c)
{
    Vec3 C = sqrt(1.0 - c);
    Vec3 Ck = (1.0 - 0.5441615108674713*C - 0.45302863761693374*(1.0 - c))/(1.0 + 1.4293127703064865*C);
    Vec3 Ca = c/pow(1.0075 + 1.16942*C,atan((0.0225272 + (-0.264641 + r)*r)*Erf(C)));
    return nonNegative(0.384016*(-0.341969 + Ca)*Ca*Ck*(-0.0578978/(0.287663 + ui*uo) + std::abs(-0.0898863 + tanh(r))));
}


Vec3 vMFdiffuseBRDF(Real ui, Real uo, Real phi, Real r, Vec3 c)
{
    if(0.0 == r) return c * M_1_PI;
    
    Real m = -log(1.0-sqrt(r));
    Real sigmai = sigmaVMF(ui,m);
    Real sigmao = sigmaVMF(uo,m);
    Real sigmano = sigmaVMF(-uo,m);
    Real sigio = sigmai * sigmao;
    Real sigdenom = uo * sigmai + ui * sigmano;

    Real r2 = r * r;
    Real r25 = r2 * sqrt(r);
    Real r3 = r * r2;
    Real r4 = r2 * r2;
    Real r45 = r4 * sqrt(r);
    Real r5 = r3 * r2;

    Real ui2 = ui * ui;
    Real uo2 = uo * uo;
    Real sqrtuiuo = sqrt((1.0 - ui2) * (1.0 - uo2));

    Real C100 = 1.0 + (-0.1 * r + 0.84 * r4) / (1.0 + 9.0 * r3);
    Real C101 = (0.0173 * r + 20.4 * r2 - 9.47 * r3)/(1.0 + 7.46 * r);
    Real C102 = (-0.927 * r + 2.37 * r2)/(1.24 + r2);
    Real C103 = (-0.11 * r - 1.54 * r2)/(1.0 - 1.05 * r + 7.1 * r2);
    Real f10 =  ((C100 + C101 * ui * uo + C102 * ui2 * uo2 + C103 * (ui2 + uo2)) * sigio) / sigdenom;

    Real C110 = (0.54*r - 0.182*r3)/(1. + 1.32*r2);
    Real C111 = (-0.097*r + 0.62*r2 - 0.375*r3)/(1. + 0.4*r3);
    Real C112 = 0.283 + 0.862*r - 0.681*r2;
    Real f11 = (sqrtuiuo * (C110 + C111 * ui * uo)) * pow(sigio, C112) / sigdenom;

    Real C120 = (2.25*r + 5.1*r2)/(1.0 + 9.8*r + 32.4*r2);
    Real C121 = (-4.32*r + 6.0*r3)/(1.0 + 9.7*r + 287.0*r3);
    Real f12 = ((1.0 - ui2) * (1.0 - uo2) * (C120 + C121 * uo) * (C120 + C121 * ui))/(ui + uo);

    Real C200 = (0.00056*r + 0.226*r2)/(1.0 + 7.07*r2);
    Real C201 = (-0.268*r + 4.57*r2 - 12.04*r3)/(1.0 + 36.7*r3);
    Real C202 = (0.418*r + 2.52*r2 - 0.97*r3)/(1.0 + 10.0*r2);
    Real C203 = (0.068*r - 2.25*r2 + 2.65*r3)/(1.0 + 21.4*r3);
    Real C204 = (0.05*r - 4.22*r3)/(1.0 + 17.6*r2 + 43.1*r3);
    Real f20 = (C200 + C201 * ui * uo + C203*ui2*uo2 + C202*(ui + uo) + C204*(ui2 + uo2))/(ui + uo);

    Real C210 = (-0.049*r - 0.027*r3)/(1.0 + 3.36*r2);
    Real C211 = (2.77*r2 - 8.332*r25 + 6.073*r3)/(1.0 + 50.0*r4);
    Real C212 = (-0.431*r2 - 0.295*r3)/(1.0 + 23.9*r3);
    Real f21 = (sqrtuiuo * (C210 + C211*ui*uo + C212*(ui + uo)))/(ui + uo);

    Real C300 = (-0.083*r3 + 0.262*r4)/(1.0 - 1.9*r2 + 38.6*r4);
    Real C301 = (-0.627*r2 + 4.95*r25 - 2.44*r3)/(1.0 + 31.5*r4);
    Real C302 = (0.33*r2 + 0.31*r25 + 1.4*r3)/(1.0 + 20.0*r3);
    Real C303 = (-0.74*r2 + 1.77*r25 - 4.06*r3)/(1.0 + 215.0*r5);
    Real C304 = (-1.026*r3)/(1.0 + 5.81*r2 + 13.2*r3);
    Real f30 = (C300 + C301*ui*uo + C303*ui2*uo2 + C302*(ui + uo) + C304*(ui2 + uo2))/(ui + uo);

    Real C310 = (0.028*r2 - 0.0132*r3)/(1.0 + 7.46*r2 - 3.315*r4);
    Real C311 = (-0.134*r2 + 0.162*r25 + 0.302*r3)/(1.0 + 57.5*r45);
    Real C312 = (-0.119*r2 + 0.5*r25 - 0.207*r3)/(1.0 + 18.7*r3);
    Real f31 =  (sqrtuiuo * (C310 + C311*ui*uo + C312*(ui + uo)))/(ui + uo);

    return M_1_PI * (
        c *         std::max(0.0, f10 + f11 * cos(phi) * 2. + f12 * cos(2.0 * phi) * 2.) +
        c * c *     std::max(0.0, f20 + f21 * cos(phi) * 2.) +
        c * c * c * std::max(0.0, f30 + f31 * cos(phi) * 2.)
        ) + fm(ui, uo, r, c);
}

Vec3 vMFDiffuseAlbedoMapping(Vec3 kd, Real roughness)
{
    Real roughness2 = roughness * roughness;
    Real s = 0.64985f + 0.631112f * roughness + 1.38922f * roughness2;
    return (-1.f + kd + sqrt(1.f - 2. * kd + kd * kd + 4.f * s * s * kd * kd)) / (2.f * s * kd) * sqrt(roughness) + (1.f - sqrt(roughness)) * kd;
}

class vMF_diffuse : public Material {
	Texture<Color4> m_albedo;
    Texture<Real> m_roughness;

public:
	vMF_diffuse(const json& param = json::object()){
		m_albedo = create_texture_color(param, "albedo", Color4(0.518, 0.331, 0.18, 1.0));
        m_roughness = create_texture_float(param, "roughness", 0.5);
	}

	virtual std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
		Vec2 s_copy = sample;
		Color3 albedo = eval(m_albedo, uv, p).xyz();
        Vec3 wi = normalize(sample_hemisphere_cos(s_copy));
        Real uo = wo.z;
        Real ui = wi.z;

        Real cosThetaI = wi.z, sinThetaI = sqrt(1.0 - cosThetaI * cosThetaI);
        Real cosThetaO = wo.z, sinThetaO = sqrt(1.0 - cosThetaO * cosThetaO);

        Real cosPhiDiff = 0.0;
        if (sinThetaI > 0.0 && sinThetaO > 0.0) {
            /* Compute cos(phiO-phiI) using the half-angle formulae */
            Real sinPhiI = clamp(wi.y / sinThetaI, -1.0, 1.0),
                  cosPhiI = clamp(wi.x / sinThetaI, -1.0, 1.0),
                  sinPhiO = clamp(wo.y / sinThetaO, -1.0, 1.0),
                  cosPhiO = clamp(wo.x / sinThetaO, -1.0, 1.0);
            cosPhiDiff = cosPhiI * cosPhiO + sinPhiI * sinPhiO;
        }
        Real phi = acos(cosPhiDiff);

        Real r = clamp(eval(m_roughness, uv, p), 0.0, .9999);
        Color3 c = vMFDiffuseAlbedoMapping(albedo, r);

        return std::optional<SampledDirection>{{vMFdiffuseBRDF(ui, uo, phi, r, c), wi}};
	}

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
		if (wi.z > 0.0 /* & wo.z > 0.0 */){
            Color3 albedo = eval(m_albedo, uv, p).xyz();
        	Real uo = wo.z;
            Real ui = wi.z;

            Real cosThetaI = wi.z, sinThetaI = sqrt(1.0 - cosThetaI * cosThetaI);
            Real cosThetaO = wo.z, sinThetaO = sqrt(1.0 - cosThetaO * cosThetaO);

            Real cosPhiDiff = 0.0;
            if (sinThetaI > 0.0 && sinThetaO > 0.0) {
                /* Compute cos(phiO-phiI) using the half-angle formulae */
                Real sinPhiI = clamp(wi.y / sinThetaI, -1.0, 1.0),
                    cosPhiI = clamp(wi.x / sinThetaI, -1.0, 1.0),
                    sinPhiO = clamp(wo.y / sinThetaO, -1.0, 1.0),
                    cosPhiO = clamp(wo.x / sinThetaO, -1.0, 1.0);
                cosPhiDiff = cosPhiI * cosPhiO + sinPhiI * sinPhiO;
            }
            Real phi = acos(cosPhiDiff);

            Real r = clamp(eval(m_roughness, uv, p), 0.0, .9999);
            Color3 c = vMFDiffuseAlbedoMapping(albedo, r);

            return vMFdiffuseBRDF(ui, uo, phi, r, c) * wi.z;
		}
		return Vec3(0.0);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
		return pdf_hemisphere_cos(wi);
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return false; // Pas d'interactions spéculaires
    }
};