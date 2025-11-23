/**
 * A6Header.h
 * 作者: kisslune
 */

#ifndef ANSWERS_A5HEADER_H
#define ANSWERS_A5HEADER_H

#include "SVF-LLVM/SVFIRBuilder.h"

/// 点到集合类型
using PTS = std::map<unsigned, std::set<unsigned>>;

/**
 * FIFO 工作队列
 */
template<class T>
class WorkList
{
public:
    /// 判断工作队列是否为空
    inline bool empty() const
    { return data_list.empty(); }

    /// 清空工作队列
    inline void clear()
    {
        data_list.clear();
        data_set.clear();
    }

    /// 将元素推入队列尾部
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

    /// 从队列头部弹出元素
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
    std::deque<T> data_list;     ///< 双端队列，用于从头尾访问元素
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
    /// 将结果写入文件
    void dumpResult();

protected:
    SVF::ConstraintGraph *consg;
    PTS pts;
};


#endif //ANSWERS_A5HEADER_H
