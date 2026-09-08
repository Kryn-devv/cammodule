"""Intent rules for the camera tools — a block to paste into IRIS.

WHERE THIS GOES
``iris/app/nlu/rules.py``, inside the ``RULES`` list, immediately after the
``device_status_query`` rule and before ``hinglish_weather``.

WHY THAT EXACT PLACE, AND NOT AT THE END
Rules are ordered and the first match wins. "who is that" already matches the
``who_is`` rule further down, which sends it to Wikipedia — so a camera rule
appended to the end of the list would never see it. Inserting here puts these
ahead of ``who_is`` and behind everything device-specific, which is what makes
"who is that", said to a robot with eyes, mean the person in front of it.

WHAT IS DELIBERATELY LEFT ALONE
* "is there anyone" / "is there any motion" already route to ``device_sensors``
  and the PIR sensor. That is a good answer — the PIR sees the whole room, the
  camera only its own view — so it is not taken. The camera answers the
  narrower "can you see anyone" and "is anyone in front of you".
* "look at me" already routes to ``face_emotion`` and makes the OLED eyes turn
  toward you. Also a good answer, also left alone.
* "this is Prakash" is not made a rule. It would match "this is a test" and
  "this is fine" just as readily. Introducing someone in those words still
  works — it goes to the LLM agent, which has ``camera_remember_face`` in its
  tool list and will ask for a name if it needs one.

NAMES ARRIVE LOWERCASED
``normalize_command`` lowercases everything before matching, so a captured
name comes through as "prakash". The builders title-case it, because the robot
says it out loud and reads it back on screen. A name that title-case gets wrong
("van der berg") can be corrected by saying it explicitly to the tool.
"""

BLOCK_TO_PASTE = '''
    # ------------------------------------------------- the camera (robot eye)
    # These sit ahead of `who_is` on purpose: see iris_integration/nlu_rules.py.
    Rule(
        name="camera_remember_face",
        intent="devices",
        tool="camera_remember_face",
        pattern=_rx(
            r"^(?:remember|learn|store|save)\\s+(?:my|this)\\s+face\\s+as\\s+(?P<name>[a-z][a-z .'-]{1,40})$"
            r"|^remember\\s+me\\s+as\\s+(?P<name2>[a-z][a-z .'-]{1,40})$"
            r"|^(?:mera|mere)\\s+chehra\\s+yaad\\s+rakho\\s*,?\\s*main\\s+(?P<name3>[a-z][a-z .'-]{1,40})\\s+hoon$"
        ),
        builder=lambda m, c: (
            {"name": raw.strip().title()}
            if (raw := (m.group("name") or m.group("name2") or m.group("name3")))
            else None
        ),
        confidence=0.96,
    ),
    Rule(
        name="camera_forget_all_faces",
        intent="devices",
        tool="camera_forget_face",
        pattern=_rx(
            r"^forget\\s+(?:all|every|everyone(?:'s)?)\\s+(?:the\\s+)?faces?"
            r"(?:\\s+you\\s+know)?$|^forget\\s+everyone$"
        ),
        static_args={"everyone": True},
        confidence=0.96,
    ),
    Rule(
        name="camera_forget_face",
        intent="devices",
        tool="camera_forget_face",
        pattern=_rx(
            r"^forget\\s+(?:my|this)\\s+face$"
            r"|^forget\\s+(?P<name>[a-z][a-z .'-]{1,40}?)(?:'s)?\\s+face$"
        ),
        builder=lambda m, c: (
            {"name": m.group("name").strip().title()} if m.group("name") else {}
        ),
        confidence=0.95,
    ),
    Rule(
        name="camera_known_faces",
        intent="devices",
        tool="camera_known_faces",
        pattern=_rx(
            r"^(?:who\\s+do\\s+you\\s+(?:know|recognise|recognize)"
            r"|whose\\s+faces?\\s+do\\s+you\\s+(?:know|remember)"
            r"|(?:list|show)\\s+(?:the\\s+)?(?:known\\s+)?faces"
            r"|which\\s+faces\\s+do\\s+you\\s+know"
            r"|kisko\\s+pehchante\\s+ho)\\??$"
        ),
        confidence=0.96,
    ),
    Rule(
        name="camera_who",
        intent="devices",
        tool="camera_who",
        pattern=_rx(
            r"^(?:who\\s+am\\s+i"
            r"|who\\s+(?:is|are)\\s+(?:this|that|there|in\\s+front\\s+of\\s+you)"
            r"|do\\s+you\\s+(?:recognise|recognize|know)\\s+me"
            # "can you see me" arrives as "see me": normalize_command strips
            # "can you " as politeness before any rule is tried.
            r"|(?:can\\s+you\\s+)?see\\s+me"
            r"|who\\s+do\\s+you\\s+see"
            r"|am\\s+i\\s+(?:the\\s+)?(?:one|owner)"
            r"|main\\s+kaun\\s+hoon"
            r"|mujhe\\s+pehchano"
            r"|kaun\\s+hai\\s+(?:ye|wahan))\\??$"
        ),
        confidence=0.96,
    ),
    Rule(
        name="camera_look_text",
        intent="devices",
        tool="camera_look",
        pattern=_rx(
            r"^(?:read\\s+(?:this|the|that)(?:\\s+(?:label|text|screen|page|sign|writing))?"
            r"|what\\s+does\\s+(?:this|that|the\\s+label)\\s+say"
            r"|ye\\s+kya\\s+likha\\s+hai)\\??$"
        ),
        static_args={"kind": "text"},
        confidence=0.95,
    ),
    Rule(
        name="camera_look_ripeness",
        intent="devices",
        tool="camera_look",
        pattern=_rx(
            r"^(?:is\\s+(?:this|that|the)\\s*(?:[a-z]+\\s+)?(?:ripe|fresh|rotten|off|bad)"
            r"|(?:how\\s+)?ripe\\s+is\\s+(?:this|that|it)"
            r"|does\\s+(?:this|that|it)\\s+look\\s+(?:ripe|fresh))\\??$"
        ),
        static_args={"kind": "ripeness"},
        confidence=0.95,
    ),
    Rule(
        name="camera_look_count",
        intent="devices",
        tool="camera_look",
        pattern=_rx(
            r"^how\\s+many\\s+(?:things|objects|items)\\s+"
            r"(?:can\\s+you\\s+see|are\\s+(?:there|in\\s+front\\s+of\\s+you))\\??$"
        ),
        static_args={"kind": "count"},
        confidence=0.95,
    ),
    Rule(
        name="camera_look_object",
        intent="devices",
        tool="camera_look",
        pattern=_rx(
            r"^(?:what(?:'s|\\s+is)\\s+this(?:\\s+(?:thing|object))?"
            r"|what(?:'s|\\s+is)\\s+that"
            r"|what\\s+am\\s+i\\s+(?:holding|showing\\s+you)"
            r"|identify\\s+(?:this|that)(?:\\s+object)?"
            r"|(?:look\\s+at|check)\\s+this(?:\\s+object)?"
            r"|ye\\s+kya\\s+hai)\\??$"
        ),
        static_args={"kind": "object"},
        confidence=0.95,
    ),
    Rule(
        name="camera_look_scene",
        intent="devices",
        tool="camera_look",
        pattern=_rx(
            r"^(?:what\\s+(?:do|can)\\s+you\\s+see"
            r"|what(?:'s|\\s+is)\\s+(?:in\\s+front\\s+of\\s+you|there)"
            r"|(?:look|have\\s+a\\s+look)\\s+(?:around|ahead)"
            r"|describe\\s+(?:what\\s+you\\s+see|the\\s+(?:room|scene|view))"
            r"|kya\\s+dikh\\s+raha\\s+hai)\\??$"
        ),
        static_args={"kind": "scene"},
        confidence=0.95,
    ),
    Rule(
        name="camera_presence",
        intent="devices",
        tool="camera_presence",
        pattern=_rx(
            # "can you see anyone" arrives as "see anyone" — normalize_command
            # strips "can you " before matching.
            r"^(?:(?:can\\s+you\\s+)?see\\s+(?:anyone|anybody|someone)"
            r"|is\\s+(?:anyone|anybody|someone)\\s+in\\s+front\\s+of\\s+you"
            r"|is\\s+(?:anyone|anybody)\\s+(?:there|around)\\s+(?:on\\s+)?(?:the\\s+)?camera"
            r"|has\\s+anything\\s+moved"
            r"|any\\s+movement\\s+(?:on\\s+)?(?:the\\s+)?camera"
            r"|koi\\s+samne\\s+hai(?:\\s+kya)?)\\??$"
        ),
        confidence=0.95,
    ),
'''

if __name__ == "__main__":  # pragma: no cover - convenience for the installer
    print(BLOCK_TO_PASTE)
