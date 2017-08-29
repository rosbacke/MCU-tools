/*
 * StateChart.cpp
 *
 *  Created on: 12 nov. 2016
 *      Author: mikaelr
 */

#include "StateChart.h"
// #include "utility/Utility.h"

void
FsmStaticData::addStateBase(int stateId, int parentId, size_t size,
                            CreateFkn fkn)
{
    int level = 0;
    if (stateId != parentId)
    {
        auto parent = findState(parentId);
        level = parent->m_level + 1;
    }
    while (static_cast<int>(m_objectSizes.size()) <= level)
        m_objectSizes.push_back(0);

    if (m_objectSizes[level] < size)
        m_objectSizes[level] = size;

    m_states[stateId] = StateInfo(parentId, level, fkn);
}

void
FsmBaseMember::possiblyDoTransition(FsmBaseBase* fbb)
{
    while (m_nextState != FsmStaticData::nullStateId)
    {
        auto i = m_setup.findState(m_nextState);
        if (i)
        {
            m_nextState = FsmStaticData::nullStateId;
            doTransition(i, fbb);
        }
    }
}

void
FsmBaseMember::doEntry(const StateInfo* newState, FsmBaseBase* fsm)
{
    int level = newState->m_level;
    auto& frame = m_stackFrames[level];
    auto& storeVec = frame.m_stateStorage;
    frame.m_activeState.reset(newState->m_maker(storeVec.get(), fsm));
};

void
FsmBaseMember::doExit(const StateInfo* currState)
{
    m_stackFrames[currState->m_level].m_activeState.reset(nullptr);
}

void
FsmBaseMember::setupTransition(const StateInfo* nextInfo, FsmBaseBase* fsm)
{
    // target lvl down to src.
    const int targetLevel = nextInfo->m_level;
    stateInfo(targetLevel) = nextInfo;
    while (nextInfo->m_level > 0)
    {
        nextInfo = m_setup.findState(nextInfo->m_parentId);
        stateInfo(nextInfo->m_level) = nextInfo;
    }

    m_currentInfo = stateInfo(0);
    doEntry(m_currentInfo, fsm);
    // Reached same level state. Start entry up again.
    while (m_currentInfo->m_level < targetLevel)
    {
        m_currentInfo = stateInfo(m_currentInfo->m_level + 1);
        doEntry(m_currentInfo, fsm);
    }
}

// Precondition: both nextInfo and m_currentInfo point to a valid info.
void
FsmBaseMember::doTransition(const StateInfo* nextInfo, FsmBaseBase* fsm)
{
    auto targetLevel = nextInfo->m_level;

    // Special case: Transition to self should give exit/entry action
    if (m_currentInfo == nextInfo)
    {
        doExit(m_currentInfo);
        doEntry(m_currentInfo, fsm);
        return;
    }

    // src level down to nextInfos level.
    while (m_currentInfo->m_level > nextInfo->m_level)
    {
        doExit(m_currentInfo);
        m_currentInfo = stateInfo(m_currentInfo->m_level - 1);
    }

    // nextInfos level down to src level.
    while (nextInfo->m_level > m_currentInfo->m_level)
    {
        stateInfo(nextInfo->m_level) = nextInfo;
        nextInfo = m_setup.findState(nextInfo->m_parentId);
    }

    // Invariant: nextInfo->m_level == m_currentInfo->m_level.
    // both level down. (same level)
    int level = m_currentInfo->m_level;
    while (nextInfo != m_currentInfo && level > 0)
    {
        doExit(m_currentInfo);
        stateInfo(level) = nextInfo;
        level--;
        m_currentInfo = stateInfo(level);
        nextInfo = m_setup.findState(nextInfo->m_parentId);
    }

    // No root state, handle transition at level 0.
    if (nextInfo != m_currentInfo)
    {
        doExit(m_currentInfo);
        stateInfo(0) = nextInfo;
        m_currentInfo = nextInfo;
        doEntry(m_currentInfo, fsm);
    }

    // Done with all exists. Possibly start going up again.
    // Invariant: m_currentInfo point to a state we have already entered.
    // prior to entering the while loop.
    while (m_currentInfo->m_level < targetLevel)
    {
        m_currentInfo = stateInfo(m_currentInfo->m_level + 1);
        doEntry(m_currentInfo, fsm);
    }
}

void
FsmBaseMember::cleanup()
{
    while (m_currentInfo->m_level > 0)
    {
        doExit(m_currentInfo);
        m_currentInfo = stateInfo(m_currentInfo->m_level - 1);
    }
    doExit(m_currentInfo);
    m_currentInfo = nullptr;
}

void
FsmBaseMember::transition(int id)
{
    m_nextState = id;
}

void
FsmBaseMember::setStartState(int id, FsmBaseBase* fsm)
{
    const auto& sizes = m_setup.sizes();
    m_stackFrames.reserve(sizes.size());
    for (const auto& el : sizes)
    {
        m_stackFrames.emplace_back(el);
    }

    m_currentInfo = nullptr;
    setupTransition(m_setup.findState(id), fsm);
}
