# Security Policy

## Scope

OpenBolo is a game that includes network code inherited from WinBolo 1.15
(UDP multiplayer, WinBoloNet HTTP tracker client). Potential security-sensitive
areas include:

- UDP packet parsing (`src/bolo/udppackets.c`, `src/bolo/netmt.c`)
- HTTP client (`src/winbolonet/http.c`)
- Server transport layer (`src/server/win32/servertransport.c`)

## Reporting a Vulnerability

Please **do not** open a public issue for security vulnerabilities.

Report vulnerabilities by opening a
[confidential issue](../../issues/new?confidential=true) in GitLab (check the
"This issue is confidential" box). Include:

- A description of the vulnerability and its potential impact
- Steps to reproduce or a proof-of-concept
- Any suggested fix if you have one

We will acknowledge the report within a few days and aim to release a fix
before public disclosure.
