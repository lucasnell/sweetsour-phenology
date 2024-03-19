#include <Rcpp.h>
#include <vector>

#include "ode.h"

using namespace Rcpp;




class OnePlantSystemFunction
{
public:
    double m;
    double R;
    double d_yp;
    double d_b0;
    double d_bp;
    double g_yp;
    double g_b0;
    double g_bp;
    double L_0;
    double P_max;
    double q;
    double s_0_h;
    double h;
    double f_0_u;
    double F_tilde;
    double u;

    OnePlantSystemFunction(const double& m_,
                           const double& R_,
                           const double& d_yp_,
                           const double& d_b0_,
                           const double& d_bp_,
                           const double& g_yp_,
                           const double& g_b0_,
                           const double& g_bp_,
                           const double& L_0_,
                           const double& P_max_,
                           const double& q_,
                           const double& s_0_,
                           const double& h_,
                           const double& f_0_,
                           const double& F_tilde_,
                           const double& u_)
        : m(m_),
          R(R_),
          d_yp(d_yp_),
          d_b0(d_b0_),
          d_bp(d_bp_),
          g_yp(g_yp_),
          g_b0(g_b0_),
          g_bp(g_bp_),
          L_0(L_0_),
          P_max(P_max_),
          q(q_),
          s_0_h(std::pow(s_0_, h_)),
          h(h_),
          f_0_u(std::pow(f_0_, u_)),
          F_tilde(F_tilde_),
          u(u_) {};

    void operator()(const VecType& x, VecType& dxdt, const double t) {

        const double& Y(x[0]);
        const double& B(x[1]);
        const double& N(x[2]);

        double F = Y + B + N;

        // double F_u = std::pow(F, u);
        // double phi = F_u / (f_0_u + F_u);
        double FF_u = std::pow(F / (F + F_tilde), u);
        double phi = FF_u / (f_0_u + FF_u);
        double psi = s_0_h / (s_0_h + std::pow(B / F, h));
        double P = P_max * (q * psi + (1-q) * phi);

        double PF = P / F;
        double Lambda = PF / (L_0 + PF);

        double gamma_y = g_yp * Lambda;
        double gamma_b = g_b0 + g_bp * Lambda;

        double delta_y = d_yp * Lambda;
        double delta_b = d_b0 + d_bp * Lambda;

        double disp_y = delta_y * Y / F + gamma_y;
        double disp_b = delta_b * B / F + gamma_b;

        double& dYdt(dxdt[0]);
        double& dBdt(dxdt[1]);
        double& dNdt(dxdt[2]);

        dYdt = disp_y * N - m * Y;
        dBdt = disp_b * N - m * B;
        dNdt = R - N * (m + disp_y + disp_b);

        return;
    }
};


struct OnePlantObserver
{
    std::vector<VecType> data;
    std::vector<double> time;
    OnePlantObserver() : data(), time() {};

    void operator()(const VecType& x, const double& t) {
        data.push_back(x);
        time.push_back(t);
        return;
    }
};



//' @export
// [[Rcpp::export]]
NumericMatrix one_plant_ode(const double& m,
                            const double& R,
                            const double& d_yp,
                            const double& d_b0,
                            const double& d_bp,
                            const double& g_yp,
                            const double& g_b0,
                            const double& g_bp,
                            const double& L_0,
                            const double& P_max,
                            const double& q,
                            const double& s_0,
                            const double& h,
                            const double& f_0,
                            const double& F_tilde,
                            const double& u,
                            const double& dt = 0.1,
                            const double& max_t = 90.0,
                            const double& Y0 = 1.0,
                            const double& B0 = 1.0,
                            const double& N0 = 1.0) {

    VecType x(3U, 0.0);
    x[0] = Y0;
    x[1] = B0;
    x[2] = N0;

    OnePlantObserver obs;
    OnePlantSystemFunction system(m, R, d_yp, d_b0, d_bp, g_yp, g_b0, g_bp,
                                  L_0, P_max, q, s_0, h, f_0, F_tilde, u);

    boost::numeric::odeint::integrate_const(
        VecStepperType(), std::ref(system),
        x, 0.0, max_t, dt, std::ref(obs));

    size_t n_steps = obs.data.size();
    NumericMatrix output(n_steps, x.size()+2U);
    colnames(output) = CharacterVector::create("t", "Y", "B", "N", "P");
    double F,F_u, phi, psi;
    double f_0_u = std::pow(f_0, u);
    double s_0_h = std::pow(s_0, h);
    for (size_t i = 0; i < n_steps; i++) {
        output(i,0) = obs.time[i];
        F = 0;
        for (size_t j = 0; j < x.size(); j++) {
            output(i,j+1U) = obs.data[i][j];
            F += obs.data[i][j];
        }
        F_u = std::pow(F, u);
        phi = F_u / (f_0_u + F_u);
        psi = s_0_h / (s_0_h + std::pow(obs.data[i][1] / F, h));
        output(i, x.size()+1U) = P_max * (q * psi + (1-q) * phi);
    }
    return output;
}
