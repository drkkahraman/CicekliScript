# CicekliScript for Visual Studio Code

<p align="center">
  <img src="icons/ccl.png" width="128" height="128" alt="CicekliScript Logo" />
</p>

Official Visual Studio Code extension for the **CicekliScript** programming language.

A complete development environment for CicekliScript, bringing together Python's ease of use, pure C11 performance, mandatory `;` discipline, and native NVIDIA CUDA tensor acceleration!

---

## 🌟 Key Feature: Built-in Bundled Compiler (Zero-Setup!)

**No need to manually install GCC, Make, or compile CicekliScript on your machine!**

This extension **bundles the compiled CicekliScript binary directly inside the package.** As soon as you install the extension in VS Code:
1. Open any `.ccl` file and click the **"▶" (Run)** button in the top right—your code **runs immediately** using the extension's built-in compiler.
2. Want to use `ccl` in external terminals too? Simply right-click anywhere in the editor and choose **"CicekliScript: Install Language to System (~/.local/bin/ccl)"** to install the binary to your system PATH with a single click!

---

## ✨ Features

- 📦 **Bundled C11 Compiler:** Zero-configuration execution directly out of the extension box.
- 🎨 **Advanced Syntax Highlighting:**
  - Full support for `cicekli.println`, `cicekli.print`, `cicekli.input`, `cicekli.deffunction`, `cicekli.window`, `cicekli.cuda`, `cicekli.tensor`
  - Control flow: `if`, `elif`, `else`, `while`, `for in`, `return`, `break`, `continue`, `def`
  - Built-in `math.*` and `doc.*` / `file.*` modules
  - Mandatory semicolon `;` visual feedback
- 🏷️ **Custom File Icons:** Branded, sleek file icons for `.ccl`, `.cicekli`, and `.ms` files in the explorer.
- ▶️ **One-Click Run:**
  - **"▶"** (Play) button in the editor title bar
  - `Ctrl + F5` (or `Cmd + F5` on macOS) keyboard shortcut
  - Right-click context menu: **"CicekliScript: Run File"**
  - Run selected code snippet directly (`cicekliscript.runSelection`)
- 💻 **Integrated Interactive REPL:**
  - Launch the CicekliScript REPL shell in the integrated terminal with one click.
- ⚡ **Rich Code Snippets:**
  - `println` → `cicekli.println("Hello World");`
  - `input` → `a = cicekli.input("Prompt: ");`
  - `deffunction` → `cicekli.deffunction fn() { ... };`
  - `window` → Complete Cairo/X11 GUI window template
  - `tensor` → CUDA tensor & matrix multiplication template
- 📑 **Auto Indentation & Bracket Matching:** Auto-closing `{}` blocks, quotes, and smart formatting.

---

## 🚀 Installation & Usage

1. In VS Code, open the **Extensions** panel (`Ctrl + Shift + X`).
2. Click the `...` (three dots) menu in the top right and select **"Install from VSIX..."**.
3. Select `cicekliscript-1.1.0.vsix`.
4. Open any `.ccl` file and hit the **"▶"** button in the top right to run!
