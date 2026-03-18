# choice_mechanism.py
# ChoiceMechanism: Module for choice/constraint simulation
# Version: 2.0

from typing import Optional, Dict, List, Any, Set
from dataclasses import dataclass, field
from enum import Enum
import random
import time

CHOICE_IS_IRREVERSIBLE = True
UNCHOSEN_ARE_LOST = True

class CollapseType(Enum):
    CONSCIOUS = "conscious"
    UNCONSCIOUS = "unconscious"
    FORCED = "forced"
    RANDOM = "random"

class ChoiceResult(Enum):
    ACTUALIZED = "actualized"
    LOST = "lost"
    PENDING = "pending"

@dataclass
class Possibility:
    id: str
    potential: float
    status: ChoiceResult = ChoiceResult.PENDING

    def actualize(self):
        self.status = ChoiceResult.ACTUALIZED

    def lose(self):
        self.status = ChoiceResult.LOST

@dataclass
class PossibilitySpace:
    possibilities: List[Possibility]
    collapsed: bool = False
    chosen: Optional[Possibility] = None

    @property
    def count(self) -> int:
        return len(self.possibilities)

    @property
    def all_potentials(self) -> float:
        return sum(p.potential for p in self.possibilities)

    def get_lost(self) -> List[Possibility]:
        return [p for p in self.possibilities if p.status == ChoiceResult.LOST]

@dataclass
class ChoiceEvent:
    before: int
    after: int
    chosen: Possibility
    lost: List[Possibility]
    collapse_type: CollapseType
    timestamp: float = field(default_factory=time.time)
    reversible: bool = False

    @property
    def loss_count(self) -> int:
        return len(self.lost)

class ChoiceEngine:

    def __init__(self):
        self.history: List[ChoiceEvent] = []

    def collapse(self, space: PossibilitySpace,
                 collapse_type: CollapseType = CollapseType.CONSCIOUS,
                 chosen_id: Optional[str] = None) -> ChoiceEvent:

        if space.collapsed:
            raise ValueError("Space already collapsed")

        if chosen_id and collapse_type == CollapseType.CONSCIOUS:
            chosen = next((p for p in space.possibilities if p.id == chosen_id), None)
            if not chosen:
                raise ValueError(f"Possibility {chosen_id} not found")
        else:
            weights = [p.potential for p in space.possibilities]
            chosen = random.choices(space.possibilities, weights=weights)[0]

        lost = []
        for p in space.possibilities:
            if p.id == chosen.id:
                p.actualize()
            else:
                p.lose()
                lost.append(p)

        space.collapsed = True
        space.chosen = chosen

        event = ChoiceEvent(
            before=space.count,
            after=1,
            chosen=chosen,
            lost=lost,
            collapse_type=collapse_type
        )

        self.history.append(event)
        return event

class ChoiceMechanism:

    def __init__(self):
        self.mechanism = ChoiceEngine()

    def create_possibility_space(self, options: List[str],
                                  potentials: Optional[List[float]] = None) -> PossibilitySpace:
        if potentials is None:
            potentials = [1.0 / len(options)] * len(options)

        possibilities = [
            Possibility(id=opt, potential=pot)
            for opt, pot in zip(options, potentials)
        ]

        return PossibilitySpace(possibilities=possibilities)

    def choose(self, space: PossibilitySpace,
               chosen_id: str,
               collapse_type: CollapseType = CollapseType.CONSCIOUS) -> ChoiceEvent:
        return self.mechanism.collapse(space, collapse_type, chosen_id)

    def let_collapse(self, space: PossibilitySpace,
                     collapse_type: CollapseType = CollapseType.RANDOM) -> ChoiceEvent:
        return self.mechanism.collapse(space, collapse_type)

    def get_doc(self) -> Dict:
        return {
            'what': 'mechanism for choice',
            'does': 'collapses possibilities into reality',
            'cost': 'loss of all unchosen possibilities',
            'processual': 'choice, collapse, actualization',
            'necessity': 'without choice nothing exists',
            'relation_to_encoding': 'opposites: expansion vs contraction'
        }
