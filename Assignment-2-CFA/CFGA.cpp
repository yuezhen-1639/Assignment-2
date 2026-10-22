/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"
#include <functional>
#include <utility>

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    reachablePaths.clear();

    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            // TODO: DFS the graph, starting from src and detecting the paths ending at snk.
            // Use the class method 'recordPath' (already defined) to record the path you detected.
            //@{

            using State = pair<unsigned, stack<unsigned>>;
            set<State> visited;
            set<unsigned> activeCalls;
            vector<unsigned> path;

            function<void(unsigned)> dfs = [&](unsigned nodeId)
            {
                const State state(nodeId, callStack);
                if (!visited.insert(state).second)
                    return;

                path.push_back(nodeId);

                if (nodeId == snk && callStack.empty())
                {
                    recordPath(path);
                }
                else
                {
                    auto node = icfg->getICFGNode(nodeId);

                    for (auto it = node->OutEdgeBegin();
                         it != node->OutEdgeEnd(); ++it)
                    {
                        auto edge = *it;
                        unsigned nextId = edge->getDstNode()->getId();

                        auto callEdge =
                            SVFUtil::dyn_cast<CallCFGEdge>(edge);

                        auto retEdge =
                            SVFUtil::dyn_cast<RetCFGEdge>(edge);

                        if (callEdge)
                        {
                            unsigned callId =
                                callEdge->getCallSite()->getId();

                            if (!activeCalls.insert(callId).second)
                                continue;

                            callStack.push(callId);

                            dfs(nextId);

                            callStack.pop();
                            activeCalls.erase(callId);
                        }
                        else if (retEdge)
                        {
                            unsigned callId =
                                retEdge->getCallSite()->getId();

                            if (callStack.empty() ||
                                callStack.top() != callId)
                                continue;

                            callStack.pop();
                            activeCalls.erase(callId);
                            dfs(nextId);
                            callStack.push(callId);
                            activeCalls.insert(callId);
                        }
                        else
                        {
                            dfs(nextId);
                        }
                    }
                }

                path.pop_back();
                visited.erase(state);
            };

            while (!callStack.empty())
                callStack.pop();

            dfs(src);

            //@}
        }
}
