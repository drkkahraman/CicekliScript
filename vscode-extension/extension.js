const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { spawn } = require('child_process');

let runStatusBarItem;
let extensionContext = null;
let guiWebviewPanel = null;

/**
 * CicekliScript çalıştırılabilir dosyasını (binary) çözümler:
 * 1. Kullanıcı özel ayarı (cicekliscript.executablePath)
 * 2. Eklenti içindeki yerleşik derleyici (extension/bin/cicekliscript.exe veya extension/bin/cicekliscript)
 * 3. Sistem dizinleri (~/.local/bin, /usr/local/bin, PATH)
 */
function resolveExecutable() {
    // 1. Kullanıcı özel ayarı
    const config = vscode.workspace.getConfiguration('cicekliscript');
    let customPath = config.get('executablePath');
    if (!customPath) {
        const oldConfig = vscode.workspace.getConfiguration('metescript');
        customPath = oldConfig.get('executablePath');
    }
    if (customPath && customPath.trim() !== '') {
        return customPath.trim();
    }

    // 2. Eklenti içine gömülü yerel derleyici (Bundled Binary)
    const extDir = (extensionContext && extensionContext.extensionPath) ? extensionContext.extensionPath : __dirname;
    const isWin = process.platform === 'win32';
    const binName = isWin ? 'cicekliscript.exe' : 'cicekliscript';
    const binAlt = isWin ? 'ccl.exe' : 'ccl';

    const bundledCandidates = [
        path.join(extDir, 'bin', binName),
        path.join(extDir, 'bin', binAlt),
        path.join(__dirname, 'bin', binName),
        path.join(__dirname, 'bin', binAlt)
    ];

    for (const bp of bundledCandidates) {
        try {
            if (fs.existsSync(bp)) {
                // Unix sistemlerde çalıştırma iznini garanti et (+x)
                if (!isWin) {
                    try {
                        const stat = fs.statSync(bp);
                        if ((stat.mode & 0o111) === 0) {
                            fs.chmodSync(bp, 0o755);
                        }
                    } catch (e) {
                        // yetki hatası olursa yoksay
                    }
                }
                return bp;
            }
        } catch (e) {
            // yoksay
        }
    }

    // 3. Sistem dizinlerini ara (~/.local/bin, /usr/local/bin, /usr/bin, Program Files, PATH)
    const homeDir = os.homedir();
    const candidatePaths = isWin ? [
        path.join(homeDir, '.cicekliscript', 'bin', 'ccl.exe'),
        path.join(homeDir, '.cicekliscript', 'bin', 'cicekliscript.exe'),
        'ccl.exe',
        'cicekliscript.exe',
        'ccl',
        'cicekliscript'
    ] : [
        path.join(homeDir, '.local', 'bin', 'ccl'),
        path.join(homeDir, '.local', 'bin', 'cicekliscript'),
        path.join(homeDir, '.local', 'bin', 'cicekli'),
        path.join(homeDir, '.local', 'bin', 'metescript'),
        '/usr/local/bin/ccl',
        '/usr/local/bin/cicekliscript',
        '/usr/bin/ccl',
        '/usr/bin/cicekliscript',
        'ccl',
        'cicekliscript',
        'metescript'
    ];

    for (const p of candidatePaths) {
        if (p === 'ccl' || p === 'cicekliscript' || p === 'metescript' || p === 'ccl.exe' || p === 'cicekliscript.exe') return p;
        try {
            if (fs.existsSync(p)) return p;
        } catch (e) {
            // yoksay
        }
    }

    return isWin ? 'cicekliscript.exe' : 'cicekliscript';
}

function getTerminal() {
    const termName = 'CicekliScript';
    let term = vscode.window.terminals.find(t => t.name === termName);
    if (!term) {
        term = vscode.window.createTerminal(termName);
    }
    return term;
}

/**
 * Windows (PowerShell / CMD) ve Unix için komut dizesini hazırlar.
 * Windows PowerShell'de tırnak içine alınmış yolların doğrudan çalışabilmesi için
 * '& "path\to\exe" "arg"' sözdizimi zorunludur.
 */
function formatRunCommand(exe, args = []) {
    const isWin = process.platform === 'win32';
    const argsStr = args.map(a => `"${a}"`).join(' ');

    if (isWin) {
        const shell = (vscode.env.shell || '').toLowerCase();
        const isCmd = shell.endsWith('cmd.exe');
        if (isCmd) {
            return `"${exe}" ${argsStr}`.trim();
        }
        // PowerShell veya varsayılan Windows terminali
        return `& "${exe}" ${argsStr}`.trim();
    }
    return `"${exe}" ${argsStr}`.trim();
}

/**
 * Eklenti içindeki derleyiciyi doğrudan sisteme kurar:
 * Linux/macOS: ~/.local/bin/cicekliscript ve ccl
 * Windows: %USERPROFILE%\.cicekliscript\bin\cicekliscript.exe ve ccl.exe
 */
async function installToSystem() {
    const extDir = (extensionContext && extensionContext.extensionPath) ? extensionContext.extensionPath : __dirname;
    const isWin = process.platform === 'win32';
    const binName = isWin ? 'cicekliscript.exe' : 'cicekliscript';

    const sourceBin = path.join(extDir, 'bin', binName);
    if (!fs.existsSync(sourceBin)) {
        vscode.window.showErrorMessage(`CicekliScript: Eklenti içindeki derleyici ikili dosyası bulunamadı (${sourceBin})!`);
        return;
    }

    const homeDir = os.homedir();
    const targetDir = isWin ? path.join(homeDir, '.cicekliscript', 'bin') : path.join(homeDir, '.local', 'bin');

    try {
        if (!fs.existsSync(targetDir)) {
            fs.mkdirSync(targetDir, { recursive: true });
        }

        const targetBin = path.join(targetDir, binName);
        fs.copyFileSync(sourceBin, targetBin);

        if (isWin) {
            const targetCcl = path.join(targetDir, 'ccl.exe');
            fs.copyFileSync(sourceBin, targetCcl);
        } else {
            fs.chmodSync(targetBin, 0o755);
            const symlinks = ['ccl', 'cicekli', 'metescript'];
            for (const sym of symlinks) {
                const symPath = path.join(targetDir, sym);
                try {
                    if (fs.existsSync(symPath) || fs.lstatSync(symPath).isSymbolicLink()) {
                        fs.unlinkSync(symPath);
                    }
                } catch (e) {}
                try {
                    fs.symlinkSync(targetBin, symPath);
                } catch (e) {}
            }
        }

        const pathEnv = process.env.PATH || '';
        const inPath = pathEnv.split(path.delimiter).some(p => path.resolve(p) === path.resolve(targetDir));

        let msg = `✓ CicekliScript derleyicisi başarıyla "${targetDir}" dizinine kuruldu!`;
        if (!inPath) {
            if (isWin) {
                msg += `\nNot: 'ccl' komutunu PowerShell/CMD'den her yerde kullanabilmek için bu dizini Windows Ortam Değişkenleri PATH listesine ekleyebilirsiniz.`;
            } else {
                msg += `\nNot: Bu dizini PATH değişkeninize eklemek için terminalde şunu çalıştırabilirsiniz: export PATH="$HOME/.local/bin:$PATH"`;
            }
        }

        const choice = await vscode.window.showInformationMessage(msg, 'Terminalde Test Et', 'Tamam');
        if (choice === 'Terminalde Test Et') {
            const term = getTerminal();
            term.show(true);
            term.sendText(formatRunCommand(targetBin, ['--version']));
        }
    } catch (err) {
        vscode.window.showErrorMessage(`CicekliScript kurulurken bir hata oluştu: ${err.message}`);
    }
}

async function runCicekliScriptFile(uri) {
    let filePath;

    if (uri && uri.fsPath) {
        filePath = uri.fsPath;
    } else {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('CicekliScript: Çalıştırılacak aktif bir dosya bulunamadı!');
            return;
        }
        if (editor.document.isDirty) {
            await editor.document.save();
        }
        filePath = editor.document.uri.fsPath;
    }

    const ext = path.extname(filePath).toLowerCase();
    if (ext !== '.ccl' && ext !== '.cicekli' && ext !== '.ms' && ext !== '.metescript') {
        const docLang = vscode.window.activeTextEditor && vscode.window.activeTextEditor.document.languageId;
        if (docLang !== 'cicekliscript' && docLang !== 'metescript') {
            const answer = await vscode.window.showWarningMessage(
                `"${path.basename(filePath)}" bir CicekliScript (.ccl) dosyası gibi görünmüyor. Yine de çalıştırmak ister misiniz?`,
                'Evet', 'İptal'
            );
            if (answer !== 'Evet') return;
        }
    }

    const exe = resolveExecutable();
    const term = getTerminal();
    term.show(true);

    const config = vscode.workspace.getConfiguration('cicekliscript');
    if (config.get('clearTerminalBeforeRun', true)) {
        term.sendText(process.platform === 'win32' ? 'cls' : 'clear');
    }

    // Dosyayı çalıştır (PowerShell/CMD/Unix uyumlu format)
    term.sendText(formatRunCommand(exe, [filePath]));
}

async function runSelection() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return;

    const selection = editor.selection;
    const text = selection.isEmpty ? editor.document.lineAt(selection.active.line).text : editor.document.getText(selection);
    if (!text || text.trim() === '') return;

    const exe = resolveExecutable();
    const term = getTerminal();
    term.show(true);

    // Kodu tek satırda -c ile çalıştır
    term.sendText(formatRunCommand(exe, ['-c', text]));
}

function openRepl() {
    const exe = resolveExecutable();
    const term = getTerminal();
    term.show(true);
    term.sendText(formatRunCommand(exe, []));
}

async function runGuiPreview(uri) {
    let filePath;
    if (uri && uri.fsPath) {
        filePath = uri.fsPath;
    } else {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('CicekliScript: Önizlenecek aktif bir dosya bulunamadı!');
            return;
        }
        if (editor.document.isDirty) {
            await editor.document.save();
        }
        filePath = editor.document.uri.fsPath;
    }

    const exe = resolveExecutable();
    if (!exe) {
        vscode.window.showErrorMessage('CicekliScript: Derleyici ikili dosyası bulunamadı!');
        return;
    }

    if (!guiWebviewPanel) {
        guiWebviewPanel = vscode.window.createWebviewPanel(
            'cicekliscriptGui',
            'CicekliScript GUI Önizleme',
            vscode.ViewColumn.Beside,
            {
                enableScripts: true,
                retainContextWhenHidden: true
            }
        );
        guiWebviewPanel.onDidDispose(() => {
            guiWebviewPanel = null;
        });
    } else {
        guiWebviewPanel.reveal(vscode.ViewColumn.Beside);
    }

    guiWebviewPanel.webview.html = getLoadingWebviewHtml(path.basename(filePath));

    // CICEKLI_GUI_HEADLESS=1 ile çalıştırıp JSON dökümünü al
    const proc = spawn(exe, [filePath], {
        env: { ...process.env, CICEKLI_GUI_HEADLESS: '1' }
    });

    let stdout = '';
    let stderr = '';

    proc.stdout.on('data', (d) => { stdout += d.toString(); });
    proc.stderr.on('data', (d) => { stderr += d.toString(); });

    proc.on('close', (code) => {
        let guiData = null;
        const re = /(?:CICEKLI_GUI|METE_GUI)\s+(\{.*\})/g;
        let match;
        while ((match = re.exec(stdout)) !== null) {
            try {
                guiData = JSON.parse(match[1]);
            } catch (e) {}
        }

        const cleanStdout = stdout.split('\n').filter(line => !/^(?:CICEKLI_GUI|METE_GUI)\s+\{/.test(line.trim())).join('\n');

        if (guiWebviewPanel) {
            guiWebviewPanel.webview.html = getGuiWebviewHtml(path.basename(filePath), guiData, cleanStdout, stderr, code);
        }
    });

    proc.on('error', (err) => {
        if (guiWebviewPanel) {
            guiWebviewPanel.webview.html = getErrorWebviewHtml(path.basename(filePath), err.message);
        }
    });
}

function getGuiWebviewHtml(fileName, guiData, stdout, stderr, exitCode) {
    const dataJson = JSON.stringify(guiData || null);
    return `<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<style>
  body {
    background: var(--vscode-editor-background, #1e1e1e);
    color: var(--vscode-editor-foreground, #d4d4d4);
    font-family: var(--vscode-font-family, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif);
    margin: 0;
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }
  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding-bottom: 8px;
    border-bottom: 1px solid var(--vscode-panel-border, #333);
  }
  .title {
    font-size: 16px;
    font-weight: 600;
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .tag {
    font-size: 11px;
    padding: 2px 6px;
    border-radius: 4px;
    background: #0284c7;
    color: #fff;
  }
  .canvas-card {
    background: #111;
    border: 1px solid var(--vscode-panel-border, #444);
    border-radius: 8px;
    overflow: hidden;
    display: inline-block;
    align-self: flex-start;
    box-shadow: 0 4px 16px rgba(0,0,0,0.4);
  }
  .window-bar {
    background: #252526;
    padding: 6px 12px;
    font-size: 12px;
    display: flex;
    align-items: center;
    gap: 8px;
    border-bottom: 1px solid #333;
  }
  .window-dots {
    display: flex;
    gap: 4px;
  }
  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
  }
  .dot-red { background: #ff5f56; }
  .dot-yellow { background: #ffbd2e; }
  .dot-green { background: #27c93f; }
  canvas {
    display: block;
    background: #ffffff;
  }
  .terminal-box {
    background: var(--vscode-terminal-background, #141414);
    border: 1px solid var(--vscode-panel-border, #333);
    border-radius: 6px;
    padding: 12px;
    font-family: var(--vscode-editor-font-family, monospace);
    font-size: 12px;
    max-height: 200px;
    overflow-y: auto;
    white-space: pre-wrap;
  }
  .term-title {
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--vscode-descriptionForeground, #888);
    margin-bottom: 6px;
  }
  .no-gui {
    padding: 20px;
    background: rgba(255, 193, 7, 0.1);
    border: 1px solid rgba(255, 193, 7, 0.3);
    border-radius: 6px;
    color: #ffc107;
  }
</style>
</head>
<body>
  <div class="header">
    <div class="title">
      <span>📐 CicekliScript GUI Önizleme</span>
      <span class="tag">${fileName}</span>
    </div>
    <div style="font-size: 12px; color: #888;">Çıkış Kodu: ${exitCode}</div>
  </div>

  <div id="guiContainer"></div>

  <div>
    <div class="term-title">Çıktı &amp; Loglar</div>
    <div class="terminal-box">${stdout || stderr ? (stdout + (stderr ? '\nHata:\n' + stderr : '')) : '(Standart çıktı yok)'}</div>
  </div>

  <script>
    const data = ${dataJson};
    const container = document.getElementById('guiContainer');

    function rgbStr(f) {
      const c = (v) => Math.max(0, Math.min(255, Math.round(v * 255)));
      return 'rgb(' + c(f[0]) + ',' + c(f[1]) + ',' + c(f[2]) + ')';
    }

    if (data && data.w && data.h) {
      const card = document.createElement('div');
      card.className = 'canvas-card';

      const bar = document.createElement('div');
      bar.className = 'window-bar';
      bar.innerHTML = '<div class="window-dots"><span class="dot dot-red"></span><span class="dot dot-yellow"></span><span class="dot dot-green"></span></div><span style="margin-left: 6px;">' + (data.title || 'CicekliScript Penceresi') + ' (' + data.w + 'x' + data.h + ')</span>';
      card.appendChild(bar);

      const canvas = document.createElement('canvas');
      canvas.width = data.w;
      canvas.height = data.h;
      card.appendChild(canvas);
      container.appendChild(card);

      const ctx = canvas.getContext('2d');
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, data.w, data.h);

      (data.ops || []).forEach(op => {
        const type = op[0];
        if (type === 'color') {
          ctx.fillStyle = rgbStr(op.slice(1, 4));
          ctx.fillRect(0, 0, data.w, data.h);
        } else if (type === 'rect') {
          ctx.fillStyle = rgbStr(op.slice(5, 8));
          ctx.fillRect(op[1], op[2], op[3], op[4]);
        } else if (type === 'circle') {
          ctx.fillStyle = rgbStr(op.slice(4, 7));
          ctx.beginPath();
          ctx.arc(op[1], op[2], Math.max(0, op[3]), 0, 2 * Math.PI);
          ctx.fill();
        } else if (type === 'line') {
          ctx.strokeStyle = rgbStr(op.slice(5, 8));
          ctx.lineWidth = op[8] || 1;
          ctx.beginPath();
          ctx.moveTo(op[1], op[2]);
          ctx.lineTo(op[3], op[4]);
          ctx.stroke();
        } else if (type === 'text') {
          ctx.fillStyle = rgbStr(op.slice(2, 5));
          ctx.font = 'bold ' + (op[5] || 14) + 'px Sans, Arial, sans-serif';
          ctx.fillText(op[1], op[6], op[7] + (op[5] || 14));
        }
      });
    } else {
      container.innerHTML = '<div class="no-gui">⚠️ Bu programda aktif bir GUI penceresi (<code>cicekli.window(...)</code>) oluşturulmadı veya henüz çizim yapılmadı.<br><br>💡 İpucu: GUI penceresini yerel masaüstünüzde (Windows / Linux penceresi olarak) çalıştırmak için yukarıdaki <strong>▶ CicekliScript</strong> (Ctrl+F5) butonunu kullanabilirsiniz.</div>';
    }
  </script>
</body>
</html>`;
}

function getLoadingWebviewHtml(fileName) {
    return `<!DOCTYPE html><html><body style="background:#1e1e1e;color:#ccc;font-family:sans-serif;padding:24px;"><h3>⏳ "${fileName}" çalıştırılıyor ve grafikler çiziliyor...</h3></body></html>`;
}

function getErrorWebviewHtml(fileName, err) {
    return `<!DOCTYPE html><html><body style="background:#1e1e1e;color:#ff5f56;font-family:sans-serif;padding:24px;"><h3>❌ Hata: ${fileName}</h3><pre>${err}</pre></body></html>`;
}

function updateStatusBarItem() {
    const editor = vscode.window.activeTextEditor;
    if (editor && (editor.document.languageId === 'cicekliscript' || editor.document.languageId === 'metescript' || editor.document.fileName.endsWith('.ccl') || editor.document.fileName.endsWith('.cicekli') || editor.document.fileName.endsWith('.ms'))) {
        runStatusBarItem.show();
    } else {
        runStatusBarItem.hide();
    }
}

function activate(context) {
    extensionContext = context;

    // 1. Çalıştırma Komutları
    const runCmd = vscode.commands.registerCommand('cicekliscript.run', (uri) => {
        return runCicekliScriptFile(uri);
    });
    const runCmdOld = vscode.commands.registerCommand('metescript.run', (uri) => {
        return runCicekliScriptFile(uri);
    });

    const runTerminalCmd = vscode.commands.registerCommand('cicekliscript.runInTerminal', (uri) => {
        return runCicekliScriptFile(uri);
    });
    const runTerminalCmdOld = vscode.commands.registerCommand('metescript.runInTerminal', (uri) => {
        return runCicekliScriptFile(uri);
    });

    const runSelectionCmd = vscode.commands.registerCommand('cicekliscript.runSelection', () => {
        return runSelection();
    });
    const runSelectionCmdOld = vscode.commands.registerCommand('metescript.runSelection', () => {
        return runSelection();
    });

    const replCmd = vscode.commands.registerCommand('cicekliscript.openRepl', () => {
        return openRepl();
    });

    const installCmd = vscode.commands.registerCommand('cicekliscript.installToSystem', () => {
        return installToSystem();
    });

    const openDocsCmd = vscode.commands.registerCommand('cicekliscript.openDocs', () => {
        vscode.env.openExternal(vscode.Uri.parse('https://drkkahraman.github.io/CicekliScript/'));
    });
    const openDocsCmdOld = vscode.commands.registerCommand('metescript.openDocs', () => {
        vscode.env.openExternal(vscode.Uri.parse('https://drkkahraman.github.io/CicekliScript/'));
    });

    const runGuiPreviewCmd = vscode.commands.registerCommand('cicekliscript.runGuiPreview', (uri) => {
        return runGuiPreview(uri);
    });

    // 2. Status Bar Butonu (▶ CicekliScript Çalıştır)
    runStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    runStatusBarItem.command = 'cicekliscript.run';
    runStatusBarItem.text = '$(play) CicekliScript';
    runStatusBarItem.tooltip = 'Aktif CicekliScript dosyasını çalıştır (Ctrl+F5)';

    context.subscriptions.push(
        runCmd,
        runCmdOld,
        runTerminalCmd,
        runTerminalCmdOld,
        runSelectionCmd,
        runSelectionCmdOld,
        replCmd,
        installCmd,
        openDocsCmd,
        openDocsCmdOld,
        runGuiPreviewCmd,
        runStatusBarItem,
        vscode.window.onDidChangeActiveTextEditor(updateStatusBarItem),
        vscode.workspace.onDidOpenTextDocument(updateStatusBarItem)
    );

    updateStatusBarItem();
}

function deactivate() {
    if (runStatusBarItem) {
        runStatusBarItem.dispose();
    }
}

module.exports = {
    activate,
    deactivate
};
