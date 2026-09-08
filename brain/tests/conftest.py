"""Put the package on the path and provide the fakes the tests share.

Deliberately no fixtures that need numpy, PIL, dlib or a network: this suite
has to pass on a bare Python install, for the same reason IRIS's own does.
"""

from __future__ import annotations

import asyncio
import inspect
import pathlib
import sys
from dataclasses import dataclass
from typing import Any, List, Optional, Sequence, Tuple

BRAIN_DIR = pathlib.Path(__file__).resolve().parents[1]
if str(BRAIN_DIR) not in sys.path:
    sys.path.insert(0, str(BRAIN_DIR))

import pytest

from robot_eye.faces import BackendSpec, Box, COSINE, EUCLIDEAN


class FakeImage:
    """Stands in for a decoded frame: only ``.shape`` is ever read."""

    def __init__(self, width: int = 800, height: int = 600):
        self.shape = (height, width, 3)


@dataclass
class FakeRecognizer:
    """A recogniser with scripted answers, so the logic around it is testable.

    ``faces`` is a list of (Box, embedding) pairs that ``embed`` returns and
    ``detect`` returns the boxes of.
    """

    faces: List[Tuple[Box, Tuple[float, ...]]]
    spec: BackendSpec = BackendSpec(
        name="fake",
        modules=(),
        metric=EUCLIDEAN,
        threshold=0.5,
        can_embed=True,
        install_hint="",
    )
    detect_calls: int = 0
    embed_calls: int = 0

    @property
    def name(self) -> str:
        return self.spec.name

    @property
    def metric(self) -> str:
        return self.spec.metric

    @property
    def threshold(self) -> float:
        return self.spec.threshold

    def detect(self, image: Any) -> List[Box]:
        self.detect_calls += 1
        return [box for box, _ in self.faces]

    def embed(self, image: Any, boxes: Optional[Sequence[Box]] = None):
        self.embed_calls += 1
        return list(self.faces)


DETECT_ONLY_SPEC = BackendSpec(
    name="fake-detect-only",
    modules=(),
    metric=EUCLIDEAN,
    threshold=0.0,
    can_embed=False,
    install_hint="pip install something",
    note="Detection only.",
)


@pytest.fixture
def fake_image() -> FakeImage:
    return FakeImage()


@pytest.fixture
def decode_fake(fake_image):
    """A decoder that ignores the bytes and hands back a fixed-size frame."""

    def decode(_frame: bytes) -> FakeImage:
        return fake_image

    return decode


@pytest.fixture
def store(tmp_path):
    from robot_eye.store import FaceStore

    return FaceStore(tmp_path / "faces.json")


def big_box(width: int = 800, height: int = 600, fraction: float = 0.2) -> Box:
    """A centred box covering roughly ``fraction`` of a frame."""
    import math

    side = int(math.sqrt(fraction * width * height))
    return Box(x=(width - side) // 2, y=(height - side) // 2, w=side, h=side)


def tiny_box() -> Box:
    return Box(x=0, y=0, w=8, h=8)


#: A pair of 4-d vectors that are close, and one that is far, under both
#: metrics — enough to exercise matching without any model.
NEAR_A = (1.0, 0.0, 0.0, 0.0)
NEAR_B = (0.98, 0.05, 0.0, 0.0)
FAR = (0.0, 0.0, 0.0, 1.0)


# --------------------------------------------------------------------------
# Running the async tests
#
# IRIS's own suite has pytest-asyncio with ``asyncio_mode = "auto"``. This
# suite has to pass on a bare install too, so coroutine tests are driven
# here. A conftest hook takes precedence over a plugin's, so this runs
# whether or not pytest-asyncio is present and the two never both try.
# --------------------------------------------------------------------------

def pytest_pyfunc_call(pyfuncitem):
    if not inspect.iscoroutinefunction(pyfuncitem.obj):
        return None
    kwargs = {
        name: pyfuncitem.funcargs[name]
        for name in pyfuncitem._fixtureinfo.argnames
        if name in pyfuncitem.funcargs
    }
    asyncio.run(pyfuncitem.obj(**kwargs))
    return True
