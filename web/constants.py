from typing import Literal, Union
from pydantic import BaseModel



class PathMakerSubmission(BaseModel):
    engine: Literal["PathMaker"]
    item_id: int
    rotations: int
    rotate: int


class SpiralAboutCenterSubmission(BaseModel):
    engine: Literal["SpiralAboutCenter"]
    item_id: int
    rotations: int
    r0: int
    r1: int


EngineSubmission = Union[
    PathMakerSubmission,
    SpiralAboutCenterSubmission
]