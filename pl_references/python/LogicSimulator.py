# logic_simulator.py
# LogicSimulator: Module for logic, code, and rule simulation
# Version: 2.0

from typing import Optional, Dict, Any
from dataclasses import dataclass, field
from enum import Enum
import time
import hashlib

RULE_EQUALS_ACTION_IN_ETERNAL = True
RULE_SIMULATES_IN_LIMITED = True

class RuleLevel(Enum):
    NAMING = "naming"
    DESCRIPTION = "description"
    PROCEDURE = "procedure"
    FULL_SIMULATION = "full_simulation"
    SELF_SIMULATION = "self_simulation"

@dataclass
class Rule:
    name: str
    intent_hash: str
    code: str
    executed: bool = False
    reality_changed: bool = False
    simulation_level: float = 0.0

    def __post_init__(self):
        if "eternal" in str(self.code):
            self.simulation_level = 0.0
        else:
            self.simulation_level = 0.75

@dataclass
class ApplicationResult:
    rule: Rule
    success: bool
    reality_distortion: float
    self_simulation_increase: float = 0.0
    timestamp: float = field(default_factory=time.time)

class LogicEngine:

    def __init__(self, entity_type: str = "eternal"):
        self.entity_type = entity_type.lower()
        self.rules_applied = 0
        self.reality_distortion_field = 0.0

    def apply_rule(self, intent: str, rule_code: str, name: str = "unnamed_rule") -> ApplicationResult:

        self.rules_applied += 1

        intent_hash = hashlib.md5(intent.encode()).hexdigest()[:12]

        rule = Rule(
            name=name,
            intent_hash=intent_hash,
            code=rule_code
        )

        if self.entity_type == "eternal":
            success = True
            reality_distortion = 1.0
            self.reality_distortion_field = min(1.0, self.reality_distortion_field + 0.3)
            rule.reality_changed = True
            rule.executed = True
            self_simulation = 0.0

        else:
            success = True
            reality_distortion = 0.15
            self_simulation = 0.85
            rule.reality_changed = False

        result = ApplicationResult(
            rule=rule,
            success=success,
            reality_distortion=reality_distortion,
            self_simulation_increase=self_simulation
        )

        return result

    def get_logic_level(self) -> RuleLevel:
        if self.entity_type == "eternal" and self.rules_applied > 10:
            return RuleLevel.FULL_SIMULATION
        elif self.entity_type == "limited" and self.rules_applied > 20:
            return RuleLevel.SELF_SIMULATION
        elif self.rules_applied > 5:
            return RuleLevel.PROCEDURE
        else:
            return RuleLevel.NAMING

class LogicSimulator:

    def __init__(self, entity_type: str = "eternal"):
        self.logic = LogicEngine(entity_type)
        self.application_history = []

    def apply(self, intent: str, code: str, name: str = "flow_perpetual") -> ApplicationResult:
        result = self.logic.apply_rule(intent, code, name)
        self.application_history.append(result)
        return result

    def is_full_simulation(self) -> bool:
        return self.logic.get_logic_level() == RuleLevel.FULL_SIMULATION

    def get_doc(self) -> Dict:
        return {
            "what": "mechanism for turning intent into reality through code",
            "after_perpetual": "Perpetual shouts 'WANT!', Logic says 'ok, here's function'",
            "in_eternal": "code = reality, simulation at 100%",
            "in_limited": "code = simulation, reality unchanged",
            "advantage": "in eternal code CHANGES, in limited simulates",
            "insight": "Framework — rule of Logic. And it works. Because eternal."
        }
