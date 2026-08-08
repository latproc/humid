/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __MACHINECOMMAND_ACTION
#define __MACHINECOMMAND_ACTION 1

#include "Action.h"
#include "symboltable.h"

/*
    A MachineCommand executes a list of actions,
    usage example:

    MachineCommand f;
    f.addAction(new WaitAction,new ActionParameterList(1));
    f.addAction(new SetStateAction,new ActionParameterList(output, "on"));
    Action::Status status = *(f);
    if (status == Action::Error) {
        // handle error
    }
    else {
    }
*/

class MachineInstance;

class MachineCommandTemplate : public ActionTemplate {
  public:
    MachineCommandTemplate(CStringHolder cmd_name, CStringHolder state, bool auto_switch = false)
        : command_name(cmd_name), state_name(state), timeout(0), switch_state(auto_switch) {}
    virtual Action *factory(MachineInstance *mi);

    std::ostream &operator<<(std::ostream &out) const {
        return out << command_name.get() << " " << state_name.get();
    }
    void setActionTemplates(std::list<ActionTemplate *> &new_actions);
    void setActionTemplate(ActionTemplate *action);

    CStringHolder &getStateName() { return state_name; }

    std::vector<ActionTemplate *> action_templates;
    CStringHolder command_name;

  private:
    CStringHolder state_name;

  public:
    long timeout;
    bool switch_state;
};

class MachineCommand : public Action {
  public:
    MachineCommand(MachineInstance *mi, MachineCommandTemplate *mct);
    ~MachineCommand();
    void addAction(Action *a, ActionParameterList *params);
    Status checkAction(Action *a, Status stat);
    Status runActions();
    Status run();
    Status checkComplete();
    void reset();
    void setActions(std::list<Action *> &new_actions);
    const std::string name() const { return command_name.get(); }
    const std::string stateName() const { return state_name.get(); }
    virtual std::ostream &operator<<(std::ostream &out) const;

    CStringHolder &getStateName() { return state_name; }
    bool autoSwitch() { return switch_state; }

  private:
    std::vector<Action *> actions;
    size_t last_step, current_step;
    CStringHolder command_name;

    CStringHolder state_name;
    Trigger *timeout_trigger;
    bool switch_state;
};

#endif
