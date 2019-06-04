#include <AIToolbox/Factored/MDP/Policies/QGreedyPolicy.hpp>

#include <AIToolbox/Utils/Core.hpp>
#include <AIToolbox/Factored/Utils/Core.hpp>
#include <AIToolbox/Factored/Bandit/Algorithms/Utils/VariableElimination.hpp>

namespace AIToolbox::Factored::MDP {
    QGreedyPolicy::QGreedyPolicy(State s, Action a, const FilterMap<QFunctionRule> & q) :
            Base(std::move(s), std::move(a)), qc_(&q), qm_(nullptr) {}

    QGreedyPolicy::QGreedyPolicy(State s, Action a, const FactoredMatrix2D & q) :
            Base(std::move(s), std::move(a)), qc_(nullptr), qm_(&q) {}

    Action QGreedyPolicy::sampleAction(const State & s) const {
        using VE = Bandit::VariableElimination;
        VE ve;
        if (qc_) {
            const auto rules = qc_->filter(s);
            return std::get<0>(ve(A, rules));
        } else {
            VE::GVE::Graph graph(A.size());

            for (const auto & basis : qm_->bases) {
                auto & factorNode = graph.getFactor(basis.actionTag)->getData();
                const bool isNew = factorNode.size() == 0;
                const size_t x = toIndexPartial(basis.tag, S, s);

                if (isNew) factorNode.reserve(basis.values.cols());

                for (size_t y = 0; y < static_cast<size_t>(basis.values.cols()); ++y) {
                    if (isNew) {
                        factorNode.emplace_back(y, VE::Factor{basis.values(x, y), {}});
                    } else {
                        factorNode[y].second.first += basis.values(x, y);
                    }
                }
            }
            return std::get<0>(ve(A, graph));
        }
    }

    double QGreedyPolicy::getActionProbability(const State & s, const Action & a) const {
        if (veccmp(a, sampleAction(s)) == 0) return 1.0;
        return 0.0;
    }
}
