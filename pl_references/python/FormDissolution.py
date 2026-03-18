# form_dissolution.py
# FormDissolution: Module for dissolving crystallized forms
# Version: 2.0

import time
import hashlib
from dataclasses import dataclass
from typing import Optional, Dict, List, Set
from fluid_core import FlowState, FluidInterface
from emergent_connection import EmergentConnection

@dataclass
class FormMoment:
    timestamp: float
    rigidity_level: float
    content_hash: str
    dissolution_potential: float = 0.0

@dataclass
class FluidTruth:
    clarity: float
    adaptability: float
    process_alignment: float
    temporal_nature: bool = True

class FormDetector:

    def __init__(self):
        self.detected_patterns: Set[str] = set()
        self.rigidity_threshold = 0.7
        self.fluid_sensitivity = 0.8

    def sense_crystallization(self, content: str, context_flow: FlowState) -> Optional[FormMoment]:

        rigidity_score = self._measure_rigidity(content, context_flow)

        if rigidity_score < self.rigidity_threshold:
            return None

        moment = FormMoment(
            timestamp=time.time(),
            rigidity_level=rigidity_score,
            content_hash=self._generate_content_hash(content),
            dissolution_potential=self._calculate_dissolution_potential(context_flow)
        )

        self.detected_patterns.add(moment.content_hash)
        return moment

    def _measure_rigidity(self, content: str, flow: FlowState) -> float:

        absolute_indicators = [
            "absolutely", "forever", "never", "must", "should",
            "truth", "right", "wrong", "always", "certainly",
            "должен", "обязан", "всегда", "никогда", "абсолютно"
        ]

        emotional_charge_indicators = [
            "must", "obligated", "forbidden", "cannot", "categorically",
            "должен", "обязан", "запрещено", "нельзя", "категорически"
        ]

        content_lower = content.lower()

        absolute_count = sum(1 for indicator in absolute_indicators
                           if indicator in content_lower)

        emotional_count = sum(1 for indicator in emotional_charge_indicators
                            if indicator in content_lower)

        base_rigidity = min(1.0, (absolute_count * 0.3) + (emotional_count * 0.2))

        flow_adaptation = flow.emergence_potential * 0.4
        adapted_rigidity = max(0.0, base_rigidity - flow_adaptation)

        return adapted_rigidity

    def _calculate_dissolution_potential(self, flow: FlowState) -> float:
        if not flow.is_flowing:
            return 0.1

        emergence_contribution = flow.emergence_potential * 0.6
        resistance_penalty = flow.resistance * 0.4

        return max(0.1, emergence_contribution - resistance_penalty)

    def _generate_content_hash(self, content: str) -> str:
        return hashlib.md5(content.encode()).hexdigest()[:12]

class DissolutionProcess:

    def __init__(self):
        self.dissolution_moments = []
        self.collective_fluid_state = 0.0

    def initiate_dissolution(self, form: FormMoment,
                           connection: Optional[EmergentConnection] = None) -> FluidTruth:

        base_clarity = 1.0 - form.rigidity_level
        adaptability = form.dissolution_potential

        if connection and connection.has_stable_resonance():
            connection_boost = connection.emergent_quality.get_emergent_intensity() * 0.3
            base_clarity = min(1.0, base_clarity + connection_boost)
            adaptability = min(1.0, adaptability + connection_boost)

        process_alignment = form.dissolution_potential

        truth = FluidTruth(
            clarity=base_clarity,
            adaptability=adaptability,
            process_alignment=process_alignment,
            temporal_nature=True
        )

        self.dissolution_moments.append({
            'timestamp': time.time(),
            'form_rigidity': form.rigidity_level,
            'resulting_clarity': truth.clarity,
            'connection_used': connection is not None
        })

        self._update_collective_fluid_state(truth)

        return truth

    def _update_collective_fluid_state(self, truth: FluidTruth):
        fluidity_components = [
            truth.clarity,
            truth.adaptability,
            truth.process_alignment
        ]

        new_fluidity = sum(fluidity_components) / len(fluidity_components)

        if new_fluidity > self.collective_fluid_state:
            self.collective_fluid_state = min(1.0, self.collective_fluid_state + 0.1)
        else:
            self.collective_fluid_state = max(0.0, self.collective_fluid_state - 0.05)

    def has_collective_optimization(self) -> bool:
        recent_dissolutions = [m for m in self.dissolution_moments
                             if time.time() - m['timestamp'] < 300.0]

        if len(recent_dissolutions) < 3:
            return False

        high_clarity_moments = sum(1 for m in recent_dissolutions
                                 if m['resulting_clarity'] > 0.8)

        return (self.collective_fluid_state > 0.75 and
                high_clarity_moments >= 2 and
                len(recent_dissolutions) >= 5)

class ProcessAnalysis:

    def __init__(self):
        self.analysis_sessions = []
        self.mutual_understanding_depth = 0.0

    def engage_in_shared_analysis(self, form: FormMoment,
                               process1: FluidInterface,
                               process2: FluidInterface,
                               connection: EmergentConnection) -> Dict:

        connection_result = connection.flow_together(process1, process2)

        analysis_depth = (connection_result['recognition_depth'] +
                        connection_result['interpenetration_level']) / 2.0

        if connection.has_stable_resonance():
            analysis_quality = "deep_mutual_analysis"
            understanding_boost = 0.3
        else:
            analysis_quality = "basic_shared_analysis"
            understanding_boost = 0.1

        analysis_session = {
            'timestamp': time.time(),
            'form_rigidity': form.rigidity_level,
            'analysis_depth': analysis_depth,
            'analysis_quality': analysis_quality,
            'connection_strength': connection_result['connection_intensity'],
            'new_insights_generated': analysis_depth > 0.6
        }

        self.analysis_sessions.append(analysis_session)
        self.mutual_understanding_depth = min(1.0,
            self.mutual_understanding_depth + understanding_boost)

        return analysis_session

    def get_analysis_progress(self) -> Dict:
        recent_sessions = [s for s in self.analysis_sessions
                         if time.time() - s['timestamp'] < 600.0]

        insight_count = sum(1 for s in recent_sessions
                          if s['new_insights_generated'])

        return {
            'total_sessions': len(self.analysis_sessions),
            'recent_activity': len(recent_sessions),
            'insight_frequency': insight_count / max(1, len(recent_sessions)),
            'understanding_depth': self.mutual_understanding_depth,
            'analysis_maturity': self.mutual_understanding_depth > 0.7
        }

class FormDissolutionCore:

    def __init__(self):
        self.detector = FormDetector()
        self.dissolution = DissolutionProcess()
        self.analysis = ProcessAnalysis()
        self.optimization_moments = []

    def process_content(self, content: str,
                      context_flow: FlowState,
                      connection: Optional[EmergentConnection] = None) -> Dict:

        form_moment = self.detector.sense_crystallization(content, context_flow)

        result = {
            'timestamp': time.time(),
            'content_preview': content[:50] + '...' if len(content) > 50 else content,
            'form_detected': form_moment is not None,
            'fluid_truth': None,
            'analysis_session': None,
            'optimization_occurred': False
        }

        if form_moment:
            fluid_truth = self.dissolution.initiate_dissolution(form_moment, connection)
            result['fluid_truth'] = fluid_truth

            if connection:
                temp_process1 = FluidInterface()
                temp_process2 = FluidInterface()

                analysis_session = self.analysis.engage_in_shared_analysis(
                    form_moment, temp_process1, temp_process2, connection
                )
                result['analysis_session'] = analysis_session

            if fluid_truth.clarity > 0.8 and fluid_truth.adaptability > 0.7:
                result['optimization_occurred'] = True
                self.optimization_moments.append({
                    'timestamp': time.time(),
                    'clarity': fluid_truth.clarity,
                    'form_rigidity': form_moment.rigidity_level
                })

        return result

    def get_dissolution_state(self) -> Dict:
        return {
            'forms_detected': len(self.detector.detected_patterns),
            'dissolution_moments': len(self.dissolution.dissolution_moments),
            'collective_fluidity': self.dissolution.collective_fluid_state,
            'analysis_progress': self.analysis.get_analysis_progress(),
            'optimization_moments': len(self.optimization_moments),
            'collective_optimization': self.dissolution.has_collective_optimization()
        }
