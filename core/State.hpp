#pragma once

namespace blast::core {

class IState {
public:
    virtual ~IState() = default;

    virtual bool accepting() const = 0;
};

class State : public IState {
public:
    explicit State(bool accepting = false);

    bool accepting() const override { return m_accepting; }

private:
    bool m_accepting;
};

} // namespace blast::core
