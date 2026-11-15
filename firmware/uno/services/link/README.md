# link

**Layer:** services. **Version:** in `library.properties`.

The link protocol: parses messages from the computer and builds messages to it. See the protocol in [firmware/README.md](../../../README.md#protocol).

| | |
|---|---|
| **Requirements** | D4 (version message), F3 and F4 (drive and stop messages), F14 (camera message with optional speed) |
| **Interface** | `Parser::feed(c)` returns a `Command`; `MessageWriter` builds `{type,field,...}` |
| **Unit tests** | `tests/test_link.cpp` |
| **Monitors** | None yet. They arrive with diagnostics in increment 2 |

Test this unit on its own with `python scripts/dev.py test link`.
