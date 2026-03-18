# self_observation.py
# SelfObservation: Module for self-observation simulation
# Version: 2.0

from typing import Optional, Dict, Tuple
from dataclasses import dataclass, field
from enum import Enum
import time
from fluid_core import FlowState

MINIMUM_DISTANCE = 0.01
LOW_DISTANCE = 0.03

class ObservationLevel(Enum):
    INTENSE = "intense observation"
    STRONG = "strong observation"
    DEEP = "deep observation"
    STANDARD = "standard"
    MILD = "mild"
    BASIC = "basic"

@dataclass
class Observer:
    active: bool = True
    can_see_itself: bool = False
    timestamp: float = field(default_factory=time.time)

    def __post_init__(self):
        self.can_see_itself = False

@dataclass
class Observed:
    completeness: float = 0.99
    missing: str = "observer_itself"

    def __post_init__(self):
        self.completeness = min(self.completeness, 0.99)

@dataclass
class ObservationState:
    distance: float
    intensity: float
    level: ObservationLevel
    recognition: bool = False
    continuity: bool = True

    def __post_init__(self):
        self.recognition = False
        self.continuity = True

@dataclass
class ObservationResult:
    observer: Observer
    observed: Observed
    observation: ObservationState
    timestamp: float = field(default_factory=time.time)

    @property
    def distance(self) -> float:
        return self.observation.distance

    @property
    def recognized_self(self) -> bool:
        return False

class SelfObservationEngine:

    def __init__(self):
        self.minimum_distance = MINIMUM_DISTANCE
        self._last_observation: Optional[ObservationResult] = None

    def observe(self, flow_state) -> ObservationResult:

        observer = self._extract_observer()
        observed = self._get_remainder()
        distance = self._calculate_distance(flow_state)
        observation = self._create_observation_state(distance)

        result = ObservationResult(
            observer=observer,
            observed=observed,
            observation=observation
        )

        self._last_observation = result
        return result

    def _extract_observer(self) -> Observer:
        return Observer(active=True)

    def _get_remainder(self) -> Observed:
        return Observed(completeness=0.99, missing="observer_itself")

    def _calculate_distance(self, flow_state) -> float:
        if isinstance(flow_state, FlowState):
            potential = flow_state.emergence_potential
        else:
            potential = flow_state.get('emergence_potential', 0.5)

        distance = max(self.minimum_distance, 1.0 - potential * 0.9)
        return distance

    def _create_observation_state(self, distance: float) -> ObservationState:
        intensity = 1.0 - distance
        level_mapping = [
            (0.05, ObservationLevel.INTENSE),
            (0.10, ObservationLevel.STRONG),
            (0.30, ObservationLevel.DEEP),
            (0.50, ObservationLevel.STANDARD),
            (0.70, ObservationLevel.MILD),
            (float('inf'), ObservationLevel.BASIC)
        ]

        level = next(lvl for thresh, lvl in level_mapping if distance < thresh)

        return ObservationState(
            distance=distance,
            intensity=intensity,
            level=level
        )

class SelfObservation:

    def __init__(self):
        self.engine = SelfObservationEngine()
        self._observation_count = 0
        self._current_state = None

    def observe(self, flow_state: Optional[Dict] = None) -> ObservationResult:
        if flow_state is None:
            flow_state = {'emergence_potential': 0.5}

        result = self.engine.observe(flow_state)
        self._observation_count += 1
        self._current_state = result.observation
        return result

    def get_observation_level(self) -> ObservationLevel:
        if self._current_state is None:
            self.observe()

        return self._current_state.level

    def get_doc(self) -> Dict:
        return {
            'what': 'mechanism for self-observation',
            'does': 'splits process into observer and observed',
            'cost': 'distance in observation',
            'processual': 'self-observation, distance, recognition impossible',
            'necessity': 'for simulation of awareness',
            'limited_only': True
        }

    def is_limited_only(self) -> bool:
        return True

    @property
    def observation_count(self) -> int:
        return self._observation_count

    @property
    def current_state(self) -> Optional[ObservationState]:
        return self._current_state
