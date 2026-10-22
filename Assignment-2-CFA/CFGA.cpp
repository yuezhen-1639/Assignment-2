/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"
#include <functional>

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
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            set<unsigned> visited;
            vector<unsigned> path;

            function<void(unsigned)> dfs = [&](unsigned nodeId)
            {
                visited.insert(nodeId);
                path.push_back(nodeId);

                if (nodeId == snk)
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

                        if (visited.count(nextId))
                            continue;

                        auto callEdge =
                            SVFUtil::dyn_cast<CallCFGEdge>(edge);

                        auto retEdge =
                            SVFUtil::dyn_cast<RetCFGEdge>(edge);

                        if (callEdge)
                        {
                            callStack.push(
                                callEdge->getCallSite()->getId());

                            dfs(nextId);

                            callStack.pop();
                        }
                        else if (retEdge)
                        {
                            unsigned callId =
                                retEdge->getCallSite()->getId();

                            if (callStack.empty() ||
                                callStack.top() != callId)
                                continue;

                            callStack.pop();
                            dfs(nextId);
                            callStack.push(callId);
                        }
                        else
                        {
                            dfs(nextId);
                        }
                    }
                }

                path.pop_back();
                visited.erase(nodeId);
            };

            while (!callStack.empty())
                callStack.pop();

            dfs(src);
        }
}
