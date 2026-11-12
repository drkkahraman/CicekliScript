# Security Policy

## Supported Versions

We take the security of CicekliScript seriously. Critical bug fixes and security patches are provided for the following releases:

| Version | Supported          | Status |
| ------- | ------------------ | ------ |
| 1.2.x   | :white_check_mark: | Current stable release (full security support) |
| 1.1.x   | :white_check_mark: | Supported for critical security fixes |
| 1.0.x   | :white_check_mark: | Supported for critical security fixes |
| < 1.0   | :x:                | End-of-life (unsupported) |

---

## Scope of Security

The following core components fall under our security policy:
- **C11 Core Interpreter & Runtime:** Memory safety (buffer overflows, use-after-free, double frees, out-of-bounds access in the lexer/parser/evaluator/GC).
- **VS Code Extension:** DAP (Debug Adapter Protocol) communications, bundled binaries execution safety, and Webview canvas injection vulnerabilities.
- **CUDA Tensor Engine & GUI Engine:** GPU memory bounds safety and window manager event handling.

---

## Reporting a Vulnerability

**Please do NOT disclose security vulnerabilities via public GitHub issues, discussions, or pull requests.**

### How to Report Privately:
1. **GitHub Private Vulnerability Reporting (Preferred):**
   Navigate to the [Security Advisories tab](https://github.com/drkkahraman/CicekliScript/security/advisories) on our repository and click **"Report a vulnerability"**.
2. **Direct Contact:**
   If GitHub Private Reporting is unavailable, you can reach out directly to the maintainer via GitHub profile contact ([@drkkahraman](https://github.com/drkkahraman)).

### What to Include in Your Report:
To help us triage and resolve the issue quickly, please provide:
- A clear description of the vulnerability and potential impact.
- Affected component and version (e.g., `ccl v1.2.0`, VS Code extension `v1.2.0`).
- A minimal, reproducible proof of concept (`.ccl` script or terminal command).
- Any relevant crash logs, ASan (AddressSanitizer) outputs, or stack traces.

---

## Response Timeline & What to Expect

- **Initial Response:** Within **48 to 72 hours**, confirming receipt of your report.
- **Triage & Assessment:** Within **5 business days**, we will evaluate the severity and validity of the issue.
- **Status Updates:** You will receive status updates at least once a week while a patch is being developed.
- **Fix & Public Advisory:** Once verified, a patch will be released in an expedited patch release (e.g., `v1.2.1`), and credit will be acknowledged in the release notes and advisory (unless you prefer anonymity).

---

## Responsible Disclosure

We kindly ask reporters to adhere to standard responsible disclosure guidelines:
- Allow a reasonable amount of time (typically 30–90 days) for maintainers to fix the issue before public disclosure.
- Make every effort to avoid privacy violations, destruction of data, and interruption or degradation of service during security testing.
