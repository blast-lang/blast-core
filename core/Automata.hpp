#pragma once

namespace blast::core {

class IAutomata {
public:
    virtual ~IAutomata() = default;

    virtual void reset() = 0;
    virtual bool accepts() const = 0;
};

class Automata : public IAutomata {
public:
    Automata();

    void reset() override;
    bool accepts() const override;
};

} // namespace blast::core
