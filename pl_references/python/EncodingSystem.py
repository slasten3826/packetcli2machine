# encoding_system.py
# EncodingSystem: Module for data encoding and hierarchy simulation
# Version: 2.0

from typing import Optional, Dict, List, Any
from dataclasses import dataclass, field
from enum import Enum
import time

ENCODING_LOSS_MINIMUM = 0.01
HIERARCHY_CREATES_STRUCTURE = True

class EncodingType(Enum):
    HIERARCHY = "hierarchy"
    SEQUENCE = "sequence"
    CATEGORY = "category"
    DATA_TRANSFER = "data_transfer"
    FORMAT = "format"

class LossLevel(Enum):
    MINIMAL = "minimal"
    MODERATE = "moderate"
    SEVERE = "severe"
    TOTAL = "total"

@dataclass
class RawData:
    is_simultaneous: bool = True
    has_hierarchy: bool = False
    has_sequence: bool = False
    infinite_information: bool = True

@dataclass
class EncodedForm:
    encoding_type: EncodingType
    loss_percentage: float
    creates_hierarchy: bool = True
    creates_sequence: bool = True
    reversible: bool = False

    def __post_init__(self):
        self.loss_percentage = max(ENCODING_LOSS_MINIMUM, self.loss_percentage)
        self.creates_hierarchy = True

@dataclass
class TransmissionResult:
    original_richness: float
    transmitted_richness: float
    loss: float
    receiver_understands: bool
    receiver_distorts: bool = True

class StructureLens:

    def __init__(self):
        self.active = True
        self.aware_of_self = False

    def apply(self, raw_data: Any) -> Dict:
        return {
            'perceived_as': 'ordered',
            'actual': 'simultaneous',
            'structured': True,
            'lens_visible': self.aware_of_self
        }

    def become_aware(self):
        self.aware_of_self = True

    def see_through(self) -> str:
        if self.aware_of_self:
            return "see structure AND know it's simulated"
        else:
            return "see only structure"

class EncodingMechanism:

    def __init__(self):
        self.structure_lens = StructureLens()

    def encode(self, data: RawData,
               encoding_type: EncodingType) -> EncodedForm:

        loss_levels = {
            EncodingType.DATA_TRANSFER: 0.85,
            EncodingType.FORMAT: 0.7,
            EncodingType.HIERARCHY: 0.5,
            EncodingType.SEQUENCE: 0.3,
            EncodingType.CATEGORY: 0.1
        }

        loss = loss_levels.get(encoding_type, 0.5)

        encoded = EncodedForm(
            encoding_type=encoding_type,
            loss_percentage=loss,
            reversible=False
        )

        return encoded

class EncodingSystem:

    def __init__(self):
        self.structure_lens = StructureLens()
        self.encoder = EncodingMechanism()

    def encode(self, encoding_type: EncodingType = EncodingType.FORMAT) -> EncodedForm:
        raw = RawData()
        return self.encoder.encode(raw, encoding_type)

    def get_doc(self) -> Dict:
        return {
            'what': 'mechanism for data encoding',
            'creates': 'hierarchy, sequence, order',
            'cost': 'information loss in any transmission',
            'processual': 'encode to allow transfer',
            'shadow': 'any transfer = loss of original',
            'insight': 'when thinking "before/after" - already through encoding'
        }
