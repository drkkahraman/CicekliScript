const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const os = require('os');

let runStatusBarItem;

function resolveExecutable() {
    const config = vscode.workspace.getConfiguration('cicekliscript');
    let customPath = config.get('executablePath');
    if (!customPath) {
        const oldConfig = vscode.workspace.getConfiguration('metescript');
        customPath = oldConfig.get('executablePath');
    }
    if (customPath && customPath.trim() !== '') {
        return customPath.trim();
    }

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
        // Platforma göre terminal temizleme
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

function updateStatusBarItem() {
    const editor = vscode.window.activeTextEditor;
    if (editor && (editor.document.languageId === 'cicekliscript' || editor.document.languageId === 'metescript' || editor.document.fileName.endsWith('.ccl') || editor.document.fileName.endsWith('.cicekli') || editor.document.fileName.endsWith('.ms'))) {
        runStatusBarItem.show();
    } else {
        runStatusBarItem.hide();
    }
}

function activate(context) {
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

    const openDocsCmd = vscode.commands.registerCommand('cicekliscript.openDocs', () => {
        vscode.env.openExternal(vscode.Uri.parse('http://localhost:6000'));
    });
    const openDocsCmdOld = vscode.commands.registerCommand('metescript.openDocs', () => {
        vscode.env.openExternal(vscode.Uri.parse('http://localhost:6000'));
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
