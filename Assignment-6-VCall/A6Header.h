#ifndef ANSWERS_A5HEADER_H
#define ANSWERS_A5HEADER_H

#include "SVF-LLVM/SVFIRBuilder.h"

/// 点到集类型定义
using PTS = std::map<unsigned, std::set<unsigned>>;

//FIFO 工作列表
template<class T>
class WorkList
{
public:
    /// 检查工作列表是否为空。
    inline bool empty() const
    { return data_list.empty(); }

    /// 清空工作列表
    inline void clear()
    {
        data_list.clear();
        data_set.clear();
    }

    /// 将数据推入工作列表末尾。
    inline bool push(const T &data)
    {
        if (this->data_set.find(data) == data_set.end())
        {
            this->data_list.push_back(data);
            this->data_set.insert(data);
            return true;
        }
        else
            return false;
    }

    /// 从工作列表头部弹出数据。
    inline T pop()
    {
        assert(!this->empty() && "work list is empty");
        T data = this->data_list.front();
        this->data_list.pop_front();
        this->data_set.erase(data);
        return data;
    }

protected:
    std::unordered_set<T> data_set;       ///< 用于避免重复元素
    std::deque<T> data_list;     ///< 可从两端访问元素
};


/// Andersen 求解器
class Andersen
{
public:
    explicit Andersen(SVF::ConstraintGraph *consg) :
            consg(consg)
    {}

    /// 运行指针分析
    void runPointerAnalysis();
    /// 更新调用图
    void updateCallGraph(SVF::CallGraph* cg);
    /// 将结果输出到文件
    void dumpResult();

protected:
    SVF::ConstraintGraph *consg;
    PTS pts;
};


#endif //ANSWERS_A5HEADER_H