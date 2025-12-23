#include "A6Header.h"

using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
        OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                 "[options] <input-bitcode...>");

    SVF::LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVF::SVFIRBuilder builder;
    auto pag = builder.build();
    auto consg = new SVF::ConstraintGraph(pag);
    consg->dump();

    Andersen andersen(consg);
    auto cg = pag->getCallGraph();

    // TODO: 完成下面两个方法的实现
    andersen.runPointerAnalysis();
    andersen.updateCallGraph(cg);

    cg->dump();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}

void Andersen::runPointerAnalysis()
{
    // TODO: 完成此方法。点到集和工作列表在 A5Header.h 中定义。
    //  约束图的实现由 SVF 库提供。
    WorkList<unsigned> workList;

    for (auto it = consg->begin(); it != consg->end(); it++)
    {
        auto p = it->first;
        SVF::ConstraintNode *node = it->second;

        for (auto edge : node->getAddrInEdges())
        {
            SVF::AddrCGEdge *addrEdge = SVF::SVFUtil::dyn_cast<SVF::AddrCGEdge>(edge);
            auto srcId = addrEdge->getSrcID();
            auto dstId = addrEdge->getDstID();

            pts[dstId].insert(srcId);
            workList.push(dstId);
        }
    }

    while (!workList.empty())
    {
        auto p = workList.pop();
        SVF::ConstraintNode *pNode = consg->getConstraintNode(p);

        // 对于 pts(p) 中的每个 o
        for (auto o : pts[p])
        {
            // 对于每个 q --Store--> p
            for (auto edge : pNode->getStoreInEdges())
            {
                SVF::StoreCGEdge *storeEdge = SVF::SVFUtil::dyn_cast<SVF::StoreCGEdge>(edge);
                auto q = storeEdge->getSrcID();

                // q --Copy--> o 是否存在？
                bool exist = false;
                SVF::ConstraintNode *qNode = consg->getConstraintNode(q);
                for (auto copyEdge : qNode->getCopyOutEdges())
                {
                    if (copyEdge->getDstID() == o)
                    {
                        exist = true;
                        break;
                    }
                }

                if (!exist)
                {
                    consg->addCopyCGEdge(q, o);
                    workList.push(q);
                }
            }

            // 对于每个 p --Load--> r
            for (auto edge : pNode->getLoadOutEdges())
            {
                SVF::LoadCGEdge *loadEdge = SVF::SVFUtil::dyn_cast<SVF::LoadCGEdge>(edge);
                auto r = loadEdge->getDstID();

                // o --Copy--> r 是否存在？
                bool exist = false;
                SVF::ConstraintNode *rNode = consg->getConstraintNode(r);
                for (auto copyEdge : rNode->getCopyInEdges())
                {
                    if (copyEdge->getSrcID() == o)
                    {
                        exist = true;
                        break;
                    }
                }

                if (!exist)
                {
                    consg->addCopyCGEdge(o, r);
                    workList.push(o);
                }
            }
        }

        // 对于每个 p --Copy--> x
        for (auto edge : pNode->getCopyOutEdges())
        {
            SVF::CopyCGEdge *copyEdge = SVF::SVFUtil::dyn_cast<SVF::CopyCGEdge>(edge);
            auto x = copyEdge->getDstID();

            auto oldSize = pts[x].size();
            pts[x].insert(pts[p].begin(), pts[p].end());

            // pts(x) 是否改变？
            if (pts[x].size() != oldSize)
            {
                workList.push(x);
            }
        }

        // 对于每个 p --Gep.fld--> x
        for (auto edge : pNode->getGepOutEdges())
        {
            SVF::GepCGEdge *gepEdge = SVF::SVFUtil::dyn_cast<SVF::GepCGEdge>(edge);
            auto x = gepEdge->getDstID();

            auto oldSize = pts[x].size();
            for (auto o : pts[p])
            {
                auto fieldObj = consg->getGepObjVar(o, gepEdge);
                pts[x].insert(fieldObj);
            }

            // pts(x) 是否改变？
            if (pts[x].size() != oldSize)
            {
                workList.push(x);
            }
        }
    }
}

void Andersen::updateCallGraph(SVF::CallGraph *cg)
{
    // TODO: 完成此方法。
    //  调用图的实现由 SVF 库提供。
    const auto &indirectCallsites = consg->getIndirectCallsites();

    for (const auto &callsitePair : indirectCallsites)
    {
        auto callNode = callsitePair.first;
        auto funPtrId = callsitePair.second;

        auto callerFun = callNode->getCaller();

        const auto &pointsToSet = pts[funPtrId];

        for (auto calleeId : pointsToSet)
        {
            if (consg->isFunction(calleeId))
            {
                auto calleeFun = consg->getFunction(calleeId);

                cg->addIndirectCallGraphEdge(callNode, callerFun, calleeFun);
            }
        }
    }
}