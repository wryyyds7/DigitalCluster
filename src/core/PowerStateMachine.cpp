#include "PowerStateMachine.h"
#include <iostream>

PowerStateMachine::PowerStateMachine()
    : m_currentState(PowerState::OFF)
{
    initTransitions();
}

void PowerStateMachine::initTransitions()
{
    // 完整状态转移表：
    //
    //  OFF   --KEY_ACC-->   ACC
    //  OFF   --KEY_ON-->    ON     (跳过 ACC，直接全电)
    //  ACC   --KEY_ON-->    ON
    //  ACC   --KEY_OFF-->   OFF
    //  ON    --KEY_START--> START
    //  ON    --KEY_OFF-->   OFF
    //  ON    --KEY_ACC-->   ACC
    //  START --KEY_START--> CRANK
    //  CRANK --ENGINE_STARTED--> ON
    //  CRANK --CRANK_TIMEOUT--> ON (超时回退到 ON)
    //  CRANK --CRANK_FAIL--> ON

    m_transitions = {
        { PowerState::OFF,   PowerEvent::KEY_ACC,       PowerState::ACC   },
        { PowerState::OFF,   PowerEvent::KEY_ON,        PowerState::ON    },
        { PowerState::ACC,   PowerEvent::KEY_ON,        PowerState::ON    },
        { PowerState::ACC,   PowerEvent::KEY_OFF,       PowerState::OFF   },
        { PowerState::ON,    PowerEvent::KEY_START,     PowerState::START },
        { PowerState::ON,    PowerEvent::KEY_OFF,       PowerState::OFF   },
        { PowerState::ON,    PowerEvent::KEY_ACC,       PowerState::ACC   },
        { PowerState::START, PowerEvent::KEY_START,     PowerState::CRANK },
        { PowerState::CRANK, PowerEvent::ENGINE_STARTED, PowerState::ON   },
        { PowerState::CRANK, PowerEvent::CRANK_TIMEOUT, PowerState::ON   },
        { PowerState::CRANK, PowerEvent::CRANK_FAIL,    PowerState::ON   },
    };
}

bool PowerStateMachine::transition(PowerEvent event)
{
    // 在锁内查找转移规则并更新状态，拷贝回调和新状态后释放锁
    // 避免在持锁状态下执行回调，防止回调中再次调用 currentState() 导致死锁
    PowerState newState = m_currentState;
    bool found = false;
    StateCallback cbCopy;

    {
        std::lock_guard<std::mutex> lk(m_mutex);

        for (const auto& t : m_transitions) {
            if (t.from == m_currentState && t.event == event) {
                std::cout << "[PowerSM] " << stateName(m_currentState)
                          << " --" << static_cast<int>(event) << "--> "
                          << stateName(t.to) << "\n";
                m_currentState = t.to;
                newState = t.to;
                cbCopy = stateChanged;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        std::cerr << "[PowerSM] Invalid transition: in state "
                  << stateName(newState)
                  << " with event " << static_cast<int>(event) << "\n";
        return false;
    }

    // 在锁外执行回调，避免死锁
    if (cbCopy) cbCopy(newState);
    return true;
}

PowerState PowerStateMachine::currentState() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_currentState;
}

std::string PowerStateMachine::stateName(PowerState s)
{
    switch (s) {
        case PowerState::OFF:   return "OFF";
        case PowerState::ACC:   return "ACC";
        case PowerState::ON:    return "ON";
        case PowerState::START: return "START";
        case PowerState::CRANK: return "CRANK";
    }
    return "UNKNOWN";
}

std::string PowerStateMachine::stateDescription(PowerState s)
{
    switch (s) {
        case PowerState::OFF:   return "System Off";
        case PowerState::ACC:   return "Accessory Power";
        case PowerState::ON:    return "Ignition On";
        case PowerState::START: return "Start Request";
        case PowerState::CRANK: return "Engine Cranking";
    }
    return "Unknown";
}

std::vector<PowerEvent> PowerStateMachine::allowedEvents() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    std::vector<PowerEvent> events;
    for (const auto& t : m_transitions) {
        if (t.from == m_currentState) events.push_back(t.event);
    }
    return events;
}
