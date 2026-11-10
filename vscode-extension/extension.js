const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const os = require('os');

let runStatusBarItem;
let extensionContext = null;

/**
 * CicekliScript çalıştırılabilir dosyasını (binary) çözümler:
 * 1. Kullanıcı özel ayarı (cicekliscript.executablePath)
 * 2. Eklenti içindeki yerleşik derleyici (extension/bin/cicekliscript)
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

    // 3. Sistem dizinlerini ara (~/.local/bin, /usr/local/bin, /usr/bin)
    const homeDir = os.homedir();
    const candidatePaths = [
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
        if (p === 'ccl' || p === 'cicekliscript' || p === 'metescript') return p;
        try {
            if (fs.existsSync(p)) return p;
        } catch (e) {
            // yoksay
        }
    }

    return 'cicekliscript';
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
 * Eklenti içindeki derleyiciyi doğrudan sisteme (~/.local/bin) kurar.
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

        if (!isWin) {
            fs.chmodSync(targetBin, 0o755);
            // ccl ve cicekli sembolik linklerini oluştur
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

        let msg = `✓ CicekliScript eklenti içinden "${targetDir}" dizinine başarıyla kuruldu!`;
        if (!inPath && !isWin) {
            msg += `\nNot: Bu dizini PATH değişkeninize eklemek için terminalde şunu çalıştırabilirsiniz: export PATH="$HOME/.local/bin:$PATH"`;
        }

        const choice = await vscode.window.showInformationMessage(msg, 'Terminalde Test Et', 'Tamam');
        if (choice === 'Terminalde Test Et') {
            const term = getTerminal();
            term.show(true);
            term.sendText(`"${targetBin}" --version`);
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

    // Dosyayı çalıştır
    term.sendText(`"${exe}" "${filePath}"`);
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
    const escaped = text.replace(/"/g, '\\"');
    term.sendText(`"${exe}" -c "${escaped}"`);
}

function openRepl() {
    const exe = resolveExecutable();
    const term = getTerminal();
    term.show(true);
    term.sendText(`"${exe}"`);
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
