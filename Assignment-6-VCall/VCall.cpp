#include "A6Header.h"

using namespace llvm;
using namespace std;

int main(int argc, char** argv)
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
    andersen.runPointerAnalysis();
    andersen.updateCallGraph(cg);

    cg->dump();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
	return 0;
}


void Andersen::runPointerAnalysis()
{
    // 使用所有约束节点初始化工作队列
    WorkList<SVF::ConstraintNode*> worklist;
    
    // 初始化点到集合：每个指针先包含自身
    for (auto it = consg->begin(); it != consg->end(); ++it) {
        SVF::ConstraintNode* node = it->second;
        pts[node->getId()].insert(node->getId());
        worklist.push(node);
    }
    
    // 处理工作队列直到为空
    while (!worklist.empty()) {
        SVF::ConstraintNode* node = worklist.pop();
        
        // 处理该节点的所有出边
        for (auto edge : node->getOutEdges()) {
            SVF::ConstraintNode* dstNode = edge->getDstNode();
            
            // 处理不同类型的约束边
            switch (edge->getEdgeKind()) {
                case SVF::ConstraintEdge::Copy: {
                    // Copy 约束：dst = src
                    // 合并点到集合
                    bool changed = false;
                    for (auto pt : pts[node->getId()]) {
                        if (pts[dstNode->getId()].insert(pt).second) {
                            changed = true;
                        }
                    }
                    if (changed) {
                        worklist.push(dstNode);
                    }
                    break;
                }
                case SVF::ConstraintEdge::Addr: {
                    // 取地址约束：dst = &src
                    // 将 src 加入 dst 的点到集合
                    if (pts[dstNode->getId()].insert(node->getId()).second) {
                        worklist.push(dstNode);
                    }
                    break;
                }
                case SVF::ConstraintEdge::Load: {
                    // Load 约束：dst = *src
                    // 对 src 的每个指向对象 o，将 o 的点到集合并入 dst 的点到集合
                    bool changed = false;
                    for (auto srcObj : pts[node->getId()]) {
                        auto srcObjNode = consg->getConstraintNode(srcObj);
                        if (srcObjNode) {
                            for (auto pt : pts[srcObj]) {
                                if (pts[dstNode->getId()].insert(pt).second) {
                                    changed = true;
                                }
                            }
                        }
                    }
                    if (changed) {
                        worklist.push(dstNode);
                    }
                    break;
                }
                case SVF::ConstraintEdge::Store: {
                    // Store 约束：*dst = src
                    // 对 dst 的每个指向对象 o，将 src 的点到集合并入 o 的点到集合
                    bool changed = false;
                    for (auto dstObj : pts[node->getId()]) {
                        auto dstObjNode = consg->getConstraintNode(dstObj);
                        if (dstObjNode) {
                            for (auto pt : pts[dstNode->getId()]) {
                                if (pts[dstObj].insert(pt).second) {
                                    changed = true;
                                }
                            }
                            if (changed) {
                                worklist.push(dstObjNode);
                            }
                        }
                    }
                    break;
                }
                default:
                    // 暂时跳过其它约束类型
                    break;
            }
        }
    }
}


void Andersen::updateCallGraph(SVF::CallGraph* cg)
{
    // 通过 ConstraintGraph 遍历所有间接调用点
    // （ConstraintGraph 内部持有 PAG 并暴露 getIndirectCallsites()）
    for (auto &cs_pair : consg->getIndirectCallsites()) {
        const SVF::CallICFGNode* cs = cs_pair.first;
        SVF::NodeID funPtrNode = cs_pair.second;

        // 查找函数指针节点的点到集合
        auto ptsIt = pts.find(funPtrNode);
        if (ptsIt == pts.end()) continue;

        // 获取调用者函数（在某些上下文可能为 nullptr）
        const SVF::FunObjVar* callerFun = nullptr;
        if (cs) callerFun = cs->getCaller();

        // 对点到集合中的每个函数对象，添加间接调用边。
        // 使用 ConstraintGraph 的辅助方法检查/获取函数对象，避免直接访问 PAG。
        for (auto funObjId : ptsIt->second) {
            if (!consg->isFunction(funObjId))
                continue;
            const SVF::FunObjVar* calleeFun = consg->getFunction(funObjId);
            if (!calleeFun) continue;

            // 确保被调用函数存在于调用图中，然后添加间接调用边
            cg->addCallGraphNode(calleeFun);
            cg->addIndirectCallGraphEdge(cs, callerFun, calleeFun);
        }
    }
}