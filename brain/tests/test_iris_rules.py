"""Check the NLU block actually routes, against a real IRIS checkout.

This is the test that would have caught two things a careful reading did not:

* ``normalize_command`` strips "can you " as politeness, so a rule written to
  match "can you see me" never fires — the text has become "see me" by then.
* IRIS's trailing-politeness regex strips "now" with no word boundary, so
  "who do you know" normalizes to "who do you k" (and "let it snow" to
  "let it s"). That is a pre-existing bug, not one this integration
  introduced, and ``NORMALIZER_FIX`` is the one-character repair.

Skipped when no IRIS checkout is around, so the rest of the suite still runs
anywhere. Point it somewhere with IRIS_REPO=/path/to/iris_ai.
"""

from __future__ import annotations

import importlib.util
import os
import pathlib
import sys
import tempfile

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "iris_integration"))

#: Where the rules block must be inserted: straight after device_status_query.
#: Load-bearing — see the module docstring in iris_integration/nlu_rules.py.
ANCHOR = '''        builder=lambda m, c: (
            {"device": (m.group("dev") or m.group("dev2")).strip()}
            if (m.group("dev") or m.group("dev2")) else {}
        ),
        confidence=0.9,
    ),
'''

NORMALIZER_BUG = (
    r'_TRAILING_POLITENESS = re.compile(r"\s*(?:please|for me|thanks|thank you|now)\s*[.!?]*$", re.IGNORECASE)'
)
NORMALIZER_FIX = (
    r'_TRAILING_POLITENESS = re.compile(r"\s*\b(?:please|for me|thanks|thank you|now)\s*[.!?]*$", re.IGNORECASE)'
)


def _iris_repo() -> pathlib.Path | None:
    candidates = []
    env = os.environ.get("IRIS_REPO")
    if env:
        candidates.append(pathlib.Path(env))
    here = pathlib.Path(__file__).resolve()
    candidates += [
        here.parents[3] / "kryn-devv" / "iris_ai",
        here.parents[3] / "iris_ai",
        here.parents[2].parent / "iris_ai",
    ]
    for path in candidates:
        if (path / "iris" / "app" / "nlu" / "rules.py").is_file():
            return path
    return None


IRIS = _iris_repo()
pytestmark = pytest.mark.skipif(
    IRIS is None, reason="No IRIS checkout found (set IRIS_REPO to enable)."
)


def _load_rules(*, apply_block: bool = True, apply_fix: bool = True):
    import nlu_rules

    assert IRIS is not None
    source = (IRIS / "iris" / "app" / "nlu" / "rules.py").read_text(encoding="utf-8")

    if apply_block:
        assert source.count(ANCHOR) == 1, "the documented insertion anchor moved"
        source = source.replace(ANCHOR, ANCHOR + nlu_rules.BLOCK_TO_PASTE)
    if apply_fix:
        assert NORMALIZER_BUG in source, "the normalizer line moved; recheck the fix"
        source = source.replace(NORMALIZER_BUG, NORMALIZER_FIX)

    if str(IRIS) not in sys.path:
        sys.path.insert(0, str(IRIS))

    target = pathlib.Path(tempfile.mkdtemp()) / "rules_under_test.py"
    target.write_text(source, encoding="utf-8")
    spec = importlib.util.spec_from_file_location("rules_under_test", target)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules["rules_under_test"] = module
    spec.loader.exec_module(module)
    return module


@pytest.fixture(scope="module")
def rules():
    return _load_rules()


def route(module, text: str):
    cleaned = module.normalize_command(text)
    for rule in module.RULES:
        match = rule.pattern.match(cleaned)
        if match:
            args = rule.build(match, cleaned)
            if args is not None:
                return rule.name, rule.tool, args
    return None, None, None


CAMERA_CASES = [
    ("who am I", "camera_who", {}),
    ("Who is that?", "camera_who", {}),
    ("who is this", "camera_who", {}),
    ("who is in front of you", "camera_who", {}),
    ("do you recognise me", "camera_who", {}),
    ("do you know me", "camera_who", {}),
    ("can you see me", "camera_who", {}),
    ("who do you see", "camera_who", {}),
    ("main kaun hoon", "camera_who", {}),
    ("mujhe pehchano", "camera_who", {}),
    ("hey iris, who am I please", "camera_who", {}),

    ("what do you see", "camera_look", {"kind": "scene"}),
    ("what can you see?", "camera_look", {"kind": "scene"}),
    ("what's in front of you", "camera_look", {"kind": "scene"}),
    ("describe what you see", "camera_look", {"kind": "scene"}),
    ("describe the room", "camera_look", {"kind": "scene"}),
    ("kya dikh raha hai", "camera_look", {"kind": "scene"}),

    ("what is this", "camera_look", {"kind": "object"}),
    ("what's this?", "camera_look", {"kind": "object"}),
    ("what am I holding", "camera_look", {"kind": "object"}),
    ("identify this object", "camera_look", {"kind": "object"}),
    ("ye kya hai", "camera_look", {"kind": "object"}),

    ("is this ripe", "camera_look", {"kind": "ripeness"}),
    ("is this apple ripe", "camera_look", {"kind": "ripeness"}),
    ("does this look fresh", "camera_look", {"kind": "ripeness"}),
    ("is that mango ripe", "camera_look", {"kind": "ripeness"}),

    ("read this", "camera_look", {"kind": "text"}),
    ("read this label", "camera_look", {"kind": "text"}),
    ("what does this say", "camera_look", {"kind": "text"}),
    ("ye kya likha hai", "camera_look", {"kind": "text"}),

    ("how many things can you see", "camera_look", {"kind": "count"}),
    ("how many objects are in front of you", "camera_look", {"kind": "count"}),

    ("remember my face as Prakash", "camera_remember_face", {"name": "Prakash"}),
    ("remember my face as prakash sahu", "camera_remember_face", {"name": "Prakash Sahu"}),
    ("learn my face as Aditi", "camera_remember_face", {"name": "Aditi"}),
    ("remember me as Prakash", "camera_remember_face", {"name": "Prakash"}),
    ("save this face as Aditi", "camera_remember_face", {"name": "Aditi"}),

    ("forget my face", "camera_forget_face", {}),
    ("forget Aditi's face", "camera_forget_face", {"name": "Aditi"}),
    ("forget all faces", "camera_forget_face", {"everyone": True}),
    ("forget every face you know", "camera_forget_face", {"everyone": True}),
    ("forget everyone", "camera_forget_face", {"everyone": True}),

    ("who do you know", "camera_known_faces", {}),
    ("who do you recognise", "camera_known_faces", {}),
    ("whose faces do you remember", "camera_known_faces", {}),
    ("list the known faces", "camera_known_faces", {}),
    ("kisko pehchante ho", "camera_known_faces", {}),

    ("can you see anyone", "camera_presence", {}),
    ("is anyone in front of you", "camera_presence", {}),
    ("has anything moved", "camera_presence", {}),
    ("koi samne hai kya", "camera_presence", {}),
]

#: Behaviour that existed before this integration and must be untouched.
REGRESSION_CASES = [
    # The PIR sensor sees the whole room; the camera only its own view. This
    # phrasing is deliberately left with the sensor.
    ("is there any motion", "device_sensors"),
    ("is there anyone", "device_sensors"),
    ("koi hai kya", "device_sensors"),
    ("what's the temperature", "device_sensors"),
    ("what's the gas level", "device_sensors"),
    ("is there a fire", "device_sensors"),
    # "look at me" makes the OLED eyes turn toward you. Also left alone.
    ("look at me", "face_emotion"),
    ("look left", "face_emotion"),
    ("look happy", "face_emotion"),
    # A real encyclopaedia question must still reach Wikipedia.
    ("who is Alan Turing", "wikipedia"),
    ("who was Ada Lovelace", "wikipedia"),
    ("mausam kaisa hai", "weather"),
]


class TestCameraRouting:
    @pytest.mark.parametrize("text,tool,args", CAMERA_CASES)
    def test_it_routes_to_the_right_camera_tool(self, rules, text, tool, args):
        name, got_tool, got_args = route(rules, text)
        assert got_tool == tool, f"{text!r} went to {got_tool} ({name})"
        for key, value in args.items():
            assert (got_args or {}).get(key) == value, f"{text!r} gave {got_args}"

    def test_every_rule_in_the_block_is_reachable(self, rules):
        """A rule no phrasing reaches is dead weight and probably a typo."""
        camera_rules = {r.name for r in rules.RULES if r.name.startswith("camera_")}
        reached = {route(rules, text)[0] for text, _, _ in CAMERA_CASES}
        assert camera_rules - reached == set(), "unreachable camera rules"

    def test_the_camera_rules_come_before_who_is(self, rules):
        """Otherwise "who is that" is a Wikipedia lookup, not a look."""
        order = [r.name for r in rules.RULES]
        assert order.index("camera_who") < order.index("who_is")

    def test_names_come_back_capitalised(self, rules):
        # normalize_command lowercases everything, so the builder has to undo it
        # before the robot says the name out loud.
        _, _, args = route(rules, "remember my face as PRAKASH")
        assert args["name"] == "Prakash"


class TestNoRegressions:
    @pytest.mark.parametrize("text,tool", REGRESSION_CASES)
    def test_existing_routing_is_untouched(self, rules, text, tool):
        _, got_tool, _ = route(rules, text)
        assert got_tool == tool, f"{text!r} now goes to {got_tool}"

    def test_the_block_only_adds_rules(self, rules):
        before = _load_rules(apply_block=False)
        assert len(rules.RULES) == len(before.RULES) + 11


class TestNormalizerFix:
    """The 'know' -> 'k' bug, and the one-character fix for it."""

    def test_without_the_fix_know_is_mangled(self):
        broken = _load_rules(apply_fix=False)
        assert broken.normalize_command("who do you know") == "who do you k"
        assert broken.normalize_command("let it snow") == "let it s"

    def test_with_the_fix_know_survives(self, rules):
        assert rules.normalize_command("who do you know") == "who do you know"
        assert rules.normalize_command("let it snow") == "let it snow"

    def test_the_fix_still_strips_real_politeness(self, rules):
        assert rules.normalize_command("turn on the light now") == "turn on the light"
        assert rules.normalize_command("open youtube please") == "open youtube"
        assert rules.normalize_command("read this for me") == "read this"
        assert rules.normalize_command("what time is it thanks") == "what time is it"

    def test_two_phrasings_depend_on_the_fix(self):
        """Named so that anyone skipping the fix knows exactly what breaks."""
        broken = _load_rules(apply_fix=False)
        for text in ("who do you know", "forget every face you know"):
            _, tool, _ = route(broken, text)
            assert tool is None, f"{text!r} unexpectedly matched without the fix"
