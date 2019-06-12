#include <AIToolbox/Bandit/Policies/ThompsonSamplingPolicy.hpp>

#include <random>

namespace AIToolbox::Bandit {
    ThompsonSamplingPolicy::ThompsonSamplingPolicy(const QFunction & q, const Vector & M2s, const std::vector<unsigned> & counts) :
            Base(q.size()), q_(q), M2s_(M2s), counts_(counts) {}

    size_t ThompsonSamplingPolicy::sampleAction() const {
        // For each arm, we sample its mean. Note that here we use a
        // standardized Student-t distribution, which we then scale using our
        // QFunction and counts parameters to obtain the correct mean
        // estimates.
        size_t bestAction = 0;
        double bestValue;
        {
            // We need at least 2 samples per arm with student-t to estimate
            // the variance.
            if (counts_[0] < 2)
                return 0;

            //     mu = est_mu - t * s / sqrt(n)
            // where
            //     s = 1 / (n-1) * sum_i (x_i - est_mu)^2
            // and
            //     t = student_t sample with n-1 degrees of freedom
            std::student_t_distribution<double> dist(counts_[0] - 1);
            bestValue = q_[0] - dist(rand_) * (M2s_[0] / (counts_[0] - 1))/ std::sqrt(counts_[0]);
        }
        for ( size_t a = 1; a < A; ++a ) {
            if (counts_[a] < 2)
                return a;

            std::student_t_distribution<double> dist(counts_[a] - 1);
            const double val = q_[a] - dist(rand_) * (M2s_[a] / (counts_[a] - 1))/ std::sqrt(counts_[a]);

            if ( val > bestValue ) {
                bestAction = a;
                bestValue = val;
            }
        }

        return bestAction;
    }

    double ThompsonSamplingPolicy::getActionProbability(const size_t & a) const {
        // The true formula here would be:
        //
        // \int_{-infty, +infty} PDF(N(a)) * CDF(N(0)) * ... * CDF(N(A-1))
        //
        // Where N(x) means the normal distribution obtained from the
        // parameters of that action.
        //
        // Instead we sample, which is easier and possibly faster if we just
        // want a rough approximation.
        constexpr unsigned trials = 100000;
        unsigned selected = 0;

        // We avoid recreating the distributions thousands of times here.
        std::vector<std::normal_distribution<double>> dists;
        dists.reserve(A);

        for (size_t aa = 0; aa < A; ++aa)
            dists.emplace_back(q_[aa], 1.0 / (counts_[aa] + 1));

        for (size_t i = 0; i < trials; ++i) {
            size_t bestAction = 0;
            double bestValue = dists[0](rand_);
            for ( size_t aa = 1; aa < A; ++aa ) {
                const auto val = dists[aa](rand_);

                if ( val > bestValue ) {
                    bestAction = aa;
                    bestValue = val;
                }
            }
            if (bestAction == a) ++selected;
        }
        return static_cast<double>(selected) / trials;
    }

    Vector ThompsonSamplingPolicy::getPolicy() const {
        // The true formula here would be:
        //
        // \int_{-infty, +infty} PDF(N(a)) * CDF(N(0)) * ... * CDF(N(A-1))
        //
        // Where N(x) means the normal distribution obtained from the
        // parameters of that action.
        //
        // Instead we sample, which is easier and possibly faster if we just
        // want a rough approximation.
        constexpr unsigned trials = 100000;

        Vector retval{A};
        retval.setZero();

        // We avoid recreating the distributions thousands of times here.
        std::vector<std::normal_distribution<double>> dists;
        dists.reserve(A);

        for (size_t aa = 0; aa < A; ++aa)
            dists.emplace_back(q_[aa], 1.0 / (counts_[aa] + 1));

        for (size_t i = 0; i < trials; ++i) {
            size_t bestAction = 0;
            double bestValue = dists[0](rand_);
            for ( size_t aa = 1; aa < A; ++aa ) {
                const auto val = dists[aa](rand_);

                if ( val > bestValue ) {
                    bestAction = aa;
                    bestValue = val;
                }
            }
            retval[bestAction] += 1.0;
        }
        retval /= retval.sum();
        return retval;
    }
}
